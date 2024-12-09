#include "runnerthread.h"

#include <string.h>
#include <time.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <sstream>

#include "configfile.h"

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
    _excessive_overtemp(false),
    _gas_status_at_overtemp(false),
    _pellet_minimum_status_at_overtemp(false),
    _prev_pellet_hot(false),
    _min_temp(16.0),
    _max_temp(17.0),
    _smart_temp(17.0),
    _temp_correction(1.0),
    _excessive_overtemp_threshold(5.0),
    _external_request(false),
    _external_request_topic(),
    _sensor_success_reads(0),
    _manual_off_time(0),
    _last_time(0),
    _day(0),
    _hour(0),
    _half_hour(0),
    _current_ext_temp(0.0),
    _current_ext_humidity(0.0),
    _pellet_startup_delay(60*60),
    _hysteresis_max(0.1f),
    _hysteresis_min(0.1f),
    _manual_pellet_minimum_forced_off(false),
    _manual_gas_forced_on(false),
    _mqtt( NULL ),
    _debug_updates(false)

{
    if ( _shared_status.isReady() )
    {
        memset( _shared_status.getWritePtr(), 0, _shared_status.getSharedSize() );
    }
    else
        _logger->logDebug("Unable to open shared memory for status!");

    // Load config file
    std::string content;
    std::string mqtt_host = "";
    std::string mqtt_username = "";
    std::string mqtt_password = "";
    if ( !_config->isEmpty() )
    {
        std::string config_mode = FrameworkUtils::string_tolower(_config->getValue( "mode" ));
        if ( config_mode == "manual" )
            _current_mode = MANUAL_MODE;
        else if ( config_mode == "external" )
            _current_mode = EXTERNAL_MODE;
        else
            _current_mode = AUTO_MODE;

        _min_temp = static_cast<float>(FrameworkUtils::string_tof( _config->getValue( "min_temp" ) ) );
        _max_temp = static_cast<float>(FrameworkUtils::string_tof( _config->getValue( "max_temp" ) ) );
        _heating_activated = _config->getValueBool( "activated" );
        _smart_temp_on = _config->getValueBool( "smart_temp" );
        if ( _config->hasValue( "pellet_startup_delay" ) )
            _pellet_startup_delay = static_cast<uint64_t>(FrameworkUtils::string_toi( _config->getValue( "pellet_startup_delay" ) ) );
        _temp_correction = static_cast<float>(FrameworkUtils::string_tof( _config->getValue( "temp_correction" ) ) );
        _program.loadConfig( _config->getSection( "program" ) );
        if ( _config->hasValue( "hysteresis_max" )  )
            _hysteresis_max = FrameworkUtils::string_tof( _config->getValue( "hysteresis_max" ) );
        if ( _config->hasValue( "hysteresis_min" )  )
            _hysteresis_min = FrameworkUtils::string_tof( _config->getValue( "hysteresis_min" ) );
        if ( _config->hasValue( "excessive_overtemp_ts") )
            _excessive_overtemp_threshold = FrameworkUtils::string_tof( _config->getValue( "excessive_overtemp_ts" ) );
        if ( _excessive_overtemp_threshold < 1.0f )
            _excessive_overtemp_threshold = 1.0f;
        if ( _excessive_overtemp_threshold < (_hysteresis_max+1.0f) )
            _excessive_overtemp_threshold = _hysteresis_max+1.0f;
        if ( _config->hasValue( "mqtt_host" ) )
        {
            mqtt_host = _config->getValue( "mqtt_host" );
            mqtt_username = _config->getValue( "mqtt_username" );
            mqtt_password = _config->getValue( "mqtt_password" );
        }
        _external_request_topic = _config->getValue( "external_request_topic" );
        _debug_updates = _config->getValueBool( "debug_updates" );
    }
    else
        _saveConfig();

    if ( mqtt_host != "" )
    {
        _mqtt = new MQTT_Interface( _logger, this, mqtt_host, mqtt_username, mqtt_password );
        if ( (_current_mode == EXTERNAL_MODE) &&
            !_external_request_topic.empty() )
            _mqtt->subscribe( _external_request_topic );
    }

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

    if ( _mqtt != NULL )
        delete _mqtt;

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
    if ( _debug_updates )
        _logger->logDebug("Appending command: " + cmd->commandStr() );
    _commands_mutex.lock();
    _commands_list.push_back( cmd );
    _commands_mutex.unlock();
}

void RunnerThread::message_received(const std::string &topic, const std::string &payload)
{
    _logger->logDebug( std::string("MQTT Topic: '") + topic + std::string("': '") + payload + std::string("'") );
    if ( topic == _external_request_topic )
    {
        _external_request = payload == "True";
        _logger->logDebug( std::string("External request: '") + payload + std::string("'"));
    }
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
	    if ( _debug_updates )
                _logger->logDebug("New EXT TEMP received: " + cmd->getParam());
            {
                std::vector<std::string> tokens = FrameworkUtils::string_split( cmd->getParam(), ":" );
                if ( tokens.size() == 3 )
                {
                    //                std::string sensor_id = tokens[0];
                    _current_ext_temp = FrameworkUtils::string_tof( tokens[1] );
                    _current_ext_humidity = FrameworkUtils::string_tof( tokens[2] );

                    if ( _mqtt != NULL )
                    {
                        std::string topic = "esterno/nord/temp";
                        std::string data = "{ \"temperature\": ";
                        data += tokens[1];
                        data += ", \"humidity\": ";
                        data += tokens[2] ;
                        data += " }";
                        _mqtt->publish( topic, data );
                    }
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
                _pellet->switchOn(false);
                update_status = true;
            }
            break;

        case Command::PELLET_OFF:
            _logger->logDebug("Pellet OFF received");
            if ( _current_mode == MANUAL_MODE )
            {
                _pellet->switchOff(false);
                update_status = true;
            }
            break;

        case Command::GAS_ON:
            _logger->logDebug("Gas ON received");
            if ( _current_mode == MANUAL_MODE )
            {
                _gas->switchOn(false);
                update_status = true;
            }
            break;

        case Command::GAS_OFF:
            _logger->logDebug("Gas OFF received");
            if ( _current_mode == MANUAL_MODE )
            {
                _gas->switchOff(false);
                update_status = true;
            }
            break;

        case Command::SET_MANUAL_OFF_TIME:
            _logger->logDebug("Manual off time received");
            if ( _current_mode == MANUAL_MODE )
            {
                uint64_t tmp_dmot = FrameworkUtils::string_tou( cmd->getParam() );
                if ( tmp_dmot > 0 )
                {
                    _logger->logEvent( LogItem::MANUAL_OFF_TIME_UPDATE );
                    _manual_off_time = _last_time+tmp_dmot;
                    update_status = true;
                }
                else if ( tmp_dmot == 0 )
                {
                    _logger->logEvent( LogItem::MANUAL_OFF_TIME_UPDATE );
                    _manual_off_time = 0;
                    update_status = true;
                }
            }
            break;

        case Command::SET_EXCESSIVE_OVERTEMP_TS:
        {
            _logger->logDebug("New excessive overtime threshold received");
            float tmp_ots = FrameworkUtils::string_tof( cmd->getParam() );
            if ( tmp_ots >= (_hysteresis_max+1.0f) && tmp_ots >= 1.0f )
            {
                if ( tmp_ots != _excessive_overtemp )
                {
                    _logger->logEvent( LogItem::EXCESSIVE_OVERTEMP_UPD );
                    _excessive_overtemp_threshold = tmp_ots;
                    update_status = true;
                    save_config = true;
                }
            }
        }
        break;

        case Command::MANUAL:
            if ( _current_mode != MANUAL_MODE )
            {
                _logger->logDebug("Manual mode");
                _current_mode = MANUAL_MODE;
                _logger->logEvent( LogItem::MANUAL_MODE );
                if ( _manual_off_time < _last_time )
                    _manual_off_time = 0;
                update_status = true;
                save_config = true;
                _external_request = false;
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

        case Command::EXTERNAL:
            if ( _mqtt != NULL &&
                ( !_external_request_topic.empty() ) )
            {
                _mqtt->subscribe( _external_request_topic );
                _logger->logDebug("External mode");
                _current_mode = EXTERNAL_MODE;
                _logger->logEvent( LogItem::MANUAL_MODE );
                update_status = true;
                save_config = true;
            }
            else
            {
                _logger->logDebug("Unable to activate external mode: MQTT not enabled or topic not defined.");
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

        case Command::SET_HISTERESYS_MAX:
        {
            _logger->logDebug("New HYSTERESIS_MAX received: " + cmd->getParam());
            float tmp_hyst = FrameworkUtils::string_tof( cmd->getParam() );
            if ( tmp_hyst >= 0.1f )
            {
                _logger->logDebug("Changed hysteresis max to " + cmd->getParam() );
                _logger->logEvent( LogItem::HYST_UPDATE );
                _hysteresis_max = tmp_hyst;
                if ( _hysteresis_max >= _excessive_overtemp_threshold )
                {
                    _logger->logEvent( LogItem::EXCESSIVE_OVERTEMP_UPD );
                    _excessive_overtemp_threshold = _hysteresis_max+1.0f;
                }
                update_status = true;
                save_config = true;
            }
        }
        break;

        case Command::SET_HISTERESYS_MIN:
        {
            _logger->logDebug("New HYSTERESIS_MIN received: " + cmd->getParam());
            float tmp_hyst = FrameworkUtils::string_tof( cmd->getParam() );
            if ( tmp_hyst >= 0.1f )
            {
                _logger->logDebug("Changed hysteresis min to " + cmd->getParam() );
                _logger->logEvent( LogItem::HYST_UPDATE );
                _hysteresis_min = tmp_hyst;
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
                // Reset the excessive overtemp, so that it can be recalculated
                _excessive_overtemp = false;
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
    {

        // Check for pellet flameout condition, because it will impact on gas
        if ( _checkFlameout() )
            update_status = true;

        // By default, all is off here:
        bool gas_on = false;
        bool pellet_on = false;
        bool pellet_minimum_on = false;

        if ( _current_mode != EXTERNAL_MODE )
        {
            // a valid temperature read is required for anti-ice detection
            if (_sensor_success_reads > 0)
            {
                // First of all, check anti-ice:
                if ( _checkAntiIce() )
                    update_status = true;
                // Anti-ice has priority on all other settings
                if ( _anti_ice_active )
                {
                    gas_on = true;
                    pellet_minimum_on = false;
                    // Pellet is not turned on, only "kept" on if already on
                    pellet_on = _pellet->isOn();
                    // we are in under temp by default:
                    _under_temp = true;
                }
            } // valid sensor reads

            // Ok, no anti-ice, keep the other logics active (here we are either in MANUAL or AUTO mode, not EXTERNAL mode)
            if ( !_anti_ice_active )
            {
                // Start by updating smart temperature value
                _updateSmartTemp();

                float sensor_temp = _temp_sensor->getTemp();
                // Calculating over temp and under temp depends on mode.
                // In MANUAL mode:
                //       OverTemp means above maximum temperature
                //       UnderTemp means below minimum temperature
                //    In other words, when in "manual" mode there is a gap between _max_temp and _min_temp where
                //    no action is taken (neither over nor under temp). Inside this gap, you can manually control.
                // In AUTO mode, instead, it depends if the program is set for "high" or for "low":
                //       OverTemp means above current threshold
                //       UnderTemp means below current threshold
                //     In other words, when in "auto" mode you are always either "over" or "under", in any case
                //     an action will always be taken to wither keep temperature stable or try to change it.
                float max_temp = _max_temp;
                float min_temp = _min_temp;
                float max_hyst = _hysteresis_max;
                float min_hyst = _hysteresis_min;
                if ( _current_mode == AUTO_MODE )
                {
                    if ( _program.useHigh() )
                    {
                        if ( _smart_temp_on )
                            max_temp = _smart_temp;
                        min_temp = max_temp;
                        min_hyst = max_hyst;
                    }
                    else
                    {
                        max_temp = min_temp;
                        max_hyst = min_hyst;
                    }
                }
                // Verify overtemp and undertemp conditions
                if ( _checkTargetTemperature( sensor_temp,
                                            max_temp, max_hyst,
                                            min_temp, min_hyst,
                                            _over_temp,
                                            _under_temp ) )
                    update_status = true;

                // Verify excess temperature condition:
                if ( _checkExcessTemperature( sensor_temp,
                                            _max_temp,
                                            _excessive_overtemp_threshold,
                                            _excessive_overtemp ) )
                    update_status = true;

                // When in "excessive overtemp", we shall turn off everything, pellet included.
                if ( !_excessive_overtemp )
                {
                    // In manual mode, control is fully manual and we keep whatever the user selected
                    if  (_current_mode == MANUAL_MODE)
                    {
                        // In manual mode, the user can enable an "off" timer
                        bool all_is_off = false;
                        if ( _manual_off_time != 0 )
                        {
                            // When reached, all will be shut off and the timer is disabled
                            if ( _manual_off_time < _last_time )
                            {
                                all_is_off = true;
                                _manual_off_time = 0;
                                _logger->logDebug("Manual OFF time reached");
                                _logger->logEvent( LogItem::MANUAL_OFF_TIME );
                                _logger->logDebug("Manual OFF time reset");
                                _logger->logEvent( LogItem::MANUAL_OFF_TIME_UPDATE );
                                update_status = true;
                            }
                        }
                        // Only if off timer has not been triggered...
                        if ( !all_is_off )
                        {
                            // Always respect manual user selection
                            pellet_on = _pellet->isOn();
                            pellet_minimum_on = _pellet->isLow() || _pellet_minimum_status_at_overtemp;
                            // The exception is GAS, which goes ON if a pellet flamout is detected.
                            // (note that if pellet is OFF, flameout is always false)
                            gas_on = (_gas->isOn() || _gas_status_at_overtemp) || (!pellet_minimum_on && _pellet_flameout);
                        }
                    }
                    // In automatic mode we use the program as source for pellet and gas status
                    else if ( _current_mode == AUTO_MODE )
                    {
                        pellet_on = _program.usePellet();
                        gas_on = _program.useGas();
                    }

                    // When over temp is detected, turn off/minimum
                    if ( _over_temp )
                    {
                        // Status is stored so it's resumed properly
                        _gas_status_at_overtemp = gas_on;
                        _pellet_minimum_status_at_overtemp = pellet_minimum_on;
                        if ( pellet_on )
                            pellet_minimum_on = true;
                        if ( gas_on )
                            gas_on = false;
                    }
                    else
                    {
                        _gas_status_at_overtemp = false;
                        _pellet_minimum_status_at_overtemp = false;
                    }
                } // end of not excessive temp
            } // not in anti-ice condition, but we are in AUTO or MANUAL mode.
        }
        else // else: we are in EXTERNAL mode down here:
        {
            // Currently, until implemented in the Home Assistant side, we read gas or pellet from the program
            pellet_on = _program.usePellet();
            gas_on = _program.useGas();

            // Currently the "external request" means, we are under temp and need to turn on stuff:
            _under_temp = _external_request;
            // we do not consider over temperature when in external mode, not anti_ice
            _over_temp = false;
        }

        // More logic to detect which generator must be used now
        // When under temp is detected, turn on the appropriate generator
        if ( _under_temp ) // this applies also to ice condition detected
        {
            // If pellet is on and not in flameout, switch to modulation
            if ( pellet_on && !_pellet_flameout )
            {
                if ( (_current_mode == MANUAL_MODE) && pellet_minimum_on )
                    _manual_pellet_minimum_forced_off = true;
                pellet_minimum_on = false;
            }
            else // Pellet is off or in flameout, let's turn gas on now:
            {
                if ( (_current_mode == MANUAL_MODE) && !gas_on )
                    _manual_gas_forced_on = true;
                gas_on = true;
            }
        }
        else
        {
            if ( _manual_gas_forced_on )
                gas_on = false;
            if ( _manual_pellet_minimum_forced_off )
                pellet_minimum_on = true;
            _manual_gas_forced_on = false;
            _manual_pellet_minimum_forced_off = false;
        }

        // Now ensure request consistency with generators current state:
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

        // When pellet is ON and HOT we must take care than gas is OFF
        // otherwise, gas will heat on the primary circuit and will prevent
        // pellet from dissipating the produced heat. This will cause the pellet
        // to go above the maximum temperature and switch off autmatically.
        // We want to prevent this situation, so if pellet is ON and HOT
        // we switch off gas byd efault.
        // If pellet is OFF we can safely turn on gas.
        // If pellet is ON but not yet HOT, we can also safely turn on gas.
        if ( _pellet->isOn() && _pellet->isHot() )
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
	if ( _debug_updates )
            _logger->logDebug("Time interval (" + FrameworkUtils::utostring(current_time) +" - " + FrameworkUtils::utostring(_last_time) + " >= 60)");
        _updateCurrentTime( current_time );

        // We debug the temperatures only every minute
        if ( _sensor_success_reads > 0 )
        {
            if ( !_history.update( _temp_sensor->getTemp(), _temp_sensor->getHumidity(),
                                 _current_ext_temp, _current_ext_humidity ) )
                _logger->logDebug("Unable to write to history file");
            if ( _mqtt != NULL )
            {
                std::string topic = "terra/disimpegno/temp";
                std::string data = "{ \"temperature\": ";
                data += FrameworkUtils::ftostring( _temp_sensor->getTemp() );
                data += ", \"humidity\": ";
                data += FrameworkUtils::ftostring( _temp_sensor->getHumidity() );
                data += " }";
                _mqtt->publish( topic, data );
            }
        }
        if ( _mqtt != NULL )
        {
            std::string topic = "terra/tv/pellet_status";
            std::string data = "{ ";
            data += "\"on\": ";
            data += _pellet->isOn() ? "1" : "0";
            data += ", \"modulation\": ";
            data += _pellet->isLow() ? "0" : "1";
            data += ", \"hot\": ";
            data += _pellet->isHot() ? "1" : "0";
            data += " }";
            _mqtt->publish( topic, data );
        }
        if ( _mqtt != NULL )
        {
            std::string topic = "terra/disimpegno/gas_status";
            std::string data = "{ ";
            data += "\"on\": ";
            data += _gas->isOn() ? "1" : "0";
            data += " }";
            _mqtt->publish( topic, data );
        }

    }

    if ( update_status )
    {
        if ( _debug_updates )
        {
            _pellet->printStatus();
            _gas->printStatus();
            _temp_sensor->printStatus();
        }
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


bool RunnerThread::_checkExcessTemperature(float sensor_temp,
                                           float target_temperature,
                                           float excessive_overtemp_threshold,
                                           bool& excessive_overtemp )
{
    bool update_status = false;
    // Excessive over-temperature is activated when sensor_temp is past the threshold
    if ( !excessive_overtemp )
    {
        excessive_overtemp = sensor_temp > ( target_temperature + excessive_overtemp_threshold );
        if ( excessive_overtemp )
        {
            _logger->logDebug("Excessive overtemp detected");
            _logger->logEvent( LogItem::EXCESSIVE_OVERTEMP_ON );
            update_status = true;
        }
    }
    // ... but stays active until sensor_temp drops below the target temperature, no matter the program mode
    else if ( excessive_overtemp )
    {
        excessive_overtemp = sensor_temp > target_temperature;
        if ( !excessive_overtemp )
        {
            _logger->logDebug("Resume from excessive overtemp");
            _logger->logEvent( LogItem::EXCESSIVE_OVERTEMP_OFF );
            update_status = true;
        }
    }
    return update_status;
}


bool RunnerThread::_checkTargetTemperature(float sensor_temp,
                                           float target_temperature_max,
                                           float hysteresis_max,
                                           float target_temperature_min,
                                           float hysteresis_min,
                                           bool& prev_over_temp,
                                           bool& prev_under_temp )
{
    bool update_status = false;
    if ( prev_under_temp )
    {
        target_temperature_max += hysteresis_max;
        target_temperature_min += hysteresis_min;
    }
    bool new_over = sensor_temp > target_temperature_max;
    bool new_under = sensor_temp < target_temperature_min;

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
    float delta = -1.0f;
    if ( _hour <= 8)
        delta = -1.0f;
    else if ( _hour == 9 )
        delta = -1.0f;
    else if ( _hour == 10 )
        delta = -1.0f;
    else if ( _hour == 11 )
        delta = -0.9f;
    else if ( _hour == 12 )
        delta = -0.9f;
    else if ( _hour == 13 )
        delta = -0.8f;
    else if ( _hour == 14 )
        delta = -0.7f;
    else if ( _hour == 15 )
        delta = -0.4f;
    else if ( _hour == 16 )
        delta = -0.2f;
    else if ( _hour == 17 )
        delta = -0.1f;
    else if ( _hour >= 18 )
        delta = -0.0f;

    _smart_temp = _max_temp + delta;
}

void RunnerThread::_saveConfig()
{
    _config->setValueBool("activated", _heating_activated );
    _config->setValueBool("smart_temp", _smart_temp_on );
    if ( _current_mode == MANUAL_MODE )
        _config->setValue( "mode", "manual" );
    else if ( _current_mode == EXTERNAL_MODE )
        _config->setValue( "mode", "external" );
    else
        _config->setValue( "mode", "auto" );
    _config->setValue( "min_temp", FrameworkUtils::ftostring( _min_temp ) );
    _config->setValue( "max_temp", FrameworkUtils::ftostring( _max_temp ) );
    _config->setValue( "temp_correction", FrameworkUtils::ftostring( _temp_correction ) );
    _config->setValue( "pellet_startup_delay", FrameworkUtils::utostring(_pellet_startup_delay) );
    _config->setValue( "hysteresis_max", FrameworkUtils::ftostring( _hysteresis_max ) );
    _config->setValue( "hysteresis_min", FrameworkUtils::ftostring( _hysteresis_min ) );
    _config->setValue( "excessive_overtemp_ts", FrameworkUtils::ftostring( _excessive_overtemp_threshold ) );

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
    status.current_mode = _current_mode;
    status.pellet_on = _pellet->isOn();
    status.pellet_minimum = _pellet->isLow();
    status.pellet_hot = _pellet->isHot();
    status.pellet_flameout = _pellet_flameout;
    status.gas_on = _gas->isOn();
    status.smart_temp_on = _smart_temp_on;
    status.smart_temp = _smart_temp;
    status.max_temp = _max_temp;
    status.min_temp = _min_temp;
    status.hysteresis_max = _hysteresis_max;
    status.hysteresis_min = _hysteresis_min;
    status.temp_int = _temp_sensor->getTemp();
    status.humidity_int = _temp_sensor->getHumidity();
    status.temp_ext = _current_ext_temp;
    status.humidity_ext = _current_ext_humidity;
    status.day = _day;
    status.hour = _hour;
    status.half = _half_hour;
    status.manual_off_time = _manual_off_time;
    status.excess_threshold = _excessive_overtemp_threshold;
    status.excess_temp_reached = _excessive_overtemp;
    _program.writeRaw( &status );
    memcpy( _shared_status.getWritePtr(), &status, _shared_status.getSharedSize() );
}

