#include "runnerthread.h"

#include <string.h>
#include <time.h>
#include <stdio.h>

#include "configfile.h"
#include "debugprint.h"
#include "profiler.h"

#include "command.h"
#include "program.h"

RunnerThread::RunnerThread(const std::string &cfg, const std::string &exchange_path, const std::string &hst):
    ScheduledThread("Runner", 10 * 1000 ),
    _config_file( cfg ),
    _exchange_path( exchange_path ),
    _history_file( hst ),
    _commands_mutex("CommandsMutex"),
    _pellet_command(false),
    _gas_command(false),
    _manual_mode(true),
    _current_temp(20.0),
    _min_temp(16.0),
    _max_temp(17.0),
    _error(false),
    _history_warned(false),
    _str_manual("manual"),
    _str_auto("auto"),
    _str_on("on"),
    _str_off("off")
{
    _status_json_template.push_back("{\"mode\":\"");
    _status_json_template.push_back("\",\"pellet\":{\"command\":\"");
    _status_json_template.push_back("\",\"status\":\"off\"},\"gas\":{\"command\":\"");
    _status_json_template.push_back("\",\"status\":\"off\"},\"temp\":{\"min\":");
    _status_json_template.push_back(",\"max\":");
    _status_json_template.push_back("},\"now\":{\"d\":");
    _status_json_template.push_back(",\"h\":");
    _status_json_template.push_back(",\"f\":");
    _status_json_template.push_back("},\"program\":");

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
    bool update_history = false;
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

        case Command::SET_MIN_TEMP:
        {
            float tmp_temp = FrameworkUtils::string_tof( cmd->getParam() );
            if ( tmp_temp != _min_temp )
            {
                _min_temp = tmp_temp;
                _str_min_t = FrameworkUtils::ftostring( _min_temp );
                update_status = true;
                save_config = true;
            }
        }
            break;

        case Command::SET_MAX_TEMP:
        {
            float tmp_temp = FrameworkUtils::string_tof( cmd->getParam() );
            if ( tmp_temp != _max_temp )
            {
                _max_temp = tmp_temp;
                _str_max_t = FrameworkUtils::ftostring( _max_temp );
                update_status = true;
                save_config = true;
            }
        }
            break;

        case Command::PROGRAM:
            if ( _program.change( cmd->getParam() ) )
            {
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


    uint64_t current_time = FrameworkTimer::getTimeEpoc();
    if ( (_last_time+60) <= current_time )
    {
        _current_temp += 0.5;
        if ( _current_temp > 28.0 )
            _current_temp = 12.0;

        _last_time = current_time;
        time_t now = (time_t)_last_time;
        struct tm *tm_struct = localtime(&now);
        _str_f = tm_struct->tm_min > 30 ? "1" : "0";
        _str_day = FrameworkUtils::tostring( (tm_struct->tm_wday-1)%7 );
        _str_h = FrameworkUtils::tostring( tm_struct->tm_hour );
        update_status = true;
        update_history = true;
    }

    if ( save_config )
        saveConfig();
    if ( update_status )
        updateStatus();
    if ( update_history )
        updateHistory();

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
        _str_min_t = FrameworkUtils::ftostring( _min_temp );
        _max_temp = FrameworkUtils::string_tof( config.getValue( "max_temp" ) );
        _str_max_t = FrameworkUtils::ftostring( _max_temp );
        _program.loadConfig( config.getSection( "program" ) );
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
    config.setValue( "max_temp", FrameworkUtils::ftostring( _max_temp ) );
    ConfigData* prog_section = config.getSection( "program" );
    if ( prog_section == NULL )
        prog_section = config.newSection( "program" );
    _program.saveConfig( prog_section );
    if ( !FrameworkUtils::str_to_file( _config_file, config.toStr() ) )
        debugPrintError() << "Unable to save file " << _config_file << "\n";
}

void RunnerThread::updateStatus()
{
    FILE* status_file = fopen( (_exchange_path+"/_status").c_str(), "w" );
    if ( status_file )
    {
        fwrite( _status_json_template[0].c_str(), _status_json_template[0].length(), 1, status_file);
        if ( _manual_mode )
            fwrite( _str_manual.c_str(), _str_manual.length(), 1, status_file);
        else
            fwrite( _str_auto.c_str(), _str_auto.length(), 1, status_file);

        fwrite( _status_json_template[1].c_str(), _status_json_template[1].length(), 1, status_file);
        if ( _pellet_command )
            fwrite( _str_on.c_str(), _str_on.length(), 1, status_file);
        else
            fwrite( _str_off.c_str(), _str_off.length(), 1, status_file);

        fwrite( _status_json_template[2].c_str(), _status_json_template[2].length(), 1, status_file);
        if ( _gas_command )
            fwrite( _str_on.c_str(), _str_on.length(), 1, status_file);
        else
            fwrite( _str_off.c_str(), _str_off.length(), 1, status_file);

        fwrite( _status_json_template[3].c_str(), _status_json_template[3].length(), 1, status_file);
        fwrite( _str_min_t.c_str(), _str_min_t.length(), 1, status_file);

        fwrite( _status_json_template[4].c_str(), _status_json_template[4].length(), 1, status_file);
        fwrite( _str_max_t.c_str(), _str_max_t.length(), 1, status_file);

        fwrite( _status_json_template[5].c_str(), _status_json_template[5].length(), 1, status_file);
        fwrite( _str_day.c_str(), _str_day.length(), 1, status_file);

        fwrite( _status_json_template[6].c_str(), _status_json_template[6].length(), 1, status_file);
        fwrite( _str_h.c_str(), _str_h.length(), 1, status_file);

        fwrite( _status_json_template[7].c_str(), _status_json_template[7].length(), 1, status_file);
        fwrite( _str_f.c_str(), _str_f.length(), 1, status_file);

        fwrite( _status_json_template[8].c_str(), _status_json_template[8].length(), 1, status_file);

        _program.writeJSON( status_file );

        fwrite( "}", 1, 1, status_file );
        fclose( status_file );
    }
}

void RunnerThread::updateHistory()
{
    _temp_history.push_back( _current_temp );
    while ( _temp_history.size() > 100 )
        _temp_history.pop_front();

    FILE* history_file = fopen( _history_file.c_str(), "a" );
    if ( history_file != NULL )
    {
        char buffer[ sizeof(_last_time) + sizeof(_current_temp) ];
        memcpy( &buffer[0], &_last_time, sizeof(_last_time) );
        memcpy( &buffer[sizeof(_last_time)], &_current_temp, sizeof(_current_temp) );
        fwrite( buffer, sizeof(_last_time) + sizeof(_current_temp), 1, history_file );
        fclose(history_file);
    }
    else if ( !_history_warned )
    {
        _history_warned = true;
        debugPrintWarning() << "Warning: unable to open history file!\n";
    }

    FILE* history_json = fopen( (_exchange_path+"/_history").c_str(), "w" );
    if ( history_json )
    {
        fwrite( "[", 1, 1, history_json );
        for ( std::list<float>::iterator t = _temp_history.begin(); t != _temp_history.end(); ++t )
        {
            float temp = *t;
            if ( t != _temp_history.begin() )
                fwrite( ",", 1, 1, history_json );
            std::string x = FrameworkUtils::ftostring( temp );
            fwrite( x.c_str(), x.length(), 1, history_json );
        }
        fwrite( "]", 1, 1, history_json );
        fclose( history_json );
    }
}

void RunnerThread::schedulingStopped()
{
}

