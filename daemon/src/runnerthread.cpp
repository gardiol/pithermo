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
#endif

// GPIO
// 5 = minimo
// 4 = ???
// 6 = quarto relé libero
// 0 = gas
// 2 = pellet
// 3 = ???
RunnerThread::RunnerThread(const std::string &cfg,
                           const std::string &exchange_path,
                           const std::string &hst, Logger *l):
    ScheduledThread("Runner", 10 * 1000 ),
    _logger(l),
    _config_file( cfg ),
    _exchange_path( exchange_path ),
    _history_file( hst ),
    _commands_mutex("CommandsMutex"),
    _manual_mode(true),
    _anti_ice_active(false),
    _min_temp(16.0),
    _max_temp(17.0),
    _temp_correction(1.0),
    _sensor_failed_reads(0),
    _sensor_success_reads(0),
    _sensor_timer(),
    _pellet_command_gpio(2),
    _pellet_minimum_gpio(5),
    _pellet_feedback_gpio(),
    _gas_command_gpio(0),
    _sensor_gpio(),
    _last_time(0),
    _current_temp(0.0),
    _current_humidity(50.0),
    _program_gas(false),
    _program_pellet(false),
    _program_pellet_minimum(false),
    _gpio_error(false),
    _history_warned(false),
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
    _status_json_template.push_back("\"},\"pellet\":{\"command\":\"");
    _status_json_template.push_back("\",\"status\":\"");
    _status_json_template.push_back("\",\"minimum\":\"");
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
    bool x = !readGpioBool( _gas_command_gpio );
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
    bool x = !readGpioBool( _pellet_command_gpio );
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
    bool x = !readGpioBool( _pellet_minimum_gpio );
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
    return !readGpioBool( _pellet_feedback_gpio );
}

bool RunnerThread::scheduledRun(uint64_t elapsed_time_us, uint64_t cycle)
{
    bool update_status = false;
    bool check_program = false;
    bool save_config = false;
    bool ret = true;

    // Processa comandi
    _commands_mutex.lock();
    while ( _commands_list.size() > 0 )
    {
        Command* cmd = _commands_list.front();
        _commands_list.pop_front();
        switch ( cmd->command() )
        {
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
            _logger->logDebug("Invalid command received");
            break;
        }
    }
    _commands_mutex.unlock();

    // Aggiorna tempo, history e programma
    uint64_t current_time = FrameworkTimer::getTimeEpoc();
    if ( (_last_time+60) <= current_time )
    {
        _logger->logDebug("Time interval (" + FrameworkUtils::tostring(current_time) +" - " + FrameworkUtils::tostring(_last_time) + " >= 60)");
        _last_time = current_time;
        updateCurrentTime();
        updateProgram();
        writeHistoryJson();
        update_status = true;
        if ( !_manual_mode )
            check_program = true;
    }

    if ( check_program )
    {
        _logger->logDebug("(in auto mode) Check program...");
        _logger->logDebug( std::string("program gas ") + (_program_gas ? "on" : "off"));
        _logger->logDebug( std::string("program pellet ") + (_program_pellet ? "on" : "off"));
        _logger->logDebug( std::string("program pellet minimum ") + (_program_pellet_minimum ? "on" : "off"));
        if ( checkGas() )
        {
            if ( !_program_gas )
                gasOff();
        }
        else
        {
            if ( _program_gas )
                gasOn();
        }
        if ( checkPellet() )
        {
            if ( !_program_pellet )
                pelletOff();
        }
        else
        {
            if ( _program_pellet )
                pelletOn();
        }
        if ( checkPelletMinimum() )
        {
            if ( !_program_pellet_minimum )
                pelletMinimum(true);
        }
        else
        {
            if ( _program_pellet_minimum )
                pelletMinimum(false);
        }
    }

    // Applica i constraint di temperatura
    if ( !_sensor_timer.isRunning() || _sensor_timer.elapsedLoop() )
    {
        if ( readSensor( _current_temp, _current_humidity ) )
        {
            _logger->logTemp( _current_temp, _current_humidity );
            updateHistory( _current_temp, _current_humidity, _last_time );
            if ( _current_temp <= 5.0 )
            {
                if ( !_anti_ice_active )
                {
                    _logger->logEvent("Anti-ice ON");
                    gasOn();
                    update_status = true;
                    _anti_ice_active = true;
                }
            }
            else if ( _anti_ice_active )
            {
                _logger->logEvent("Anti-ice OFF");
                gasOff();
                update_status = true;
                _anti_ice_active = false;
            }
            else if ( _current_temp > _max_temp )
            {
                if ( _manual_mode )
                {
                    _logger->logEvent("Max temp reached - manual mode");
                    if ( checkGas() )
                        gasOff();
                    if ( checkPellet() )
                        pelletMinimum(true);
                    update_status = true;
                }
                else
                {
                    _logger->logEvent("Max temp reached - program mode");
                    if ( _program_gas )
                        gasOff();
                    if ( _program_pellet )
                        pelletMinimum(true);
                }
            }
            else if ( _current_temp < _min_temp )
            {
                if ( _manual_mode )
                {
                    _logger->logEvent("Min temp reached - manual mode");
                    if ( checkPellet() )
                        pelletMinimum( false );
                    else
                        gasOn();
                }
                else
                {
                    _logger->logEvent("Min temp reached - program mode");
                    if ( _program_gas )
                        gasOn();
                    if ( _program_pellet )
                    {
                        pelletOn();
                        if ( _program_pellet_minimum )
                            pelletMinimum(true);
                        else
                            pelletMinimum(false);
                    }
                }
            }
        }
        _sensor_timer.reset();
    }

    if ( save_config )
        saveConfig();
    if ( update_status )
        updateStatus( checkGas(),
                      checkPellet(),
                      checkPelletMinimum(),
                      pelletFeedback() );

    return ret;
}

bool RunnerThread::scheduleStart()
{
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
        _program.loadConfig( config.getSection( "program" ) );
    }
    else
        saveConfig();

    // Carica un log iniziale dell'history
    FILE* history_file = fopen( _history_file.c_str(), "r" );
    if ( history_file != NULL )
    {
        fseek( history_file, 0, SEEK_END );
        uint32_t len = ftell(history_file);
        uint32_t size = HistoryItem::getSize() * default_history_items;
        if ( size > len )
            fseek( history_file, 0, SEEK_SET);
        else
            fseek( history_file, len - size, SEEK_SET);
        for (uint32_t i = 0; !feof(history_file); ++i )
        {
            HistoryItem item(history_file);
            if ( item.isValid() )
                _th_history.push_back( item );
        }
        fclose(history_file);
    }

    _sensor_timer.setLoopTime( 8000 * 1000 );
    _last_time = FrameworkTimer::getTimeEpoc();
    updateCurrentTime();
    updateProgram();

    writeHistoryJson();
    updateStatus( checkGas(),
                  checkPellet(),
                  checkPelletMinimum(),
                  pelletFeedback() );

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
    _day = (tm_struct->tm_wday-1)%7;
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

void RunnerThread::saveConfig()
{
    std::string content;
    FrameworkUtils::file_to_str( _config_file, content );
    ConfigFile config("config", content );
    config.setValue( "mode", _manual_mode ? "manual" : "auto" );
    config.setValue( "min_temp", _str_min_t );
    config.setValue( "max_temp", _str_max_t );
    config.setValue( "temp_correction", FrameworkUtils::ftostring( _temp_correction ) );
    config.setValueBool( "debug", _logger->getDebug() );

    ConfigData* prog_section = config.getSection( "program" );
    if ( prog_section == NULL )
        prog_section = config.newSection( "program" );
    _program.saveConfig( prog_section );
    if ( !FrameworkUtils::str_to_file( _config_file, config.toStr() ) )
        debugPrintError() << "Unable to save file " << _config_file << "\n";
}

void RunnerThread::updateStatus( bool gas_on, bool pellet_on, bool pellet_minimum, bool pellet_feedback )
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
                if ( pellet_on )
                    fwrite( _str_on.c_str(), _str_on.length(), 1, status_file);
                else
                    fwrite( _str_off.c_str(), _str_off.length(), 1, status_file);
                break;
            case 4:
                if ( pellet_feedback )
                    fwrite( _str_on.c_str(), _str_on.length(), 1, status_file);
                else
                    fwrite( _str_off.c_str(), _str_off.length(), 1, status_file);
                break;
            case 5:
                if ( pellet_minimum )
                    fwrite( _str_on.c_str(), _str_on.length(), 1, status_file);
                else
                    fwrite( _str_off.c_str(), _str_off.length(), 1, status_file);
                break;
            case 6:
                if ( gas_on )
                    fwrite( _str_on.c_str(), _str_on.length(), 1, status_file);
                else
                    fwrite( _str_off.c_str(), _str_off.length(), 1, status_file);
                break;
            case 7:
                fwrite( _str_min_t.c_str(), _str_min_t.length(), 1, status_file);
                break;
            case 8:
                fwrite( _str_max_t.c_str(), _str_max_t.length(), 1, status_file);
                break;
            case 9:
                fwrite( _str_day.c_str(), _str_day.length(), 1, status_file);
                break;
            case 10:
                fwrite( _str_h.c_str(), _str_h.length(), 1, status_file);
                break;
            case 11:
                fwrite( _str_f.c_str(), _str_f.length(), 1, status_file);
                break;
            case 12:
                break;
            }
        }

        _program.writeJSON( status_file );

        fwrite( "}", 1, 1, status_file );
        fclose( status_file );
    }
}

void RunnerThread::updateHistory( float last_temp, float last_humidity, uint32_t last_time )
{
    _th_history.push_back( HistoryItem( last_time, last_temp, last_humidity ) );
    while ( _th_history.size() > default_history_items )
        _th_history.pop_front();

    FILE* history_file = fopen( _history_file.c_str(), "a" );
    if ( history_file != NULL )
    {
        _th_history.back().writeToFile( history_file );
        fclose(history_file);
    }
    else if ( !_history_warned )
    {
        _history_warned = true;
        debugPrintWarning() << "Warning: unable to open history file (" << _history_file << ")!\n";
    }
}

void RunnerThread::writeHistoryJson()
{
    FILE* history_json = fopen( (_exchange_path+"/_history").c_str(), "w" );
    if ( history_json )
    {
        fwrite("{\"temp\":[", 9, 1, history_json );
        for ( std::list<HistoryItem>::iterator t = _th_history.begin(); t != _th_history.end(); ++t )
        {
            std::string temp_str = (*t).getTempStr();
            std::string time_str = (*t).getTimeStr();
            if ( t != _th_history.begin() )
                fwrite( ",", 1, 1, history_json );
            fwrite( "{\"x\":", 5, 1, history_json );
            fwrite( time_str.c_str(), time_str.length(), 1, history_json );
            fwrite( ",\"y\":", 5, 1, history_json );
            fwrite( temp_str.c_str(), temp_str.length(), 1, history_json );
            fwrite( "}", 1, 1, history_json );
        }

        fwrite( "],\"humidity\":[", 14, 1, history_json );
        for ( std::list<HistoryItem>::iterator t = _th_history.begin(); t != _th_history.end(); ++t )
        {
            std::string humidity_str = (*t).getHumidityStr();
            std::string time_str = (*t).getTimeStr();
            if ( t != _th_history.begin() )
                fwrite( ",", 1, 1, history_json );
            fwrite( "{\"x\":", 5, 1, history_json );
            fwrite( time_str.c_str(), time_str.length(), 1, history_json );
            fwrite( ",\"y\":", 5, 1, history_json );
            fwrite( humidity_str.c_str(), humidity_str.length(), 1, history_json );
            fwrite( "}", 1, 1, history_json );
        }
        fwrite( "]}", 2, 1, history_json );
        fclose( history_json );
    }
}

bool RunnerThread::readSensor( float & current_temp, float & current_humidity )
{
    bool ret = false;
#ifdef NOPI
    float new_humidity = current_humidity+1.0;
    float new_temp = current_temp+1;
    if ( new_humidity > 95.0 )
        new_humidity = 40;
    if ( new_temp > 30 )
        new_temp = 1;
    if ( true )
    {
#else
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

        if ( (new_humidity > 0) && (new_humidity < 950)  &&
             (fabs(current_humidity-new_humidity)<10.0) )
        {
#endif
            current_humidity = new_humidity;
#ifndef NOPI
            ret = true;
        }
        if ( (current_temp == 0.0) ||
             ( ( new_temp < 600 ) &&
               (fabs(abs(current_temp)-fabs(new_temp))<10.0) ) )
        {
#endif
            current_temp = new_temp;
            ret = true;
#ifndef NOPI
        }
#endif
        _sensor_success_reads++;
    }
    else
        _sensor_failed_reads++;
    _logger->logDebug("Sensor reads: " + FrameworkUtils::tostring( _sensor_success_reads ) + " ok - " + FrameworkUtils::tostring( _sensor_failed_reads ) + " failed");
}

void RunnerThread::setGpioBool(uint8_t num, bool activate)
{
#ifndef NOPI
    pinMode( num, OUTPUT);
    digitalWrite( num, activate ? HIGH : LOW );
#endif
}

bool RunnerThread::readGpioBool(uint8_t num)
{
#ifdef NOPI
    return false;
#else
    pinMode( num, OUTPUT);
    uint8_t pin = digitalRead( num );
    return pin == HIGH;
#endif
}
