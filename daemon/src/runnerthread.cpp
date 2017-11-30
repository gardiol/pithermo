#include "runnerthread.h"

#include <stdio.h>

#define EPOCH_DOW 3
#define SECS_PER_DAY 86400
#define SECS_PER_HOUR 3600
#define SECS_PER_HHOUR 1800

#include "configfile.h"
#include "debugprint.h"

#include "command.h"
#include "program.h"

RunnerThread::RunnerThread(const std::string &cfg, const std::string &exchange_path):
    ScheduledThread("Runner", 10 * 1000 ),
    _config_file( cfg ),
    _exchange_path( exchange_path ),
    _commands_mutex("CommandsMutex"),
    _pellet_command(false),
    _gas_command(false),
    _manual_mode(true),
    _current_temp(20.0),
    _min_temp(16.0),
    _max_gas_temp(17.0),
    _max_temp(22.0),
    _error(false),
    _program(NULL)
{
    startThread();
    while ( !isRunning() && !_error )
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
    if ( _program != NULL )
    {
        delete _program;
        _program = NULL;
    }
}

void RunnerThread::appendCommand(Command *cmd)
{
    _commands_mutex.lock();
    _commands_list.push_back( cmd );
    _commands_mutex.unlock();
}

bool RunnerThread::scheduledRun(uint64_t elapsed_time_us, uint64_t cycle)
{
    bool update_status = false;
    bool save_config = false;
    bool ret = true;
    _commands_mutex.lock();
    while ( _commands_list.size() > 0 )
    {
        Command* cmd = _commands_list.front();
        _commands_list.pop_front();
        switch ( cmd->command() )
        {
        case Command::PELLET_ON:
            if ( !_pellet_command )
            {
                _pellet_command = true;
                update_status = true;
            }
            break;

        case Command::PELLET_OFF:
            if ( _pellet_command )
            {
                _pellet_command = false;
                update_status = true;
            }
            break;

        case Command::GAS_ON:
            if ( !_gas_command )
            {
                _gas_command = true;
                update_status = true;
            }
            break;

        case Command::GAS_OFF:
            if ( _gas_command )
            {
                _gas_command = false;
                update_status = true;
            }
            break;

        case Command::MANUAL:
            if ( !_manual_mode )
            {
                _manual_mode = true;
                update_status = true;
                save_config = true;
            }
            break;

        case Command::AUTO:
            if ( _manual_mode )
            {
                _manual_mode = false;
                update_status = true;
                save_config = true;
            }
            break;

        case Command::INVALID:
        default:
            break;
        }
    }
    _commands_mutex.unlock();


    uint32_t current_time = FrameworkTimer::getTimeEpoc();
    if ( (_last_time+60) < current_time )
    {
        _last_time = current_time;
        update_status = true;
    }

    if ( save_config )
        saveConfig();
    if ( update_status )
        updateStatus();

    return ret;
}

bool RunnerThread::scheduleStart()
{
    _last_time = 0;
    _error = false;
    std::string content;
    FrameworkUtils::file_to_str( _config_file, content );
    ConfigFile config("config", content );
    if ( !config.isEmpty() )
    {
        _manual_mode = FrameworkUtils::string_tolower(config.getValue( "mode" )) == "manual";
        _min_temp = FrameworkUtils::string_tof( config.getValue( "min_temp" ) );
        _max_gas_temp = FrameworkUtils::string_tof( config.getValue( "max_gas_temp" ) );
        _max_temp = FrameworkUtils::string_tof( config.getValue( "max_temp" ) );
    }
    else
        debugPrintWarning() << "Warning: empty config file!\n";
    return !_error;
}

void RunnerThread::saveConfig()
{
    std::string content;
    FrameworkUtils::file_to_str( _config_file, content );
    ConfigFile config("config", content );
    config.setValue( "mode", _manual_mode ? "manual" : "auto" );
    config.setValue( "min_temp", FrameworkUtils::ftostring( _min_temp ) );
    config.setValue( "max_gas_temp", FrameworkUtils::ftostring( _max_gas_temp ) );
    config.setValue( "max_temp", FrameworkUtils::ftostring( _max_temp ) );
    FrameworkUtils::str_to_file( _config_file, config.toStr() );
}

void RunnerThread::updateStatus()
{
    FILE* status_file = fopen( (_exchange_path+"/_status").c_str(), "w" );
    if ( status_file )
    {
        std::string json = "{";
        json += "\"mode\":";
        json += ( _manual_mode ? "\"manual\"" : "\"auto\"" );
        json += ",";
        json += "\"pellet\":{\"command\":";
        json += (_pellet_command ? "\"on\"" : "\"off\"" );
        json += ",\"status\":\"off\"},";
        json += "\"gas\":{\"command\":";
        json += (_gas_command ? "\"on\"" : "\"off\"" );
        json += ",\"status\":\"off\"},";

        json += "\"program\":[";
        for ( int d = 0; d < 7; d++ )
        {
            json += "[";
            for ( int h = 0; h < 24; h++ )
            {
                for ( int f = 0; f < 2; f++ )
                {
                    bool pellet_on = _program != NULL ? _program->getPellet(d,h,f) : false;
                    bool gas_on = _program != NULL ? _program->getGas(d,h,f) : false;
                    json += "\"";
                    json += pellet_on ? (gas_on ? "x" : "p") : (gas_on ? "g" : "");
                    json += "\"";
                    if ( (h != 23) || (f != 1) )
                        json += ",";
                }
            }
            json += "]";
            if ( d != 6 )
                json += ",";
        }
        json += "],";

        uint32_t day_of_week = (uint32_t)( ( (_last_time / SECS_PER_DAY) + EPOCH_DOW) % 7);
        uint32_t hour = (uint32_t)( (_last_time / SECS_PER_HOUR) % 24);
        uint32_t half = (uint32_t)( (_last_time / SECS_PER_HHOUR) % 2);

        json += "\"now\":{\"d\":"+FrameworkUtils::tostring( day_of_week ) +
                ",\"h\":"+FrameworkUtils::tostring(hour) +
                ",\"f\":"+FrameworkUtils::tostring(half) + "}";

        json +="}\n";
        fwrite( json.c_str(), json.length(), 1, status_file );
        fclose( status_file );
    }
}

void RunnerThread::schedulingStopped()
{
}

