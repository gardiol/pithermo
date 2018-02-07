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

#ifndef NOPI
#include <wiringPi.h>
#else
#define OUTPUT 0
#define INPUT 1
#endif

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
    _config_file( cfg ),
    _exchange_path( exchange_path ),
    _commands_mutex("CommandsMutex"),
    _manual_mode(true),
    _anti_ice_active(false),
    _over_temp(false),
    _under_temp(false),
    _gas_was_on_before_over(false),
    _pellet_flameout(false),
    _prev_pellet_feedback(false),
    _min_temp(16.0),
    _max_temp(17.0),
    _temp_correction(1.0),
    _sensor_failed_reads(0),
    _sensor_success_reads(0),
    _sensor_timer(),
    _pellet_command_gpio(2),
    _pellet_minimum_gpio(5),
    _pellet_feedback_gpio(7),
    _gas_command_gpio(0),
    _sensor_gpio(),
    _num_warnings(5),
    _last_time(0),
    _current_temp(0.0),
    _current_humidity(50.0),
    _program_gas(false),
    _program_pellet(false),
    _program_pellet_minimum(false),
    _gpio_error(false),
    _history( hst, exchange_path ),
    _str_manual("manual"),
    _str_auto("auto"),
    _str_on("on"),
    _str_off("off"),
    _str_min_t(FrameworkUtils::ftostring( _min_temp ) ),
    _str_max_t(FrameworkUtils::ftostring( _max_temp ) ),
    _str_day("0"),
    _str_h("00"),
    _str_f("0"),
    _str_pellet_off_warning("La stufa a pellet sarà spenta!"),
    _str_pellet_on_warning("La stufa a pellet sarà accesa!"),
    _day(0),
    _hour(0),
    _half_hour(0)
{
    _status_json_template.push_back("{\"mode\":\"");
    _status_json_template.push_back("\",\"antiice\":\"");
    _status_json_template.push_back("\",\"warnings\":{\"modeswitch\":\"");
    _status_json_template.push_back("\",\"messages\":[");
    _status_json_template.push_back("]},\"pellet\":{\"command\":\"");
    _status_json_template.push_back("\",\"status\":\"");
    _status_json_template.push_back("\",\"minimum\":\"");
    _status_json_template.push_back("\",\"flameout\":\"");
    _status_json_template.push_back("\"},\"gas\":{\"command\":\"");
    _status_json_template.push_back("\",\"status\":\"off\"},\"temp\":{\"min\":");
    _status_json_template.push_back(",\"max\":");
    _status_json_template.push_back("},\"now\":{\"d\":");
    _status_json_template.push_back(",\"h\":");
    _status_json_template.push_back(",\"f\":");
    _status_json_template.push_back("},\"program\":");

#ifdef NOPI
    debugPrintNotice("") << "DemoMode\n";
    if ( true )
#else
    if (wiringPiSetup () != -1)
#endif
    {
        startThread();
        while ( !_gpio_error && !isRunning() )
            FrameworkTimer::msleep_s( 100 );
        _logger->logEvent( "Runner created");
    }
    else
        _logger->logEvent( "WiringPi initialization error");
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
    saveConfig();
    _logger->logEvent( "Runner closed");
}

void RunnerThread::appendCommand(Command *cmd)
{
    _logger->logDebug("Appending command: " + cmd->commandStr() );
    _commands_mutex.lock();
    _commands_list.push_back( cmd );
    _commands_mutex.unlock();
}

bool RunnerThread::checkGas()
{
    bool x = !readGpioBool( _gas_command_gpio, OUTPUT );
    _logger->logDebug(std::string("Check Gas: ") + (x ? "on" : "off") );
    return x;
}

void RunnerThread::gasOn()
{ // on = LOW/false
    setGpioBool( _gas_command_gpio, false );
    _logger->logEvent("gas ON");
}

void RunnerThread::gasOff()
{ // off = HIGH/true
    setGpioBool( _gas_command_gpio, true );
    _logger->logEvent("gas OFF");
}

bool RunnerThread::checkPellet()
{
    bool x = !readGpioBool( _pellet_command_gpio, OUTPUT );
    _logger->logDebug(std::string("Check Pellet: ") + (x ? "on" : "off") );
    return x;
}

void RunnerThread::pelletOn()
{ // on = LOW/false
    setGpioBool( _pellet_command_gpio, false );
    _logger->logEvent("pellet ON");
}

void RunnerThread::pelletOff()
{ // off = HIGH/true
    setGpioBool( _pellet_command_gpio, true );
    _logger->logEvent("pellet OFF");
}

bool RunnerThread::checkPelletMinimum()
{
    bool x = !readGpioBool( _pellet_minimum_gpio, OUTPUT );
    _logger->logDebug(std::string("Check PelletMinimum: ") + (x ? "on" : "off") );
    return x;
}

void RunnerThread::pelletMinimum(bool m)
{ // on: LOW - off: HIGH
    setGpioBool( _pellet_minimum_gpio, !m );
    if ( m )
        _logger->logEvent("pellet MINIMO");
    else
        _logger->logEvent("pellet MODULAZIONE");
}

bool RunnerThread::pelletFeedback()
{   // HIGH: mandata fredda, termostato off, relé chiuso
    // LOW: mandata calda, termostato on, relé aperto
    bool fdb = !readGpioBool( _pellet_feedback_gpio, INPUT );
    if ( fdb )
        _logger->logDebug("Feedback termostato mandata pellet CALDO");
    else
        _logger->logDebug("Feedback termostato mandata pellet FREDDO");
    return fdb;
}

bool RunnerThread::scheduledRun(uint64_t elapsed_time_us, uint64_t cycle)
{
    bool update_status = false;
    bool check_program = false;
    bool save_config = false;
    bool ret = true;
    bool current_pellet_feedback = pelletFeedback();

    // Processa comandi
    _commands_mutex.lock();
    while ( _commands_list.size() > 0 )
    {
        Command* cmd = _commands_list.front();
        _commands_list.pop_front();
        switch ( cmd->command() )
        {
        case Command::FLAMEOUT_RESET:
            if ( _pellet_flameout )
            {
                _pellet_flameout = false;
                _logger->logDebug("Flameout cancelled");
                appendMessage("Flameout cancellato!");
                if ( !_manual_mode )
                    check_program = true;
            }
            break;

        case Command::SET_HISTORY:
            _logger->logDebug("Change history received");
            _history.setMode( cmd->getParam() );
            _logger->logDebug("Change history, mode: " + cmd->getParam());
            break;

        case Command::PELLET_MINIMUM_ON:
            _logger->logDebug("Pellet Minimum ON received");
            if ( _manual_mode )
            {
                pelletMinimum( true );
                update_status = true;
            }
            break;

        case Command::PELLET_MINIMUM_OFF:
            _logger->logDebug("Pellet Minimum OFF received");
            if ( _manual_mode )
            {
                pelletMinimum( false );
                update_status = true;
            }
            break;

        case Command::PELLET_ON:
            _logger->logDebug("Pellet ON received");
            if ( _manual_mode )
            {
                pelletOn();
                update_status = true;
            }
            break;

        case Command::PELLET_OFF:
            _logger->logDebug("Pellet OFF received");
            if ( _manual_mode )
            {
                pelletOff();
                update_status = true;
            }
            break;

        case Command::GAS_ON:
            _logger->logDebug("Gas ON received");
            if ( _manual_mode )
            {
                gasOn();
                update_status = true;
            }
            break;

        case Command::GAS_OFF:
            _logger->logDebug("Gas OFF received");
            if ( _manual_mode )
            {
                gasOff();
                update_status = true;
            }
            break;

        case Command::MANUAL:
            _logger->logEvent("Manual mode");
            _manual_mode = true;
            update_status = true;
            save_config = true;
            break;

        case Command::AUTO:
            _logger->logEvent("Auto mode");
            _manual_mode = false;
            updateProgram();
            check_program = true;
            update_status = true;
            save_config = true;
            break;

        case Command::SET_MIN_TEMP:
        {
            _logger->logDebug("New MIN TEMP received: " + cmd->getParam());
            float tmp_temp = FrameworkUtils::string_tof( cmd->getParam() );
            if ( tmp_temp != _min_temp )
            {
                _logger->logEvent("Changed min temp to " + cmd->getParam() );
                _min_temp = tmp_temp;
                _str_min_t = FrameworkUtils::ftostring( _min_temp );
                update_status = true;
                save_config = true;
            }
        }
            break;

        case Command::SET_MAX_TEMP:
        {
            _logger->logDebug("New MAX TEMP received: " + cmd->getParam());
            float tmp_temp = FrameworkUtils::string_tof( cmd->getParam() );
            if ( tmp_temp != _max_temp )
            {
                _logger->logEvent("Changed max temp to " + cmd->getParam() );
                _max_temp = tmp_temp;
                _str_max_t = FrameworkUtils::ftostring( _max_temp );
                update_status = true;
                save_config = true;
            }
        }
            break;

        case Command::PROGRAM:
            _logger->logDebug("New program received: " + cmd->getParam() );
            if ( _program.change( cmd->getParam() ) )
            {
                _logger->logEvent("Program changed");
                update_status = true;
                save_config = true;
                updateProgram();
                if ( !_manual_mode )
                    check_program = true;
            }
            break;

        case Command::INVALID:
        default:
            _logger->logDebug("Invalid command received (" + cmd->commandStr() + ")");
            break;
        }
    }
    _commands_mutex.unlock();

    // Cerca di leggere il sensore di temperatura
    if ( !_sensor_timer.isRunning() || _sensor_timer.elapsedLoop() )
    {
        if ( readSensor( _current_temp, _current_humidity ) )
            _logger->logTemp( _current_temp, _current_humidity );
        _sensor_timer.reset();
    }

    // Applica i constraint di temperatura se abbiamo almeno una lettura valida dal sensore
    if ( _sensor_success_reads > 0 )
    {   // Anti-ice ha sempre la priorità
        if ( _current_temp <= 5.0 )
        {
            if ( !_anti_ice_active )
            {
                appendMessage("Anti-ice attivato!");
                _logger->logEvent("Anti-ice ON");
                gasOn();
                update_status = true;
                _anti_ice_active = true;
            }
        }
        else if ( _anti_ice_active )
        {
            appendMessage("Anti-ice disattivato!");
            _logger->logEvent("Anti-ice OFF");
            gasOff();
            update_status = true;
            _anti_ice_active = false;
        }
        else // Se non siamo a rischio congelamento...
        {
            if ( !_over_temp && (_current_temp >= _max_temp) )
            {   // Superato il massimo, spegnamo:
                _over_temp = true;
                appendMessage("Max temp superata");
                _logger->logEvent("over temp start");
                _gas_was_on_before_over = checkGas();
                if ( _gas_was_on_before_over )
                    gasOff();
                if ( checkPellet() )
                    pelletMinimum(true);
                update_status = true;
            }
            else if ( _over_temp && (_current_temp < _max_temp) )
            {   // Rientrati dall'over-temperatura:
                _over_temp = false;
                appendMessage("Max temp rientrata");
                _logger->logEvent("over temp end");
                if ( _manual_mode )
                {
                    if ( _gas_was_on_before_over || _pellet_flameout )
                        gasOn();
                    if ( checkPellet() )
                        pelletMinimum(false);
                    _gas_was_on_before_over = false;
                    update_status = true;
                }
                else
                    check_program = true;
            }

            if ( !_under_temp && (_current_temp < _min_temp) )
            {   // Siamo sotto il minimo! Accendiamo qualcosa:
                _under_temp = true;
                appendMessage("Sotto la min temp");
                _logger->logEvent("under min temp start");
                if ( checkPellet() && !_pellet_flameout )
                    pelletMinimum( false );
                else
                    gasOn();
            }
            else if ( _under_temp && (_current_temp >= _min_temp) )
            {   // Siamo tornati "sopra" la temperatura minima:
                _under_temp = false;
                appendMessage("Min temp rientrata");
                _logger->logEvent("under min temp end");
                if ( _manual_mode )
                {   // Spegnamo:
                    if ( checkGas() )
                        gasOff();
                    if ( checkPellet() )
                        pelletMinimum(true);
                    update_status = true;
                }
                else
                    check_program = true;
            }
        }
    }

    if ( !_pellet_flameout && // we where not in flameout
         (checkPellet() && // pellet is on
          !current_pellet_feedback && // and pellet temp was HOT
          _prev_pellet_feedback) ) // but now it's not anymore...
    {   // Pellet has shut off for some reasons! (no more fuel?)
        _pellet_flameout = true;
        appendMessage("Pellet flameout! ");
        _logger->logEvent("pellet flameout start");
        if ( !checkGas() ) // Force gas on
            gasOn();
    }
    else if ( _pellet_flameout &&   // we are in flameout...
              current_pellet_feedback ) // but now pellet is back HOT...
    {
        _pellet_flameout = false;
        appendMessage("Pellet flameout annullato! ");
        _logger->logEvent("pellet flameout end");
        if ( !_manual_mode )
            check_program = true;
        else if ( checkGas() ) // This we can do anyway, since gas will be cut off by the feedback thermostat.
            gasOff();
    }

    // Aggiorna tempo, history e programma
    uint64_t current_time = FrameworkTimer::getTimeEpoc();
    if ( (_last_time+60) <= current_time )
    {
        _logger->logDebug("Time interval (" + FrameworkUtils::tostring(current_time) +" - " + FrameworkUtils::tostring(_last_time) + " >= 60)");
        _last_time = current_time;
        updateCurrentTime();
        updateProgram();
        if ( _sensor_success_reads > 0 )
        {
            if ( !_history.update( _current_temp, _current_humidity ) )
                _logger->logDebug("Unable to write to history file");
        }
        update_status = true;
        if ( !_manual_mode )
            check_program = true;
    }

    if ( !(_under_temp || _over_temp || _anti_ice_active ) && // Skip program under extraordinary circumstances
         ( ( (cycle == 0) && !_manual_mode ) || check_program ) )
    {
        _logger->logDebug("(in auto mode) Check program...");
        if ( _pellet_flameout && _program_pellet && !_program_pellet_minimum )
        {
            _program_gas = true;
            _program_pellet_minimum = true;
            _logger->logDebug( "program gas FORZATO: PELLET FLAMEOUT" );
            _logger->logDebug( "program pellet flameout" );
            _logger->logDebug( "program pellet minimum forzato per flamout" );
        }
        else
        {
            _logger->logDebug( std::string("program gas ") + (_program_gas ? "on" : "off"));
            _logger->logDebug( std::string("program pellet ") + (_program_pellet ? "on" : "off"));
            _logger->logDebug( std::string("program pellet minimum ") + (_program_pellet_minimum ? "on" : "off"));
        }
        if ( checkGas() )
        {
            if ( !_program_gas )
            {
                appendMessage("programma: gas spento");
                update_status = true;
                gasOff();
            }
        }
        else
        {
            if ( _program_gas )
            {
                appendMessage("programma: gas acceso");
                update_status = true;
                gasOn();
            }
        }
        if ( checkPellet() )
        {
            if ( !_program_pellet )
            {
                appendMessage("programma: pellet spento");
                update_status = true;
                pelletOff();
            }
        }
        else
        {
            if ( _program_pellet )
            {
                appendMessage("programma: pellet acceso");
                update_status = true;
                pelletOn();
            }
        }
        if ( checkPelletMinimum() )
        {
            if ( _program_pellet && !_program_pellet_minimum )
            {
                appendMessage("programma: pellet in modulazione");
                update_status = true;
                pelletMinimum(false);
            }
        }
        else
        {
            if ( _program_pellet && _program_pellet_minimum )
            {
                appendMessage("programma: pellet al minimo");
                update_status = true;
                pelletMinimum(true);
            }
        }
    }

    if ( save_config )
        saveConfig();
    if ( update_status )
        updateStatus( checkGas(),
                      checkPellet(),
                      checkPelletMinimum(),
                      current_pellet_feedback,
                      _pellet_flameout );

    _prev_pellet_feedback = current_pellet_feedback;
    return ret;
}

bool RunnerThread::scheduleStart()
{
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
        _str_min_t = FrameworkUtils::ftostring( _min_temp );
        _max_temp = FrameworkUtils::string_tof( config.getValue( "max_temp" ) );
        _str_max_t = FrameworkUtils::ftostring( _max_temp );
        _temp_correction = FrameworkUtils::string_tof( config.getValue( "temp_correction" ) );
        history_len = FrameworkUtils::string_toi( config.getValue("history_len") );
        history_mode = FrameworkUtils::string_tolower( config.getValue( "history_mode" ) );
        _num_warnings = FrameworkUtils::string_toi( config.getValue("num_warnings") );
        _program.loadConfig( config.getSection( "program" ) );
    }
    else
        saveConfig();

    // Carica un log iniziale dell'history
    _history.initialize( history_mode, history_len );

    _sensor_timer.setLoopTime( 8000 * 1000 );
    _last_time = FrameworkTimer::getTimeEpoc();
    updateCurrentTime();
    updateProgram();

    updateStatus( checkGas(),
                  checkPellet(),
                  checkPelletMinimum(),
                  pelletFeedback(),
                  _pellet_flameout);

    return !_gpio_error;
}

uint8_t RunnerThread::fixReading(const int read)
{
    if (read > 255 || read < 0)
        debugPrintError("WiringPi") << "Invalid data from wiringPi library\n";
    return (uint8_t)read;
}

void RunnerThread::updateCurrentTime()
{
    time_t now = (time_t)_last_time;
    struct tm *tm_struct = localtime(&now);
    _day = (6+tm_struct->tm_wday)%7;
    _hour = tm_struct->tm_hour;
    _half_hour = tm_struct->tm_min >= 30 ? 1 : 0;
    _str_day = FrameworkUtils::tostring( _day );
    _str_h = FrameworkUtils::tostring( _hour );
    _str_f = FrameworkUtils::tostring( _half_hour );
}

void RunnerThread::updateProgram()
{
    _program.getProgram( _day, _hour, _half_hour,
                         _program_gas,
                         _program_pellet,
                         _program_pellet_minimum );
}

void RunnerThread::appendMessage(const std::string &msg)
{
    time_t t = FrameworkTimer::getTimeEpoc();
    struct tm tm = *localtime(&t);
    std::string stamp = (tm.tm_mday < 10 ? "0" : "" ) + FrameworkUtils::tostring( tm.tm_mday ) + "/" +
            (tm.tm_mon < 10 ? "0" : "" ) +FrameworkUtils::tostring( tm.tm_mon+1 ) + "/" +
            FrameworkUtils::tostring( tm.tm_year+1900 ) + " " +
            (tm.tm_hour < 10 ? "0" : "" ) + FrameworkUtils::tostring( tm.tm_hour ) + ":" +
            (tm.tm_min < 10 ? "0" : "" ) + FrameworkUtils::tostring( tm.tm_min ) + "." +
            (tm.tm_sec < 10 ? "0" : "" ) + FrameworkUtils::tostring( tm.tm_sec );

    _messages.push_back( stamp + " - " + msg );
    while ( _messages.size() > _num_warnings )
        _messages.pop_front();
}

void RunnerThread::saveConfig()
{
    std::string content;
    FrameworkUtils::file_to_str( _config_file, content );
    ConfigFile config("config", content );
    config.setValue( "mode", _manual_mode ? "manual" : "auto" );
    config.setValue( "min_temp", _str_min_t );
    config.setValue( "max_temp", _str_max_t );
    config.setValue( "temp_correction", FrameworkUtils::ftostring( _temp_correction ) );
    config.setValue("history_mode", _history.getMode() );
    config.setValue("history_len", FrameworkUtils::tostring( _history.getLen() ) );
    config.setValue("num_warnings", FrameworkUtils::tostring( _num_warnings ) );
    config.setValueBool( "debug", _logger->getDebug() );

    ConfigData* prog_section = config.getSection( "program" );
    if ( prog_section == NULL )
        prog_section = config.newSection( "program" );
    _program.saveConfig( prog_section );
    if ( !FrameworkUtils::str_to_file( _config_file, config.toStr() ) )
        debugPrintError() << "Unable to save file " << _config_file << "\n";
}

void RunnerThread::updateStatus( bool gas_on, bool pellet_on, bool pellet_minimum, bool pellet_feedback, bool pellet_flameout )
{
    FILE* status_file = fopen( (_exchange_path+"/_status").c_str(), "w" );
    if ( status_file )
    {
        for ( int i = 0; i < _status_json_template.size(); ++i )
        {
            fwrite( _status_json_template[i].c_str(),
                    _status_json_template[i].length(), 1, status_file);
            switch (i)
            {
            case 0:
                if ( _manual_mode )
                    fwrite( _str_manual.c_str(), _str_manual.length(), 1, status_file);
                else
                    fwrite( _str_auto.c_str(), _str_auto.length(), 1, status_file);
                break;
            case 1:
                if ( _anti_ice_active )
                    fwrite( _str_on.c_str(), _str_on.length(), 1, status_file);
                else
                    fwrite( _str_off.c_str(), _str_off.length(), 1, status_file);
                break;
            case 2:
                if ( pellet_on && !_program_pellet )
                    fwrite( _str_pellet_off_warning.c_str(), _str_pellet_off_warning.size(), 1, status_file);
                else if ( !pellet_on && _program_pellet )
                    fwrite( _str_pellet_on_warning.c_str(), _str_pellet_on_warning.size(), 1, status_file);
                break;
            case 3:
                for ( std::list<std::string>::iterator m = _messages.begin();
                      m != _messages.end(); ++m )
                {
                    std::string msg = *m;
                    if ( m != _messages.begin() )
                        fwrite(",", 1, 1, status_file );
                    fwrite("\"", 1, 1, status_file );
                    fwrite( msg.c_str(), msg.length(), 1, status_file );
                    fwrite("\"", 1, 1, status_file );
                }
                break;
            case 4:
                if ( pellet_on )
                    fwrite( _str_on.c_str(), _str_on.length(), 1, status_file);
                else
                    fwrite( _str_off.c_str(), _str_off.length(), 1, status_file);
                break;
            case 5:
                if ( pellet_feedback )
                    fwrite( _str_on.c_str(), _str_on.length(), 1, status_file);
                else
                    fwrite( _str_off.c_str(), _str_off.length(), 1, status_file);
                break;
            case 6:
                if ( pellet_minimum )
                    fwrite( _str_on.c_str(), _str_on.length(), 1, status_file);
                else
                    fwrite( _str_off.c_str(), _str_off.length(), 1, status_file);
                break;
            case 7:
                if ( pellet_flameout )
                    fwrite( _str_on.c_str(), _str_on.length(), 1, status_file);
                else
                    fwrite( _str_off.c_str(), _str_off.length(), 1, status_file);
                break;
            case 8:
                if ( gas_on )
                    fwrite( _str_on.c_str(), _str_on.length(), 1, status_file);
                else
                    fwrite( _str_off.c_str(), _str_off.length(), 1, status_file);
                break;
            case 9:
                fwrite( _str_min_t.c_str(), _str_min_t.length(), 1, status_file);
                break;
            case 10:
                fwrite( _str_max_t.c_str(), _str_max_t.length(), 1, status_file);
                break;
            case 11:
                fwrite( _str_day.c_str(), _str_day.length(), 1, status_file);
                break;
            case 12:
                fwrite( _str_h.c_str(), _str_h.length(), 1, status_file);
                break;
            case 13:
                fwrite( _str_f.c_str(), _str_f.length(), 1, status_file);
                break;
            case 14:
                break;
            }
        }

        _program.writeJSON( status_file );

        fwrite( "}", 1, 1, status_file );
        fclose( status_file );
    }
}

bool RunnerThread::readSensor( float & current_temp, float & current_humidity )
{
    bool ret = false;
#ifndef NOPI
    int pin = 1;
    uint8_t laststate = HIGH;
    uint8_t counter = 0;
    int dht22_dat[5] = {0,0,0,0,0};
    uint8_t j = 0, i;

    // pull pin down for 18 milliseconds
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
    delay(18);
    // then pull it up for 40 microseconds
    digitalWrite(pin, HIGH);
    delayMicroseconds(40);
    // prepare to read the pin
    pinMode(pin, INPUT);

    // detect change and read data
    for ( i=0; i< 85; i++)
    {
        counter = 0;
        while (fixReading(digitalRead(pin)) == laststate)
        {
            counter++;
            delayMicroseconds(1);
            if (counter == 255)
                break;
        }
        laststate = fixReading(digitalRead(pin));
        if (counter == 255)
            break;
        // ignore first 3 transitions
        if ((i >= 4) && (i%2 == 0)) {
            // shove each bit into the storage bytes
            dht22_dat[j/8] <<= 1;
            if (counter > 16)
                dht22_dat[j/8] |= 1;
            j++;
        }
    }

    // check we read 40 bits (8bit x 5 ) + verify checksum in the last byte
    // print it out if data is good
    if ((j >= 40) &&
            (dht22_dat[4] == ((dht22_dat[0] + dht22_dat[1] + dht22_dat[2] + dht22_dat[3]) & 0xFF)) )
    {
        float new_humidity = (dht22_dat[0] * 256 + dht22_dat[1]) / 10.0;
        float new_temp = (((dht22_dat[2] & 0x7F)* 256 + dht22_dat[3]) / 10.0) + _temp_correction;
        /* negative temp */
        if ((dht22_dat[2] & 0x80) != 0)
            new_temp = -new_temp;

        if ( (_sensor_success_reads == 0) || // prima lettura sempre buona!
             ( (new_humidity > 0.0) &&    // dato valido?
               (new_humidity < 95.0) &&   // dato valido?
               (fabs(current_humidity-new_humidity)<10.0) // delta ragionevole?
               ) )
        {
            current_humidity = new_humidity;
            ret = true;
        }
        if ( (_sensor_success_reads == 0) || // prima lettura sempre buona!
             ( ( new_temp > -60.0 ) && // dato valido?
               ( new_temp < 60.0 ) &&  // dato valido?
               (fabs(current_temp-new_temp)<2.0) // delta ragionevole?
               ) )
        {
            current_temp = new_temp;
            ret = true;
        }

        // Skip fake readings (will turn on anti-ice):
        if ( (current_humidity == 0) && (current_temp == 0) && ret )
            ret = false;

        _sensor_success_reads++;
    }
    else
        _sensor_failed_reads++;
#else
    float new_humidity = current_humidity+1.0;
    float new_temp = current_temp+1;
    if ( new_humidity > 95.0 )
        new_humidity = 40;
    if ( new_temp > 30 )
        new_temp = 1;
    current_humidity = new_humidity;
    current_temp = new_temp;
    ret = true;
    _sensor_success_reads++;
#endif
    _logger->logDebug("Sensor reads: " + FrameworkUtils::tostring( _sensor_success_reads ) + " ok - " + FrameworkUtils::tostring( _sensor_failed_reads ) + " failed");
    return ret;
}

void RunnerThread::setGpioBool(uint8_t num, bool activate)
{
#ifndef NOPI
    pinMode( num, OUTPUT);
    digitalWrite( num, activate ? HIGH : LOW );
#endif
}

bool RunnerThread::readGpioBool(uint8_t num, int mode )
{
#ifdef NOPI
    return false;
#else
    pinMode( num, mode);
    uint8_t pin = digitalRead( num );
    return pin == HIGH;
#endif
}
