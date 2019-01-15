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
#include "sharedstatus.h"

// GPIO
// 5 = minimo
// 4 = ???
// 6 = pellet
// 0 = gas
// 2 = quarto relé libero
// 7 = pellet feedback
RunnerThread::RunnerThread(ConfigFile *config,
                           const std::string &hst, Logger *l):
    ScheduledThread("Runner", 1000 * 1000 ),
    _shared_status("SharedStatus", SharedStatusKey, sizeof( SharedStatus ), SharedMemory::read_write_create_shm ),
    _logger(l),
    _gas(nullptr),
    _pellet(nullptr),
    _temp_sensor(nullptr),
    _history( hst ),
    _config( config ),
    _commands_mutex("CommandsMutex"),
    _current_mode(MANUAL_MODE),
    _heating_activated(false),
    _smart_temp_on(false),
    _anti_ice_active(false),
    _over_temp(false),
    _under_temp(false),
    _pellet_flameout(false),
    _prev_pellet_hot(false),
    _min_temp(16.0),
    _max_temp(17.0),
    _smart_temp(17.0),
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
    if ( _shared_status.isReady() )
    {
        memset( _shared_status.getWritePtr(), 0, _shared_status.getSharedSize() );
    }
    else
        _logger->logDebug("Unable to open shared memory for status!");

    // Load config file
    std::string content;
    if ( !_config->isEmpty() )
    {
        _current_mode = FrameworkUtils::string_tolower(_config->getValue( "mode" )) == "manual" ? MANUAL_MODE : AUTO_MODE;
        _min_temp = static_cast<float>(FrameworkUtils::string_tof( _config->getValue( "min_temp" ) ) );
        _max_temp = static_cast<float>(FrameworkUtils::string_tof( _config->getValue( "max_temp" ) ) );
        _heating_activated = _config->getValueBool( "activated" );
        _smart_temp_on = _config->getValueBool( "smart_temp" );
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

    if ( _shared_status.isReady() )
    {
        _updateStatus();
        startThread();
        while ( !isRunning() )
            FrameworkTimer::msleep_s( 100 );
    }
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

        case Command::SMART_TEMP_ON:
            _logger->logDebug("Smart temp on command received");
            if ( !_smart_temp_on )
            {
                _logger->logEvent( LogItem::SMART_TEMP_ON );
                save_config = true;
                _smart_temp_on = true;
                update_status = true;
            }
            break;

        case Command::SMART_TEMP_OFF:
            _logger->logDebug("Smart temp off command received");
            if ( _smart_temp_on )
            {
                _logger->logEvent( LogItem::SMART_TEMP_OFF );
                save_config = true;
                _smart_temp_on = false;
                update_status = true;
            }
            break;

        case Command::TEMPLATE_SET:
        {
            std::vector<std::string> tokens = FrameworkUtils::string_split( cmd->getParam(), ":" );
            if ( tokens.size() == 3 )
            {
                uint32_t template_no = static_cast<uint32_t>(FrameworkUtils::string_toi( tokens[0] ));
                _program.changeTemplate( template_no, tokens[1], tokens[2] );
                save_config = true;
                update_status = true;
            }
            break;
        }
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

        case Command::SET_HISTERESYS:
        {
            _logger->logDebug("New HYSTERESIS received: " + cmd->getParam());
            float tmp_hyst = FrameworkUtils::string_tof( cmd->getParam() );
            if ( tmp_hyst >= 0.1f )
            {
                _logger->logDebug("Changed hysteresis to " + cmd->getParam() );
                _logger->logEvent( LogItem::HYST_UPDATE );
                _hysteresis = tmp_hyst;
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
                _updateSmartTemp();

                // select the current target temperature:
                if ( _current_mode == AUTO_MODE )
                {
                    // Target temp is "MAX" or "MIN" depending on program:
                    float target_temperature = _program.useHigh() ? (_smart_temp_on ? _smart_temp : _max_temp) : _min_temp;

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

void RunnerThread::_updateSmartTemp()
{
    _smart_temp = _max_temp;
}

void RunnerThread::_saveConfig()
{
    _config->setValueBool("activated", _heating_activated );
    _config->setValueBool("smart_temp", _smart_temp_on );
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
    SharedStatus status;
    status.marker = SharedStatusMarker;
    status.last_update_stamp = FrameworkTimer::getCurrentTime();
    status.active = _heating_activated;
    status.anti_ice_active = _anti_ice_active;
    status.manual_mode = _current_mode == MANUAL_MODE;
    status.pellet_on = _pellet->isOn();
    status.pellet_minimum = _pellet->isLow();
    status.pellet_hot = _pellet->isHot();
    status.pellet_flameout = _pellet_flameout;
    status.gas_on = _gas->isOn();
    status.smart_temp_on = _smart_temp_on;
    status.smart_temp = _smart_temp;
    status.max_temp = _max_temp;
    status.min_temp = _min_temp;
    status.hysteresis = _hysteresis;
    status.temp_int = _temp_sensor->getTemp();
    status.humidity_int = _temp_sensor->getHumidity();
    status.temp_ext = _current_ext_temp;
    status.humidity_ext = _current_ext_humidity;
    status.day = _day;
    status.hour = _hour;
    status.half = _half_hour;
    _program.writeRaw( &status );
    memcpy( _shared_status.getWritePtr(), &status, _shared_status.getSharedSize() );
}

