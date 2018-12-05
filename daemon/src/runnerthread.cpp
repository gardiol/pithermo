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
    _history( hst ),
    _config( config ),
    _exchange_path( exchange_path ),
    _commands_mutex("CommandsMutex"),
    _current_mode(MANUAL_MODE),
    _heating_activated(false),
    _anti_ice_active(false),
    _over_temp(false),
    _under_temp(false),
    _pellet_flameout(false),
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
    _pellet_startup_delay(60*45),
    _hysteresis(0.1f)
{
    _status_json_template.push_back("{\"mode\":\"");
    _status_json_template.push_back("\",\"active\":\"");
    _status_json_template.push_back("\",\"antiice\":\"");
    _status_json_template.push_back("\",\"warnings\":{\"modeswitch\":\"");
    _status_json_template.push_back("\"},\"pellet\":{\"command\":\"");
    _status_json_template.push_back("\",\"status\":\"");
    _status_json_template.push_back("\",\"minimum\":\"");
    _status_json_template.push_back("\",\"flameout\":\"");
    _status_json_template.push_back("\"},\"gas\":{\"command\":\"");
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
    std::string content;
    if ( !_config->isEmpty() )
    {
        _current_mode = FrameworkUtils::string_tolower(_config->getValue( "mode" )) == "manual" ? MANUAL_MODE : AUTO_MODE;
        _min_temp = static_cast<float>(FrameworkUtils::string_tof( _config->getValue( "min_temp" ) ) );
        _max_temp = static_cast<float>(FrameworkUtils::string_tof( _config->getValue( "max_temp" ) ) );
        _heating_activated = _config->getValueBool( "activated" );
        if ( _config->hasValue( "pellet_startup_delay" ) )
            _pellet_startup_delay = static_cast<uint64_t>(FrameworkUtils::string_toi( _config->getValue( "pellet_startup_delay" ) ) );
        _temp_correction = static_cast<float>(FrameworkUtils::string_tof( _config->getValue( "temp_correction" ) ) );
        _program.loadConfig( _config->getSection( "program" ) );
        if ( _config->hasValue( "hysteresis" ) )
            _hysteresis = FrameworkUtils::string_tof( _config->getValue( "hysteresis" ) );
    }
    else
        _saveConfig();

    // Load history log:
    _history.initialize( _current_ext_temp, _current_ext_humidity );

    // Ensure on times will not be zeroized later
    _updateCurrentTime( FrameworkTimer::getTimeEpoc() );

    // Initialize generators:
    _gas = new Generator( "gas", _logger, 0, -1, -1,
                          LogItem::GAS_ON, LogItem::GAS_OFF,
                          LogItem::NO_EVENT, LogItem::NO_EVENT,
                          LogItem::NO_EVENT, LogItem::NO_EVENT,0); // command is GPIO 0
    _pellet = new Generator( "pellet", _logger, 6, 7, 5,
                             LogItem::PELLET_ON, LogItem::PELLET_OFF,
                             LogItem::PELLET_MINIMUM, LogItem::PELLET_MODULATION,
                             LogItem::PELLET_FLAMEOUT_ON, LogItem::PELLET_FLAMEOUT_OFF, _pellet_startup_delay ); // command = 2, status = 7, min/mod=5
    _temp_sensor = new TempSensor( _logger, 1, _temp_correction ); // 1 temp sensor

    // Ensure we DO NOT print at next check
    _prev_pellet_hot = _pellet->isHot();

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
            if ( !_heating_activated )
            {
                _logger->logEvent( LogItem::ACTIVATED );
                save_config = true;
                _heating_activated = true;
                update_status = true;
            }
            break;

        case Command::DEACTIVATE:
            _logger->logDebug("Deactivate command received");
            if ( _heating_activated )
            {
                _logger->logEvent( LogItem::DEACTIVATED );
                save_config = true;
                _heating_activated = false;
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

        case Command::PELLET_MINIMUM_ON:
            _logger->logDebug("Pellet Minimum ON received");
            if ( (_current_mode == MANUAL_MODE) && _pellet->isOn() )
            {
                _pellet->setPower( Generator::POWER_LOW );
                update_status = true;
            }
            break;

        case Command::PELLET_MINIMUM_OFF:
            _logger->logDebug("Pellet Minimum OFF received");
            if ( (_current_mode == MANUAL_MODE) && _pellet->isOn() )
            {
                _pellet->setPower( Generator::POWER_HIGH );
                update_status = true;
            }
            break;

        case Command::PELLET_ON:
            _logger->logDebug("Pellet ON received");
            if ( _current_mode == MANUAL_MODE )
            {
                _pellet->switchOn();
                update_status = true;
            }
            break;

        case Command::PELLET_OFF:
            _logger->logDebug("Pellet OFF received");
            if ( _current_mode == MANUAL_MODE )
            {
                _pellet->switchOff();
                update_status = true;
            }
            break;

        case Command::GAS_ON:
            _logger->logDebug("Gas ON received");
            if ( _current_mode == MANUAL_MODE )
            {
                _gas->switchOn();
                update_status = true;
            }
            break;

        case Command::GAS_OFF:
            _logger->logDebug("Gas OFF received");
            if ( _current_mode == MANUAL_MODE )
            {
                _gas->switchOff();
                update_status = true;
            }
            break;

        case Command::MANUAL:
            if ( _current_mode != MANUAL_MODE )
            {
                _logger->logDebug("Manual mode");
                _current_mode = MANUAL_MODE;
                _logger->logEvent( LogItem::MANUAL_MODE );
                update_status = true;
                save_config = true;
            }
            break;

        case Command::AUTO:
            if ( _current_mode != AUTO_MODE )
            {
                _logger->logDebug("Auto mode");
                _current_mode = AUTO_MODE;
                _logger->logEvent( LogItem::AUTO_MODE );
                update_status = true;
                save_config = true;
            }
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

bool RunnerThread::scheduledRun(uint64_t, uint64_t)
{
    bool update_status = _checkCommands();

    // Read internal temperature sensor and calculate temp trend
    if ( _temp_sensor->readSensor() )
    {
        update_status = true;
        _sensor_success_reads++;
    }

    if ( _heating_activated )
    {   // Don't do anything until we have a valid temperature read
        if (_sensor_success_reads > 0)
        {
            // First of all, check anti-ice:
            if ( _checkAntiIce() )
                update_status = true;

            if ( _checkFlameout() )
                update_status = true;

            // By default, all is off here:
            bool gas_on = false;
            bool pellet_on = false;
            bool pellet_minimum_on = false;

            if ( !_anti_ice_active )
            {
                // select the current target temperature:
                if ( _current_mode == AUTO_MODE )
                {
                    // Target temp is "MAX" or "MIN" depending on program:
                    float target_temperature = ( _program.useHigh() ? _max_temp : _min_temp );

                    // If pellet should be on, we will turn it on no matter what:
                    // (this is to prevent the pellet to turn off)
                    pellet_on = _program.usePellet();

                    if ( _checkTargetTemperature( _temp_sensor->getTemp(),
                                                  target_temperature,
                                                  _over_temp,
                                                  _under_temp) )
                        update_status = true;

                    if ( _over_temp )
                    {   // remember that by default, all is off... so is gas.
                        if ( pellet_on )
                            pellet_minimum_on = true;
                    }
                    if ( _under_temp )
                    {
                        // Gas goes ON only in GAS program modes OR if a flameout is detected...
                        gas_on = _program.useGas() || _pellet_flameout;
                        // remember that by default, all is off... so is pellet minimum.
                    }
                }
                else
                {// In manual mode, control is fully manual.
                    pellet_on = _pellet->isOn();
                    pellet_minimum_on = _pellet->isLow();
                    // The exception is GAS, which goes ON if a pellet flamout is detected.
                    // (note that if pellet is OFF, flameout is always false)
                    gas_on = _gas->isOn() || (!pellet_minimum_on && _pellet_flameout);
                    _under_temp = false;
                    _over_temp = false;
                }
            }
            else // anti-ice active, turn everything on!
            {
                gas_on = true;
                pellet_minimum_on = false;
                pellet_on = _pellet->isOn();
            }

            // Now all conditions are set...
            // Check some more "general" conditions...
            // Turn GAS off always if pellet is HOT:
            if ( _pellet->isHot() )
                gas_on = false;

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
                    if ( _pellet->switchOn() )
                        update_status = true;
                if ( pellet_minimum_on && !_pellet->isLow() )
                {
                    if ( _pellet->setPower( Generator::POWER_LOW ) )
                        update_status = true;
                }
                else if ( !pellet_minimum_on && _pellet->isLow() )
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
        } // valid temperature reads
    }
    else // Heating is deactivated
    {
        if ( _pellet->isOn() )
            _pellet->switchOff();
        if ( _gas->isOn() )
            _gas->switchOff();
    }

    // Update time and history
    uint64_t current_time = FrameworkTimer::getTimeEpoc();
    if ( (_last_time+60) <= current_time )
    {
        _logger->logDebug("Time interval (" + FrameworkUtils::utostring(current_time) +" - " + FrameworkUtils::utostring(_last_time) + " >= 60)");
        _updateCurrentTime( current_time );

        // We debug the temperatures only every minute
        if ( _sensor_success_reads > 0 )
        {
            if ( !_history.update( _temp_sensor->getTemp(), _temp_sensor->getHumidity(),
                                   _current_ext_temp, _current_ext_humidity ) )
                _logger->logDebug("Unable to write to history file");
        }
    }

    if ( update_status )
    {
        _pellet->printStatus();
        _gas->printStatus();
        _temp_sensor->printStatus();
        _updateStatus();
    }
    return true;
}

bool RunnerThread::_checkAntiIce()
{
    bool update_status = false;
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
        if ( _temp_sensor->getTemp() > 8.0f )
        {
            _logger->logDebug("Anti-ice OFF (gas spento)");
            _logger->logEvent( LogItem::ANTI_ICE_OFF );
            update_status = true;
            _anti_ice_active = false;
        }
    }
    return update_status;
}

bool RunnerThread::_checkFlameout()
{
    bool update_status = false;
    bool current_missed_start = _pellet->checkHotTimeout(_last_time);
    // Pellet temperature has changed?
    bool pellet_hot = _pellet->isHot();
    if ( _prev_pellet_hot != pellet_hot )
    {   // Pellet temperature changed...
        _logger->logEvent( pellet_hot ? LogItem::PELLET_HOT : LogItem::PELLET_COLD);
        _logger->logDebug(std::string("Pellet temp changed! now ") + (pellet_hot ? "HOT" : "COLD" ) );
        _prev_pellet_hot = pellet_hot;
    }
    if ( current_missed_start != _pellet_flameout )
        _pellet_flameout = current_missed_start;
    return update_status;
}

bool RunnerThread::_checkTargetTemperature(float sensor_temp,
                                           float target_temperature,
                                           bool& prev_over_temp,
                                           bool& prev_under_temp)
{
    bool update_status = false;
    if ( prev_under_temp )
        target_temperature += _hysteresis;
    bool new_over = sensor_temp > target_temperature;
    bool new_under = sensor_temp < target_temperature;

    if ( new_over != prev_over_temp )
    {
        prev_over_temp = new_over;
        update_status = true;
        if ( prev_over_temp )
        {
            _logger->logDebug("over target temp start");
            _logger->logEvent( LogItem::OVER_TEMP_ON );
        }
        else
        {
            _logger->logDebug("over target temp end");
            _logger->logEvent( LogItem::OVER_TEMP_OFF );
        }
    }
    if ( new_under != prev_under_temp )
    {
        prev_under_temp = new_under;
        update_status = true;
        if ( new_under )
        {
            _logger->logDebug("Under target temp start");
            _logger->logEvent( LogItem::UNDER_TEMP_ON );
        }
        else
        {
            _logger->logDebug("over target temp end");
            _logger->logEvent( LogItem::UNDER_TEMP_OFF );
        }
    }
    return update_status;
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
    _config->setValueBool("activated", _heating_activated );
    _config->setValue( "mode", _current_mode == MANUAL_MODE ? "manual" : "auto" );
    _config->setValue( "min_temp", FrameworkUtils::ftostring( _min_temp ) );
    _config->setValue( "max_temp", FrameworkUtils::ftostring( _max_temp ) );
    _config->setValue( "temp_correction", FrameworkUtils::ftostring( _temp_correction ) );
    _config->setValue( "pellet_startup_delay", FrameworkUtils::utostring(_pellet_startup_delay) );
    _config->setValue( "hysteresis", FrameworkUtils::ftostring( _hysteresis ) );

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
    std::string str_humidity = FrameworkUtils::ftostring( _temp_sensor->getHumidity() );
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
                if ( _heating_activated )
                {
                    if ( _current_mode == MANUAL_MODE )
                        fwrite( "manual", 6, 1, status_file);
                    else
                        fwrite( "auto", 4, 1, status_file);
                }
                break;
            case 1:
                if ( _heating_activated )
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
/*                if ( _pellet->isOn() && !_program.getPellet() )
                    fwrite( str_pellet_off_warning.c_str(), str_pellet_off_warning.size(), 1, status_file);
                else if ( !_pellet->isOn() && _program.getPellet() )
                    fwrite( str_pellet_on_warning.c_str(), str_pellet_on_warning.size(), 1, status_file);
     */           break;
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
                if ( _gas->isOn() )
                    fwrite( "on", 2, 1, status_file);
                else
                    fwrite( "off", 3, 1, status_file);
                break;
            case 9:
                fwrite( str_min_t.c_str(), str_min_t.length(), 1, status_file);
                break;
            case 10:
                fwrite( str_max_t.c_str(), str_max_t.length(), 1, status_file);
                break;
            case 11:
                fwrite( str_temp.c_str(), str_temp.length(), 1, status_file);
                break;
            case 12:
                fwrite( str_ext_temp.c_str(), str_ext_temp.length(), 1, status_file);
                break;
            case 13:
                fwrite( str_humidity.c_str(), str_humidity.length(), 1, status_file);
                break;
            case 14:
                fwrite( str_ext_humidity.c_str(), str_ext_humidity.length(), 1, status_file);
                break;
            case 15:
                fwrite( str_day.c_str(), str_day.length(), 1, status_file);
                break;
            case 16:
                fwrite( str_h.c_str(), str_h.length(), 1, status_file);
                break;
            case 17:
                fwrite( str_f.c_str(), str_f.length(), 1, status_file);
                break;
            case 18:
                break;
            }
        }
        _program.writeJSON( status_file );

        fwrite( "}", 1, 1, status_file );
        fclose( status_file );
    }
}

