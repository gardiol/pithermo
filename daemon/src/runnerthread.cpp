#include "runnerthread.h"

#include <string.h>
#include <time.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "configfile.h"
#include "debugprint.h"
#include "profiler.h"

#include "command.h"
#include "program.h"

// GPIO
// 5 = minimo
// 4 = ???
// 6 = quarto relé libero
// 0 = gas
// 2 = pellet
// 7 = pellet feedback
RunnerThread::RunnerThread(const std::string &cfg,
                           const std::string &exchange_path,
                           const std::string &hst, Logger *l):
    ScheduledThread("Runner", 10 * 1000 ),
    _logger(l),
    _gas(NULL),
    _pellet(NULL),
    _temp_sensor(NULL),
    _history( hst, exchange_path ),
    _config_file( cfg ),
    _exchange_path( exchange_path ),
    _commands_mutex("CommandsMutex"),
    _manual_mode(true),
    _manual_gas_on(false),
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
    _pellet_startup_delay(60*45)
{
    _status_json_template.push_back("{\"mode\":\"");
    _status_json_template.push_back("\",\"antiice\":\"");
    _status_json_template.push_back("\",\"warnings\":{\"modeswitch\":\"");
    _status_json_template.push_back("\"},\"pellet\":{\"command\":\"");
    _status_json_template.push_back("\",\"status\":\"");
    _status_json_template.push_back("\",\"minimum\":\"");
    _status_json_template.push_back("\",\"flameout\":\"");
    _status_json_template.push_back("\",\"time\":\"");
    _status_json_template.push_back("\",\"mintime\":\"");
    _status_json_template.push_back("\"},\"gas\":{\"command\":\"");
    _status_json_template.push_back("\",\"time\":\"");
    _status_json_template.push_back("\",\"status\":\"off\"},\"temp\":{\"min\":");
    _status_json_template.push_back(",\"max\":");
    _status_json_template.push_back(",\"int\":");
    _status_json_template.push_back(",\"ext\":");
    _status_json_template.push_back(",\"hum\":");
    _status_json_template.push_back("},\"now\":{\"d\":");
    _status_json_template.push_back(",\"h\":");
    _status_json_template.push_back(",\"f\":");
    _status_json_template.push_back("},\"program\":");

    // Load config file
    uint32_t history_len = 1;
    std::string history_mode = "day";
    std::string content;
    FrameworkUtils::file_to_str( _config_file, content );
    ConfigFile config("config", content );
    if ( !config.isEmpty() )
    {
        _logger->enableDebug( config.getValueBool("debug") );
        _manual_mode = FrameworkUtils::string_tolower(config.getValue( "mode" )) == "manual";
        _min_temp = FrameworkUtils::string_tof( config.getValue( "min_temp" ) );
        _max_temp = FrameworkUtils::string_tof( config.getValue( "max_temp" ) );
        if ( config.hasValue( "pellet_startup_delay" ) )
            _pellet_startup_delay = FrameworkUtils::string_toi( config.getValue( "pellet_startup_delay" ) );
        _temp_correction = FrameworkUtils::string_tof( config.getValue( "temp_correction" ) );
        history_len = FrameworkUtils::string_toi( config.getValue("history_len") );
        history_mode = FrameworkUtils::string_tolower( config.getValue( "history_mode" ) );
        _program.loadConfig( config.getSection( "program" ) );
    }
    else
        _saveConfig();

    // Load history log:
    _history.initialize( history_mode, history_len );

    uint64_t gas_on_time = 0;
    uint64_t pellet_on_time = 0;
    uint64_t pellet_min_time = 0;
    uint64_t gas_on_since = 0;
    uint64_t pellet_on_since = 0;
    uint64_t pellet_low_on_since = 0;
    _logger->calculateTodayTimes( gas_on_time, pellet_on_time, pellet_min_time,
                                  gas_on_since,pellet_on_since,pellet_low_on_since );

    // Initialize generators:
    _gas = new Generator( "gas", _logger, 0, -1, -1,
                          gas_on_time, 0,
                          gas_on_since, 0,
                          LogItem::GAS_ON, LogItem::GAS_OFF,
                          LogItem::NO_EVENT, LogItem::NO_EVENT ); // command is GPIO 0
    _pellet = new Generator( "pellet", _logger, 2, 7, 5,
                             pellet_on_time, pellet_min_time,
                             pellet_on_since, pellet_low_on_since,
                             LogItem::PELLET_ON, LogItem::PELLET_OFF,
                             LogItem::PELLET_MINIMUM, LogItem::PELLET_MODULATION ); // command = 2, status = 7, min/mod=5
    _temp_sensor = new TempSensor( _logger, 1, _temp_correction ); // 1 temp sensor

    // Ensure we print at next check
    _prev_pellet_hot = !_pellet->isHot();

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
    if ( _temp_sensor != NULL )
    {
        delete _temp_sensor;
        _temp_sensor = NULL;
    }
    if ( _gas != NULL )
    {
        delete _gas;
        _gas = NULL;
    }
    if ( _pellet != NULL )
    {
        delete _pellet;
        _pellet = NULL;
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
        case Command::EXT_TEMP:
            _logger->logDebug("New EXT TEMP received: " + cmd->getParam());
            _current_ext_temp = FrameworkUtils::string_tof( cmd->getParam() );
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
            if ( (tmp_temp != _min_temp) && (tmp_temp < _max_temp) )
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
            if ( (tmp_temp != _max_temp) && (tmp_temp > _min_temp) )
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
        default:
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
    if ( _temp_sensor->getTemp() <= 5.0 )
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
        if ( _temp_sensor->getTemp() > 6.0 )
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
        if ( !_over_temp && (_temp_sensor->getTemp() > (_max_temp+0.1)) )
        {   // Do not go over temp if everything is already off
            if ( _gas->isOn() || (_pellet->isOn() && !_pellet->isLow()) )
            {   // Over max, and we have something to turn off:
                _over_temp = true;
                _logger->logDebug("over temp start");
                _logger->logEvent( LogItem::OVER_TEMP_ON );
                update_status = true;
            }
        }   // We are already over temp, are we back under the max (with histeresys)?
        else if ( _over_temp && (_temp_sensor->getTemp() < _max_temp) )
        {   // Rientrati dall'over-temperatura:
            _over_temp = false;
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
        else if ( _under_temp && (_temp_sensor->getTemp() > (_min_temp+0.1)) )
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

    // Read internal temperature sensor:
    if ( _temp_sensor->readSensor() )
        _sensor_success_reads++;

    if ( _checkFlameout() )
        update_status = true;

    // If we have at least ONE successful read, check some special conditions:
    if ( _sensor_success_reads > 0 )
        if ( _checkSpecialConditions() )
            update_status = true;

    // Update time and history
    uint64_t current_time = FrameworkTimer::getTimeEpoc();
    if ( (_last_time+60) <= current_time )
    {
        update_status = true;
        _logger->logDebug("Time interval (" + FrameworkUtils::tostring(current_time) +" - " + FrameworkUtils::tostring(_last_time) + " >= 60)");
        _updateCurrentTime( current_time );
        // We save the temperatures only every minute
        if ( _sensor_success_reads > 0 )
        {
            if ( !_history.update( _temp_sensor->getTemp(), _temp_sensor->getHimidity(), _current_ext_temp ) )
                _logger->logDebug("Unable to write to history file");
        }
    }

    // Now we have collected all the conditions, and we can proceed to apply them:
    bool gas_on = _manual_mode ? _gas->isOn() : _program.getGas();
    bool pellet_on = _manual_mode ? _pellet->isOn() : _program.getPellet();
    bool pellet_minimum = _manual_mode ? _pellet->isLow() : _program.getPelletMinimum();
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
            if ( pellet_on && !pellet_minimum )
                pellet_minimum = true;
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

void RunnerThread::_updateCurrentTime( uint64_t new_time )
{
    _last_time = new_time;
    time_t now = (time_t)_last_time;
    struct tm *tm_struct = localtime(&now);
    int32_t new_day = (6+tm_struct->tm_wday)%7;
    int32_t new_hour = tm_struct->tm_hour;
    int32_t new_half = (tm_struct->tm_min >= 30 ? 1 : 0);
    bool time_mod = false;
    if ( new_day != _day )
    {
        if ( _day != 0 )
        {   // do not reset on first run or we will zeroize precalulated on times
            _gas->resetTimes();
            _pellet->resetTimes();
        }
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
}

void RunnerThread::_saveConfig()
{
    std::string content;
    FrameworkUtils::file_to_str( _config_file, content );
    ConfigFile config("config", content );
    config.setValue( "mode", _manual_mode ? "manual" : "auto" );
    config.setValue( "min_temp", FrameworkUtils::ftostring( _min_temp ) );
    config.setValue( "max_temp", FrameworkUtils::ftostring( _max_temp ) );
    config.setValue( "temp_correction", FrameworkUtils::ftostring( _temp_correction ) );
    config.setValue("history_mode", _history.getMode() );
    config.setValue("history_len", FrameworkUtils::tostring( _history.getLen() ) );
    config.setValueBool( "debug", _logger->getDebug() );
    config.setValue( "pellet_startup_delay", FrameworkUtils::tostring(_pellet_startup_delay) );

    ConfigData* prog_section = config.getSection( "program" );
    if ( prog_section == NULL )
        prog_section = config.newSection( "program" );
    _program.saveConfig( prog_section );
    if ( !FrameworkUtils::str_to_file( _config_file, config.toStr() ) )
        debugPrintError() << "Unable to save file " << _config_file << "\n";
}

void RunnerThread::_updateStatus()
{
    std::string str_ext_temp = FrameworkUtils::ftostring( _current_ext_temp );
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
                if ( _manual_mode )
                    fwrite( "manual", 6, 1, status_file);
                else
                    fwrite( "auto", 4, 1, status_file);
                break;
            case 1:
                if ( _anti_ice_active )
                    fwrite( "on", 2, 1, status_file);
                else
                    fwrite( "off", 3, 1, status_file);
                break;
            case 2:
                if ( _pellet->isOn() && !_program.getPellet() )
                    fwrite( str_pellet_off_warning.c_str(), str_pellet_off_warning.size(), 1, status_file);
                else if ( !_pellet->isOn() && _program.getPellet() )
                    fwrite( str_pellet_on_warning.c_str(), str_pellet_on_warning.size(), 1, status_file);
                break;
            case 3:
                if ( _pellet->isOn() )
                    fwrite( "on", 2, 1, status_file);
                else
                    fwrite( "off", 3, 1, status_file);
                break;
            case 4:
                if ( _pellet->isHot() )
                    fwrite( "on", 2, 1, status_file);
                else
                    fwrite( "off", 3, 1, status_file);
                break;
            case 5:
                if ( _pellet->isLow() )
                    fwrite( "on", 2, 1, status_file);
                else
                    fwrite( "off", 3, 1, status_file);
                break;
            case 6:
                if ( _pellet_flameout )
                    fwrite( "on", 2, 1, status_file);
                else
                    fwrite( "off", 3, 1, status_file);
                break;
            case 7:
            {
                uint32_t total_time = _pellet->todayOnTime();
                std::string str = FrameworkUtils::tostring( total_time );
                fwrite( str.c_str(), str.length(), 1, status_file);
            }
                break;
            case 8:
            {
                uint32_t total_time = _pellet->todayLowOnTime();
                std::string str = FrameworkUtils::tostring( total_time );
                fwrite( str.c_str(), str.length(), 1, status_file);
            }
                break;
            case 9:
                if ( _gas->isOn() )
                    fwrite( "on", 2, 1, status_file);
                else
                    fwrite( "off", 3, 1, status_file);
                break;
            case 10:
            {
                uint32_t total_time = _gas->todayOnTime();
                std::string str = FrameworkUtils::tostring( total_time );
                fwrite( str.c_str(), str.length(), 1, status_file);
            }
                break;
            case 11:
                fwrite( str_min_t.c_str(), str_min_t.length(), 1, status_file);
                break;
            case 12:
                fwrite( str_max_t.c_str(), str_max_t.length(), 1, status_file);
                break;
            case 13:
                fwrite( str_temp.c_str(), str_temp.length(), 1, status_file);
                break;
            case 14:
                fwrite( str_ext_temp.c_str(), str_ext_temp.length(), 1, status_file);
                break;
            case 15:
                fwrite( str_humidity.c_str(), str_humidity.length(), 1, status_file);
                break;
            case 16:
                fwrite( str_day.c_str(), str_day.length(), 1, status_file);
                break;
            case 17:
                fwrite( str_h.c_str(), str_h.length(), 1, status_file);
                break;
            case 18:
                fwrite( str_f.c_str(), str_f.length(), 1, status_file);
                break;
            case 19:
                break;
            }
        }
        _program.writeJSON( status_file );

        fwrite( "}", 1, 1, status_file );
        fclose( status_file );
    }
}

