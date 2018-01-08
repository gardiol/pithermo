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

#include <wiringPi.h>

// GPIO
// 5 = minimo
// 4 = ???
// 6 = quarto relé libero
// 0 = gas
// 2 = pellet
// 3 = ???
RunnerThread::RunnerThread(const std::string &cfg,
                           const std::string &exchange_path,
                           const std::string &hst):
    ScheduledThread("Runner", 10 * 1000 ),
    _config_file( cfg ),
    _exchange_path( exchange_path ),
    _history_file( hst ),
    _commands_mutex("CommandsMutex"),
    _pellet_on(false),
    _pellet_minimum(false),
    _pellet_feedback(false),
    _gas_on(false),
    _manual_mode(true),
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
    _gpio_error(false),
    _history_warned(false),
    _print_sensor(true),
    _str_manual("manual"),
    _str_auto("auto"),
    _str_on("on"),
    _str_off("off"),
    _str_min_t(FrameworkUtils::ftostring( _min_temp ) ),
    _str_max_t(FrameworkUtils::ftostring( _max_temp ) ),
    _str_day("0"),
    _str_h("00"),
    _str_f("0")
{
    _status_json_template.push_back("{\"mode\":\"");
    _status_json_template.push_back("\",\"pellet\":{\"command\":\"");
    _status_json_template.push_back("\",\"status\":\"");
    _status_json_template.push_back("\",\"minimum\":\"");
    _status_json_template.push_back("\"},\"gas\":{\"command\":\"");
    _status_json_template.push_back("\",\"status\":\"off\"},\"temp\":{\"min\":");
    _status_json_template.push_back(",\"max\":");
    _status_json_template.push_back("},\"now\":{\"d\":");
    _status_json_template.push_back(",\"h\":");
    _status_json_template.push_back(",\"f\":");
    _status_json_template.push_back("},\"program\":");

    if (wiringPiSetup () != -1)
    {
        startThread();
        while ( !_gpio_error && !isRunning() )
            FrameworkTimer::msleep_s( 100 );
    }
    else
        debugPrintError( "WiringPi setup" ) << "WiringPi error!\n";
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

    uint64_t last_time = 0;
    float current_temp = 0.0;
    float current_humidity = 0.0;

    _commands_mutex.lock();
    while ( _commands_list.size() > 0 )
    {
        Command* cmd = _commands_list.front();
        _commands_list.pop_front();
        switch ( cmd->command() )
        {
        case Command::PELLET_MINIMUM_ON:
            if ( !_pellet_minimum  && _manual_mode )
            {
                _pellet_minimum = true;
                setGpioBool( _pellet_minimum_gpio, !_pellet_minimum );
                update_status = true;
            }
            break;

        case Command::PELLET_MINIMUM_OFF:
            if ( _pellet_minimum && _manual_mode )
            {
                _pellet_minimum = false;
                setGpioBool( _pellet_minimum_gpio, !_pellet_minimum );
                update_status = true;
            }
            break;

        case Command::PELLET_ON:
            if ( !_pellet_on  && _manual_mode )
            {
                _pellet_on = true;
                setGpioBool( _pellet_command_gpio, !_pellet_on );
                update_status = true;
            }
            break;

        case Command::PELLET_OFF:
            if ( _pellet_on && _manual_mode )
            {
                _pellet_on = false;
                setGpioBool( _pellet_command_gpio, !_pellet_on );
                update_status = true;
            }
            break;

        case Command::GAS_ON:
            if ( !_gas_on && _manual_mode )
            {
                _gas_on = true;
                setGpioBool( _gas_command_gpio, !_gas_on );
                update_status = true;
            }
            break;

        case Command::GAS_OFF:
            if ( _gas_on && _manual_mode )
            {
                _gas_on = false;
                setGpioBool( _gas_command_gpio, !_gas_on );
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
    if ( (last_time+60) <= current_time )
    {
        last_time = current_time;
        time_t now = (time_t)last_time;
        struct tm *tm_struct = localtime(&now);
        _str_f = tm_struct->tm_min > 30 ? "1" : "0";
        _str_day = FrameworkUtils::tostring( (tm_struct->tm_wday-1)%7 );
        _str_h = FrameworkUtils::tostring( tm_struct->tm_hour );
        update_status = true;
        update_history = true;
    }

    if ( !_manual_mode )
    {
        ;
    }

    if ( save_config )
        saveConfig();
    if ( update_status )
        updateStatus();
    if ( update_history )
        updateHistory( current_temp, _current_humidity, current_time );

    if ( !_sensor_timer.isRunning() || _sensor_timer.elapsedLoop() )
    {
        readSensor();
        _sensor_timer.reset();
    }

    return ret;
}

bool RunnerThread::scheduleStart()
{
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
        _temp_correction = FrameworkUtils::string_tof( config.getValue( "temp_correction" ) );
        _print_sensor = config.getValueBool( "print_sensor" );
        // Questi sono letti ma poi ignorati volutamente
        _pellet_on = config.getValueBool( "pellet_on" );
        _pellet_minimum = config.getValueBool( "pellet_minimum" );
        _gas_on = config.getValueBool( "gas_on" );
        _program.loadConfig( config.getSection( "program" ) );
    }
    else
        saveConfig();
    // Per evitare accensioni non volute o spegnimenti accidentali,
    // leggiamo lo stato dei relé e usiamo questi come dato di partenza
    _pellet_on = !readGpioBool( _pellet_command_gpio );
    _pellet_minimum = !readGpioBool( _pellet_minimum_gpio );
    _pellet_feedback = false; //!readGpioBool( _pellet_feedback_gpio );
    _gas_on = !readGpioBool( _gas_command_gpio );

    _sensor_timer.setLoopTime( 8000 * 1000 );

    return !_gpio_error;
}

void RunnerThread::schedulingStopped()
{
}

uint8_t RunnerThread::fixReading(const int read)
{
    if (read > 255 || read < 0)
        debugPrintError("WiringPi") << "Invalid data from wiringPi library\n";
    return (uint8_t)read;
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
    config.setValueBool( "print_sensor", _print_sensor );
    config.setValueBool( "pellet_on", _pellet_on );
    config.setValueBool( "pellet_minimum", _pellet_minimum );
    config.setValueBool( "gas_on", _gas_on );

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
                if ( _pellet_on )
                    fwrite( _str_on.c_str(), _str_on.length(), 1, status_file);
                else
                    fwrite( _str_off.c_str(), _str_off.length(), 1, status_file);
                break;
            case 2:
                if ( _pellet_feedback )
                    fwrite( _str_on.c_str(), _str_on.length(), 1, status_file);
                else
                    fwrite( _str_off.c_str(), _str_off.length(), 1, status_file);
                break;
            case 3:
                if ( _pellet_minimum )
                    fwrite( _str_on.c_str(), _str_on.length(), 1, status_file);
                else
                    fwrite( _str_off.c_str(), _str_off.length(), 1, status_file);
                break;
            case 4:
                if ( _gas_on )
                    fwrite( _str_on.c_str(), _str_on.length(), 1, status_file);
                else
                    fwrite( _str_off.c_str(), _str_off.length(), 1, status_file);
                break;
            case 5:
                fwrite( _str_min_t.c_str(), _str_min_t.length(), 1, status_file);
                break;
            case 6:
                fwrite( _str_max_t.c_str(), _str_max_t.length(), 1, status_file);
                break;
            case 7:
                fwrite( _str_day.c_str(), _str_day.length(), 1, status_file);
                break;
            case 8:
                fwrite( _str_h.c_str(), _str_h.length(), 1, status_file);
                break;
            case 9:
                fwrite( _str_f.c_str(), _str_f.length(), 1, status_file);
                break;
            case 10:
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
    while ( _th_history.size() > 100 )
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

    FILE* history_json = fopen( (_exchange_path+"/_history").c_str(), "w" );
    if ( history_json )
    {
        fwrite( "[", 1, 1, history_json );
        for ( std::list<HistoryItem>::iterator t = _th_history.begin(); t != _th_history.end(); ++t )
        {
            float temp = (*t).getTemp();
            float humidity = (*t).getHumidity();
            uint64_t ltime = (*t).getTime();
            if ( t != _th_history.begin() )
                fwrite( ",", 1, 1, history_json );
            std::string temp_str = FrameworkUtils::ftostring( temp );
            std::string humidity_str = FrameworkUtils::ftostring( humidity );
            std::string time_str = FrameworkUtils::tostring( ltime );
            fwrite("{time:", 6, 1, history_json );
            fwrite( time_str.c_str(), time_str.length(), 1, history_json );
            fwrite(",temp:", 6, 1, history_json );
            fwrite( temp_str.c_str(), temp_str.length(), 1, history_json );
            fwrite(",humidity:", 10, 1, history_json );
            fwrite( humidity_str.c_str(), humidity_str.length(), 1, history_json );
            fwrite("}", 1, 1, history_json );
        }
        fwrite( "]", 1, 1, history_json );
        fclose( history_json );
    }
}

bool RunnerThread::readSensor( float & current_temp, float & current_humidity )
{
    bool ret = false;
    int pin = 1;
    uint8_t laststate = HIGH;
    uint8_t counter = 0;
    uint8_t j = 0, i;
    int dht22_dat[5] = {0,0,0,0,0};

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
            current_humidity = new_humidity;
            ret = true;
        }
        if ( (_current_temp == 0.0) ||
             ( ( new_temp < 600 ) &&
               (fabs(abs(current_temp)-fabs(new_temp))<10.0) ) )
        {
            current_temp = new_temp;
            ret = true;
        }
        _sensor_success_reads++;
        if ( ret && _print_sensor )
            debugPrintNotice( "Sensor: " ) << "temp: " << _current_temp << " Humidity: " << _current_humidity << "(" << _sensor_success_reads << "/" << (_sensor_success_reads+_sensor_failed_reads) << ")\n";
    }
    else
        _sensor_failed_reads++;
}

void RunnerThread::setGpioBool(uint8_t num, bool activate)
{
    pinMode( num, OUTPUT);
    digitalWrite( num, activate ? HIGH : LOW );
}

bool RunnerThread::readGpioBool(uint8_t num)
{
    pinMode( num, OUTPUT);
    uint8_t pin = digitalRead( num );
    return pin == HIGH;
}
