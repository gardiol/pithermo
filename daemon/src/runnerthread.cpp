#include "runnerthread.h"

#include <string.h>
#include <time.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <sstream>

#include "configfile.h"
#include "debugprint.h"
#include "profiler.h"

#include "command.h"
#include "program.h"

// GPIO
// 5 = minimo
// 4 = ???
// 6 = pellet
// 0 = gas
// 2 = quarto relé libero
// 7 = pellet feedback
RunnerThread::RunnerThread(ConfigFile *config,
                           const std::string &exchange_path,
                           const std::string &hst, Logger *l):
    ScheduledThread("Runner", 1000 * 1000 ),
    _logger(l),
    _gas(nullptr),
    _pellet(nullptr),
    _temp_sensor(nullptr),
    _history( hst, exchange_path ),
    _config( config ),
    _exchange_path( exchange_path ),
    _commands_mutex("CommandsMutex"),
    _manual_mode(true),
    _activated(false),
    _manual_gas_on(false),
    _anti_ice_active(false),
    _over_temp(false),
    _over_temp_start_time(0),
    _under_temp(false),
    _pellet_flameout(false),
    _resume_gas_on(false),
    _resume_pellet_mod(false),
    _prev_pellet_hot(false),
    _min_temp(16.0),
    _max_temp(17.0),
    _temp_correction(1.0),
    _sensor_success_reads(0),
    _last_time(0),
    _day(0),
    _hour(0),
    _half_hour(0),
    _current_ext_temp(0.0),
    _current_ext_humidity(0.0),
    _temp_trend_last_valid(0),
    _temp_trend_prev(0.0),
    _temp_trend_D1prev(0.0),
    _temp_trend_D2mean(0.0),
    _pellet_startup_delay(60*45)
{
    _status_json_template.push_back("{\"mode\":\"");
    _status_json_template.push_back("\",\"active\":\"");
    _status_json_template.push_back("\",\"antiice\":\"");
    _status_json_template.push_back("\",\"warnings\":{\"modeswitch\":\"");
    _status_json_template.push_back("\"},\"pellet\":{\"command\":\"");
    _status_json_template.push_back("\",\"status\":\"");
    _status_json_template.push_back("\",\"minimum\":\"");
    _status_json_template.push_back("\",\"flameout\":\"");
    _status_json_template.push_back("\",\"time\":\"");
    _status_json_template.push_back("\",\"mintime\":\"");
    _status_json_template.push_back("\",\"stime\":\"");
    _status_json_template.push_back("\",\"smintime\":\"");
    _status_json_template.push_back("\"},\"gas\":{\"command\":\"");
    _status_json_template.push_back("\",\"time\":\"");
    _status_json_template.push_back("\",\"stime\":\"");
    _status_json_template.push_back("\",\"status\":\"off\"},\"temp\":{\"min\":");
    _status_json_template.push_back(",\"max\":");
    _status_json_template.push_back(",\"int\":");
    _status_json_template.push_back(",\"ext\":");
    _status_json_template.push_back(",\"hum\":");
    _status_json_template.push_back(",\"ext_hum\":");
    _status_json_template.push_back("},\"now\":{\"d\":");
    _status_json_template.push_back(",\"h\":");
    _status_json_template.push_back(",\"f\":");
    _status_json_template.push_back("},\"program\":");

    // Load config file
    uint32_t history_len = 1;
    std::string history_mode = "day";
    std::string content;
    if ( !_config->isEmpty() )
    {
        _logger->enableDebug( _config->getValueBool("debug") );
        _manual_mode = FrameworkUtils::string_tolower(_config->getValue( "mode" )) == "manual";
        _min_temp = static_cast<float>(FrameworkUtils::string_tof( _config->getValue( "min_temp" ) ) );
        _max_temp = static_cast<float>(FrameworkUtils::string_tof( _config->getValue( "max_temp" ) ) );
        _activated = _config->getValueBool( "activated" );
        if ( _config->hasValue( "pellet_startup_delay" ) )
            _pellet_startup_delay = static_cast<uint64_t>(FrameworkUtils::string_toi( _config->getValue( "pellet_startup_delay" ) ) );
        _temp_correction = static_cast<float>(FrameworkUtils::string_tof( _config->getValue( "temp_correction" ) ) );
        history_len = static_cast<uint32_t>(FrameworkUtils::string_toi( _config->getValue("history_len") ) );
        history_mode = FrameworkUtils::string_tolower( _config->getValue( "history_mode" ) );
        _program.loadConfig( _config->getSection( "program" ) );
    }
    else
        _saveConfig();

    // Load history log:
    _history.initialize( history_mode, history_len );
    _current_ext_temp = _history.getLastExtTemp();
    _current_ext_humidity = _history.getLastExtHumidity();

    // Ensure on times will not be zeroized later
    _updateCurrentTime( FrameworkTimer::getTimeEpoc() );

    uint64_t gas_on_time = _logger->getTodayGasOnTime();
    uint64_t pellet_on_time = _logger->getTodayPelletOnTime();
    uint64_t pellet_min_time = _logger->getTodayPelletLowTime();
    uint64_t gas_on_since = _logger->getTodayGasOnSince();
    uint64_t pellet_on_since = _logger->getTodayPelletOnSince();
    uint64_t pellet_low_on_since = _logger->getTodayPelletLowOnSince();
    uint64_t gas_season_on = _logger->getSeasonGasOnTime();
    uint64_t pellet_season_on = _logger->getSeasonPelletOnTime();
    uint64_t pellet_season_low = _logger->getSeasonPelletLowTime();

    // Initialize generators:
    _gas = new Generator( "gas", _logger, 0, -1, -1,
                          gas_on_time, 0,
                          gas_season_on, 0,
                          gas_on_since, 0,
                          LogItem::GAS_ON, LogItem::GAS_OFF,
                          LogItem::NO_EVENT, LogItem::NO_EVENT ); // command is GPIO 0
    _pellet = new Generator( "pellet", _logger, 6, 7, 5,
                             pellet_on_time, pellet_min_time,
                             pellet_season_on, pellet_season_low,
                             pellet_on_since, pellet_low_on_since,
                             LogItem::PELLET_ON, LogItem::PELLET_OFF,
                             LogItem::PELLET_MINIMUM, LogItem::PELLET_MODULATION ); // command = 2, status = 7, min/mod=5
    _temp_sensor = new TempSensor( _logger, 1, _temp_correction ); // 1 temp sensor

    // Ensure we print at next check
    _prev_pellet_hot = !_pellet->isHot();

    _updateStatus();
    startThread();
    while ( !isRunning() )
        FrameworkTimer::msleep_s( 100 );
}

RunnerThread::~RunnerThread()
{
    requestTerminate();
    waitForEnd();
    _commands_mutex.lock();
    for ( std::list<Command*>::iterator c = _commands_list.begin();
          c != _commands_list.end();
          ++c )
    {
        Command* cmd = *c;
        delete cmd;
    }
    _commands_list.clear();
    _commands_mutex.unlock();
    if ( _temp_sensor != nullptr )
    {
        delete _temp_sensor;
        _temp_sensor = nullptr;
    }
    if ( _gas != nullptr )
    {
        delete _gas;
        _gas = nullptr;
    }
    if ( _pellet != nullptr )
    {
        delete _pellet;
        _pellet = nullptr;
    }
    _saveConfig();
}

void RunnerThread::appendCommand(Command *cmd)
{
    _logger->logDebug("Appending command: " + cmd->commandStr() );
    _commands_mutex.lock();
    _commands_list.push_back( cmd );
    _commands_mutex.unlock();
}

bool RunnerThread::_checkCommands()
{
    bool update_status = false;
    bool save_config = false;
    _commands_mutex.lock();
    while ( _commands_list.size() > 0 )
    {
        Command* cmd = _commands_list.front();
        _commands_list.pop_front();
        switch ( cmd->command() )
        {
        case Command::ACTIVATE:
            _logger->logDebug("Activate command received");
            if ( !_activated )
            {
                _logger->logEvent( LogItem::ACTIVATED );
                save_config = true;
                _activated = true;
                update_status = true;
            }
            break;

        case Command::DEACTIVATE:
            _logger->logDebug("Deactivate command received");
            if ( _activated )
            {
                _logger->logEvent( LogItem::DEACTIVATED );
                save_config = true;
                _activated = false;
                update_status = true;
            }
            break;

        case Command::EXT_TEMP:
            _logger->logDebug("New EXT TEMP received: " + cmd->getParam());
        {
            std::vector<std::string> tokens = FrameworkUtils::string_split( cmd->getParam(), ":" );
            if ( tokens.size() == 3 )
            {
//                std::string sensor_id = tokens[0];
                _current_ext_temp = FrameworkUtils::string_tof( tokens[1] );
                _current_ext_humidity = FrameworkUtils::string_tof( tokens[2] );
            }
            else
                _logger->logDebug( "Error: invalid ext-temp (" + cmd->getParam() + ")");
        }
            break;

        case Command::FLAMEOUT_RESET:
            _logger->logDebug("Flameout reset received");
            if ( _pellet_flameout )
            {
                _pellet_flameout = false;
                _logger->logEvent( LogItem::PELLET_FLAMEOUT_OFF );
                update_status = true;
            }
            break;

        case Command::SET_HISTORY:
            _logger->logDebug("Change history received");
            _history.setMode( cmd->getParam() );
            _logger->logDebug("Change history, mode: " + cmd->getParam());
            break;

        case Command::PELLET_MINIMUM_ON:
            _logger->logDebug("Pellet Minimum ON received");
            if ( _manual_mode && _pellet->isOn() )
            {
                _pellet->setPower( Generator::POWER_LOW );
                update_status = true;
            }
            break;

        case Command::PELLET_MINIMUM_OFF:
            _logger->logDebug("Pellet Minimum OFF received");
            if ( _manual_mode && _pellet->isOn() )
            {
                _pellet->setPower( Generator::POWER_HIGH );
                update_status = true;
            }
            break;

        case Command::PELLET_ON:
            _logger->logDebug("Pellet ON received");
            if ( _manual_mode )
            {
                _pellet->switchOn();
                update_status = true;
            }
            break;

        case Command::PELLET_OFF:
            _logger->logDebug("Pellet OFF received");
            if ( _manual_mode )
            {
                _pellet->switchOff();
                update_status = true;
            }
            break;

        case Command::GAS_ON:
            _logger->logDebug("Gas ON received");
            if ( _manual_mode )
            {
                _gas->switchOn();
                _manual_gas_on = true;
                update_status = true;
            }
            break;

        case Command::GAS_OFF:
            _logger->logDebug("Gas OFF received");
            if ( _manual_mode )
            {
                _manual_gas_on = false;
                _gas->switchOff();
                update_status = true;
            }
            break;

        case Command::MANUAL:
            _logger->logDebug("Manual mode");
            _manual_mode = true;
            _logger->logEvent( LogItem::MANUAL_MODE );
            update_status = true;
            save_config = true;
            break;

        case Command::AUTO:
            _logger->logDebug("Auto mode");
            _manual_mode = false;
            _manual_gas_on = false;
            _logger->logEvent( LogItem::AUTO_MODE );
            update_status = true;
            save_config = true;
            break;

        case Command::SET_MIN_TEMP:
        {
            _logger->logDebug("New MIN TEMP received: " + cmd->getParam());
            float tmp_temp = FrameworkUtils::string_tof( cmd->getParam() );
            if ( (fabs(tmp_temp-_min_temp) > 0.05f) && (tmp_temp < _max_temp) )
            {
                _logger->logDebug("Changed min temp to " + cmd->getParam() );
                _logger->logEvent( LogItem::MIN_TEMP_UPDATE );
                _min_temp = tmp_temp;
                update_status = true;
                save_config = true;
            }
        }
            break;

        case Command::SET_MAX_TEMP:
        {
            _logger->logDebug("New MAX TEMP received: " + cmd->getParam());
            float tmp_temp = FrameworkUtils::string_tof( cmd->getParam() );
            if ( ( fabs(tmp_temp-_max_temp) > 0.05f) && (tmp_temp > _min_temp) )
            {
                _logger->logDebug("Changed max temp to " + cmd->getParam() );
                _logger->logEvent( LogItem::MAX_TEMP_UPDATE );
                _max_temp = tmp_temp;
                update_status = true;
                save_config = true;
            }
        }
            break;

        case Command::PROGRAM:
            _logger->logDebug("New program received: " + cmd->getParam() );
            if ( _program.change( cmd->getParam() ) )
            {
                _logger->logDebug("Program changed");
                _logger->logEvent( LogItem::PROGRAM_UPDATE );
                update_status = true;
                save_config = true;
            }
            break;

        case Command::INVALID:
            _logger->logDebug("Invalid command received (" + cmd->commandStr() + ")");
            break;
        }
    }
    _commands_mutex.unlock();
    if ( save_config )
        _saveConfig();
    return update_status;
}

bool RunnerThread::_checkFlameout()
{
    bool update_status = false;
    // Pellet temperature has changed?
    bool pellet_hot = _pellet->isHot();
    if ( _prev_pellet_hot != pellet_hot )
    {   // Pellet temperature changed...
        _logger->logEvent( pellet_hot ? LogItem::PELLET_HOT : LogItem::PELLET_COLD);
        _logger->logDebug(std::string("Pellet temp changed! now ") + (pellet_hot ? "HOT" : "COLD" ) );
        // Check for flameout:
        if ( !_pellet_flameout && // we are NOT in flameout condition...
             // Pellet is ON, was HOT, now it's cold
             (_pellet->isOn() && _prev_pellet_hot && !pellet_hot ) )
        {
            _pellet_flameout = true;
            update_status = true;
            _logger->logDebug("pellet flameout detected - pellet has gone cold");
            _logger->logEvent( LogItem::PELLET_FLAMEOUT_ON );
        }
        else if ( _pellet_flameout && // We are in flameout...
                  pellet_hot ) // Pellet is back HOT!
        {
            _pellet_flameout = false;
            update_status = true;
            _logger->logDebug("pellet flameout end");
            _logger->logEvent( LogItem::PELLET_FLAMEOUT_OFF );
        }
        _prev_pellet_hot = pellet_hot;
    }
    else // Pellet temperature still unchanged, check for missed start:
    {
        if ( _pellet->isOn() )
        {
            uint64_t pellet_on_since = _pellet->lastOnTime();
            if ( !_pellet_flameout && // not in flameout
                 !pellet_hot && // pellet is still COLD
                 ((pellet_on_since + _pellet_startup_delay) < _last_time) ) // Too long since started
            {
                _pellet_flameout = true;
                update_status = true;
                _logger->logDebug("pellet flameout detected - pellet was never hot after startup time");
                _logger->logEvent( LogItem::PELLET_FLAMEOUT_ON );
            }
        }
    }
    return update_status;
}

bool RunnerThread::_checkSpecialConditions()
{
    bool update_status = false;
    // Anti-ice is always the top priority:
    if ( _temp_sensor->getTemp() <= 5.0f )
    {
        if ( !_anti_ice_active )
        {
            _logger->logDebug("Anti-ice ON");
            _logger->logEvent( LogItem::ANTI_ICE_ON );
            update_status = true;
            _anti_ice_active = true;
        }
    }
    else if ( _anti_ice_active )
    {
        if ( _temp_sensor->getTemp() > 6.0f )
        {
            _logger->logDebug("Anti-ice OFF (gas spento)");
            _logger->logEvent( LogItem::ANTI_ICE_OFF );
            update_status = true;
            _anti_ice_active = false;
        }
    }
    else // If we are not freezing, we can check the rest.
    {
        // Now, we can check for over_temp:
        // If not already over_temp, and current temp if above max(with histeresys)
        if ( !_over_temp && (_temp_sensor->getTemp() > (_max_temp+0.1f)) )
        {   // Do not go over temp if everything is already off
            if ( _gas->isOn() || (_pellet->isOn() && !_pellet->isLow()) )
            {   // Over max, and we have something to turn off:
                _over_temp = true;
                _over_temp_start_time = FrameworkTimer::getTimeEpoc();
                // In manual model, we must instruct to resume the proper heating after we
                // detect the end of the over temp condition:
                if ( _manual_mode )
                {
                    _resume_gas_on = _gas->isOn();
                    _resume_pellet_mod = _pellet->isOn() && !_pellet->isLow();
                }
                _logger->logDebug("over temp start");
                _logger->logEvent( LogItem::OVER_TEMP_ON );
                update_status = true;
            }
        }   // We are already over temp, are we back under the max (with histeresys)?
        else if ( _over_temp && (_temp_sensor->getTemp() < _max_temp) )
        {   // Rientrati dall'over-temperatura:
            _over_temp = false;
            _over_temp_start_time = 0;
            _logger->logDebug("over temp end");
            _logger->logEvent( LogItem::OVER_TEMP_OFF );
            update_status = true;
        }

        // At last, we check for under temp:
        // We are not under_temp, but is current temp too low?
        if ( !_under_temp && (_temp_sensor->getTemp() < _min_temp) )
        {   // Do not go under temp if gas or pellet are already heating
            if ( !_gas->isOn() && !(_pellet->isOn() && !_pellet->isLow() && _pellet->isHot()) )
            {   // We are under temp, we need to turn on something
                _under_temp = true;
                _logger->logDebug("under min temp start");
                _logger->logEvent( LogItem::UNDER_TEMP_ON );
                update_status = true;
            }
        }   // We are already under temp. Have we recovered? (with histeresys)
        else if ( _under_temp && (_temp_sensor->getTemp() > (_min_temp+0.1f)) )
        {   // Siamo tornati "sopra" la temperatura minima:
            _under_temp = false;
            _logger->logDebug("under min temp end");
            _logger->logEvent( LogItem::UNDER_TEMP_OFF );
            update_status = true;
        }
    }
    return update_status;
}

bool RunnerThread::scheduledRun(uint64_t, uint64_t)
{
    bool update_status = _checkCommands();

    // Read internal temperature sensor and calculate temp trend
    if ( _temp_sensor->readSensor() )
    {
        _sensor_success_reads++;
        // Trend calculation make sense only when temperature actually differs
        if ( (_sensor_success_reads == 1) || // ensure the first read is always processed
                ( fabs(_temp_sensor->getTemp() - _temp_trend_prev) > 0.05f) )
        {
            float delta_time = _temp_sensor->getTimestamp() - _temp_trend_last_valid;
            if ( delta_time > 0.0f )
            {
                float d1_cur = (_temp_sensor->getTemp() - _temp_trend_prev) / delta_time;
                // T'' requires at least three valid temperatures
                if ( _sensor_success_reads >= 3 )
                {
                    float d2_cur = (d1_cur - _temp_trend_D1prev) / _temp_trend_last_valid;
                    _temp_trend_D2mean = (_temp_trend_D2mean + d2_cur) / 2.0f;
                    std::stringstream ss;
                    ss << "T'' Mean: " << _temp_trend_D2mean
                       << " - T': " << d1_cur
                       << " - T: " << _temp_sensor->getTemp()
                       << " - Delta time: " << delta_time;
                    _logger->logDebug( "Trend: " + ss.str() );
                }
                // This get calculated each time:
                _temp_trend_prev = _temp_sensor->getTemp();
                _temp_trend_last_valid = _temp_sensor->getTimestamp();
                // T' requires at least two valid temperatures to be meaningfull:
                if ( _sensor_success_reads >= 2 )
                {
                    _temp_trend_D1prev =  d1_cur;
                }
            }
        }
    }

    if ( _activated )
    {
        if ( _checkFlameout() )
            update_status = true;

        // If we have at least ONE successful read, check some special conditions:
        if ( _sensor_success_reads > 0 )
            if ( _checkSpecialConditions() )
                update_status = true;
    }

    // Update time and history
    uint64_t current_time = FrameworkTimer::getTimeEpoc();
    if ( (_last_time+60) <= current_time )
    {
        update_status = true;
        _logger->logDebug("Time interval (" + FrameworkUtils::utostring(current_time) +" - " + FrameworkUtils::utostring(_last_time) + " >= 60)");
        if ( _updateCurrentTime( current_time ) )
        {
            _gas->newDayResetTimes();
            _pellet->newDayResetTimes();
        }

        // We save the temperatures only every minute
        if ( _sensor_success_reads > 0 )
        {
            if ( !_history.update( _temp_sensor->getTemp(), _temp_sensor->getHimidity(),
                                   _current_ext_temp, _current_ext_humidity ) )
                _logger->logDebug("Unable to write to history file");
        }
    }

    if ( _activated )
    {
        // Now we have collected all the conditions, and we can proceed to apply them:
        bool gas_on = _manual_mode ? _gas->isOn() : _program.getGas();
        bool pellet_on = _manual_mode ? _pellet->isOn() : _program.getPellet();
        bool pellet_minimum = _manual_mode ? _pellet->isLow() : _program.getPelletMinimum();
        // We never resume gas or pellet mod when in program mode:
        if ( !_manual_mode )
            _resume_gas_on = _resume_pellet_mod = false;
        // Anti-ice relies always on gas:
        if ( _anti_ice_active )
        {
            gas_on = true; // Force GAS to ON;
            if ( pellet_on )
                pellet_minimum = false; // Turn pellet to HIGH
        }
        else
        {
            if ( _pellet_flameout )
            {   // Force gas if pellet is on high:
                if ( pellet_on && !pellet_minimum )
                    gas_on = true;
            }
            if ( _over_temp )
            {   // Turn off gas and set pellet to minimum
                gas_on = false;
                if ( pellet_on )
                {
                    if ( !pellet_minimum )
                        pellet_minimum = true;
/*                    else THIS IS COMMENTED OUT BECAUSE IT'S MORE COMPLEX THAN THIS
 *                         AND IT IS REALLY SUCH A CORNER CASE THAT'S NOT WORTH IT
                    {
                        // If we are in overtemp since too much and temperature is rising,
                        // just shutoff completely pellet...
                        if ( ((FrameworkTimer::getTimeEpoc() - _over_temp_start_time) > 3600) &&
                             (_temp_trend_D2mean > 0.0001f) )
                            pellet_on = false;
                    }*/
                }
            }
            else
            {   // Resume gas or pellet modulation.
                // This will happen only after an over temp has terminated AND we are not in program mode.
                if ( _resume_gas_on )
                {
                    gas_on = true;
                    _resume_gas_on = false;
                }
                if ( _resume_pellet_mod )
                {
                    pellet_minimum = false;
                    _resume_pellet_mod = false;
                }
            }
            if ( _under_temp )
            {   // Switch pellet to high or turn on gas
                if ( pellet_on && !_pellet_flameout )
                    pellet_minimum = false;
                else
                    gas_on = true;
            }
        }

        if ( gas_on && !_gas->isOn() )
        {
            if ( _gas->switchOn() )
                update_status = true;
        }
        else if ( !gas_on && _gas->isOn() )
        {
            if ( _gas->switchOff() )
                update_status = true;
        }

        if ( pellet_on )
        {
            if ( !_pellet->isOn() )
            {
                if ( _pellet->switchOn() )
                    update_status = true;
            }
            if ( pellet_minimum && !_pellet->isLow() )
            {
                if ( _pellet->setPower( Generator::POWER_LOW ) )
                    update_status = true;
            }
            else if ( !pellet_minimum && _pellet->isLow() )
            {
                if ( _pellet->setPower( Generator::POWER_HIGH ) )
                    update_status = true;
            }
        }
        else if ( !pellet_on && _pellet->isOn() )
        {
            if ( _pellet->switchOff() )
                update_status = true;
        }
    }
    else
    {
        if ( _pellet->isOn() )
            _pellet->switchOff();
        if ( _gas->isOn() )
            _gas->switchOff();
    }

    _logger->updateEventsJson();

    if ( update_status )
    {
        _pellet->printStatus();
        _gas->printStatus();
        _temp_sensor->printStatus();
        _updateStatus();
    }
    return true;
}

bool RunnerThread::_updateCurrentTime( uint64_t new_time )
{
    bool ret = false;
    _last_time = new_time;
    time_t now = static_cast<time_t>(_last_time);
    struct tm *tm_struct = localtime(&now);
    int32_t new_day = (6+tm_struct->tm_wday)%7;
    int32_t new_hour = tm_struct->tm_hour;
    int32_t new_half = (tm_struct->tm_min >= 30 ? 1 : 0);
    bool time_mod = false;
    if ( new_day != _day )
    {
        ret = true;
        _day = new_day;
        time_mod = true;
    }
    if ( _hour != new_hour )
    {
        _hour = new_hour;
        time_mod = true;
    }
    if ( _half_hour != new_half )
    {
        _half_hour = new_half;
        time_mod = true;
    }
    if ( time_mod )
        _program.setTime( _day, _hour, _half_hour );
    return ret;
}

void RunnerThread::_saveConfig()
{
    _config->setValueBool("activated", _activated );
    _config->setValue( "mode", _manual_mode ? "manual" : "auto" );
    _config->setValue( "min_temp", FrameworkUtils::ftostring( _min_temp ) );
    _config->setValue( "max_temp", FrameworkUtils::ftostring( _max_temp ) );
    _config->setValue( "temp_correction", FrameworkUtils::ftostring( _temp_correction ) );
    _config->setValue("history_mode", _history.getMode() );
    _config->setValue("history_len", FrameworkUtils::tostring( _history.getLen() ) );
    _config->setValueBool( "debug", _logger->getDebug() );
    _config->setValue( "pellet_startup_delay", FrameworkUtils::utostring(_pellet_startup_delay) );

    ConfigData* prog_section = _config->getSection( "program" );
    if ( prog_section == nullptr )
        prog_section = _config->newSection( "program" );
    _program.saveConfig( prog_section );
    _config->saveToFile();
}

void RunnerThread::_updateStatus()
{
    std::string str_ext_temp = FrameworkUtils::ftostring( _current_ext_temp );
    std::string str_ext_humidity = FrameworkUtils::ftostring( _current_ext_humidity );
    std::string str_temp = FrameworkUtils::ftostring( _temp_sensor->getTemp() );
    std::string str_humidity = FrameworkUtils::ftostring( _temp_sensor->getHimidity() );
    std::string str_min_t = FrameworkUtils::ftostring( _min_temp );
    std::string str_max_t = FrameworkUtils::ftostring( _max_temp );
    std::string str_day = FrameworkUtils::tostring( _day );
    std::string str_h = FrameworkUtils::tostring( _hour );
    std::string str_f = FrameworkUtils::tostring( _half_hour );
    std::string str_pellet_off_warning("La stufa a pellet sarà spenta!");
    std::string str_pellet_on_warning("La stufa a pellet sarà accesa!");

    FILE* status_file = fopen( (_exchange_path+"/_status").c_str(), "w" );
    if ( status_file )
    {
        for ( unsigned int i = 0; i < _status_json_template.size(); ++i )
        {
            fwrite( _status_json_template[i].c_str(),
                    _status_json_template[i].length(), 1, status_file);
            switch (i)
            {
            case 0:
                if ( _activated )
                {
                    if ( _manual_mode )
                        fwrite( "manual", 6, 1, status_file);
                    else
                        fwrite( "auto", 4, 1, status_file);
                }
                break;
            case 1:
                if ( _activated )
                    fwrite( "on", 2, 1, status_file);
                else
                    fwrite( "off", 3, 1, status_file);
                break;
            case 2:
                if ( _anti_ice_active )
                    fwrite( "on", 2, 1, status_file);
                else
                    fwrite( "off", 3, 1, status_file);
                break;
            case 3:
                if ( _pellet->isOn() && !_program.getPellet() )
                    fwrite( str_pellet_off_warning.c_str(), str_pellet_off_warning.size(), 1, status_file);
                else if ( !_pellet->isOn() && _program.getPellet() )
                    fwrite( str_pellet_on_warning.c_str(), str_pellet_on_warning.size(), 1, status_file);
                break;
            case 4:
                if ( _pellet->isOn() )
                    fwrite( "on", 2, 1, status_file);
                else
                    fwrite( "off", 3, 1, status_file);
                break;
            case 5:
                if ( _pellet->isHot() )
                    fwrite( "on", 2, 1, status_file);
                else
                    fwrite( "off", 3, 1, status_file);
                break;
            case 6:
                if ( _pellet->isLow() )
                    fwrite( "on", 2, 1, status_file);
                else
                    fwrite( "off", 3, 1, status_file);
                break;
            case 7:
                if ( _pellet_flameout )
                    fwrite( "on", 2, 1, status_file);
                else
                    fwrite( "off", 3, 1, status_file);
                break;
            case 8:
            {
                uint64_t total_time = _pellet->todayOnTime();
                std::string str = FrameworkUtils::utostring( total_time );
                fwrite( str.c_str(), str.length(), 1, status_file);
            }
                break;
            case 9:
            {
                uint64_t total_time = _pellet->todayLowOnTime();
                std::string str = FrameworkUtils::utostring( total_time );
                fwrite( str.c_str(), str.length(), 1, status_file);
            }
                break;                
            case 10:
            {
                uint64_t season_total_time = _pellet->seasonOnTime();
                std::string str = FrameworkUtils::utostring( season_total_time );
                fwrite( str.c_str(), str.length(), 1, status_file);
            }
                break;
            case 11:
            {
                uint64_t season_total_time = _pellet->seasonLowOnTime();
                std::string str = FrameworkUtils::utostring( season_total_time );
                fwrite( str.c_str(), str.length(), 1, status_file);
            }
                break;
            case 12:
                if ( _gas->isOn() )
                    fwrite( "on", 2, 1, status_file);
                else
                    fwrite( "off", 3, 1, status_file);
                break;
            case 13:
            {
                uint64_t total_time = _gas->todayOnTime();
                std::string str = FrameworkUtils::utostring( total_time );
                fwrite( str.c_str(), str.length(), 1, status_file);
            }
                break;
            case 14:
            {
                uint64_t seaon_total_time = _gas->seasonOnTime();
                std::string str = FrameworkUtils::utostring( seaon_total_time );
                fwrite( str.c_str(), str.length(), 1, status_file);
            }
                break;
            case 15:
                fwrite( str_min_t.c_str(), str_min_t.length(), 1, status_file);
                break;
            case 16:
                fwrite( str_max_t.c_str(), str_max_t.length(), 1, status_file);
                break;
            case 17:
                fwrite( str_temp.c_str(), str_temp.length(), 1, status_file);
                break;
            case 18:
                fwrite( str_ext_temp.c_str(), str_ext_temp.length(), 1, status_file);
                break;
            case 19:
                fwrite( str_humidity.c_str(), str_humidity.length(), 1, status_file);
                break;
            case 20:
                fwrite( str_ext_humidity.c_str(), str_ext_humidity.length(), 1, status_file);
                break;
            case 21:
                fwrite( str_day.c_str(), str_day.length(), 1, status_file);
                break;
            case 22:
                fwrite( str_h.c_str(), str_h.length(), 1, status_file);
                break;
            case 23:
                fwrite( str_f.c_str(), str_f.length(), 1, status_file);
                break;
            case 24:
                break;
            }
        }
        _program.writeJSON( status_file );

        fwrite( "}", 1, 1, status_file );
        fclose( status_file );
    }
}

