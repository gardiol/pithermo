#include "logger.h"

#include <pithermotimer.h>
#include <pithermoutils.h>

#include <time.h>

Logger::Logger(const std::string &events_path):
    _log_filename(events_path),
    _debug_filename(events_path+"_debug.txt"),
    _debug(false),
    _valid(false)
{
    // Test log file is accessible/can be created
    FILE* f = fopen( _log_filename.c_str(), "ab" );
    if ( f != nullptr )
    {
        _valid = true;
        fclose(f);
    }
}

Logger::~Logger()
{
}


void Logger::logEvent(LogItem evt)
{
    FILE* f = fopen(_log_filename.c_str(), "ab" );
    if ( f != nullptr )
    {
        evt.write(f);
        fclose(f);
    }
}

void Logger::logDebug(const std::string &str)
{
    if ( _debug )
    {
        FILE* f = fopen(_debug_filename.c_str(), "a" );
        if ( f != nullptr )
        {
            _printStamp(f);
            fprintf(f, " %s\n", str.c_str() );
            fclose(f);
        }
    }
}

bool Logger::fetchInterval(uint64_t from, uint64_t to, std::list<LogItem> &items)
{
    bool a,b,c,d;
    return fetchInterval(from, to, items, false, a, b, c, d);
}

bool Logger::fetchInterval(uint64_t from, uint64_t to, std::list<LogItem> &items, bool search_prev,
                           bool &last_pellet_on,
                           bool &last_pellet_minimum_on,
                           bool &last_gas_on,
                           bool &last_pellet_flameout)
{
    bool ret = false;
    items.clear();
    FILE* read_file = fopen( _log_filename.c_str(), "rb" );
    if ( read_file != nullptr )
    {
        bool start_found = false;
        bool end_found = false;
        fseek( read_file, 0, SEEK_END );
        long int file_len = ftell(read_file);
        int64_t item_size = LogItem::getSize();
        int64_t total_items = file_len / item_size;
        LogItem item;

        int64_t left = 0;
        int64_t right = total_items;
        bool error = false;
        while ( !start_found && !error && (left != (right-1) ) )
        {
            int64_t start_item = (left+right)/2;
            long int cursor = start_item * item_size;
            if ( fseek( read_file, cursor, SEEK_SET ) != -1 )
            {
                item.read( read_file );
                if ( item.isValid() )
                {
                    uint64_t item_time = item.getTime();
                    // This case is actually VERY difficult to happen!
                    if ( item_time == from )
                        start_found = true;
                    else if ( item_time > from )
                        right = start_item;
                    else
                        left = start_item;
                }
                else
                    error = true;
            }
            else
                error = true;
        }

        if ( !error )
        {
            // We need to find the last unmatched "on" events, as requested:
            if ( search_prev )
            {
                last_gas_on = false;
                last_pellet_on = false;
                last_pellet_minimum_on = false;
                last_pellet_flameout = false;
                bool gas_ok = false;
                bool pellet_ok = false;
                bool pellet_minimum_ok = false;
                bool pellet_flameout_ok = false;
                // Searching backwards for an unmatched "on" event.
                // Stop as soon as we find the firt "off" event or "on" event
                // for each generator;
                long int start_pos = ftell( read_file );
                // Go back at least TWO items (one is the start item already read)
                long int pos = start_pos - item_size*2;
                LogItem old_item;
                while ( !error && (pos >= 0) && (!gas_ok || !pellet_ok || !pellet_minimum_ok || !pellet_flameout_ok) )
                {
                    if ( fseek(read_file, pos, SEEK_SET) != -1 )
                    {
                        old_item.read(read_file);
                        if ( old_item.isValid() )
                        {
                            uint64_t event = old_item.getEvent();
                            if ( !gas_ok )
                            {   // gas was on ONLY if last gas event was "ON"
                                last_gas_on = event == LogItem::GAS_ON;
                                // We stop checking for gas after we saw an ON or OFF event.
                                if ( (event == LogItem::GAS_ON) || (event == LogItem::GAS_OFF) )
                                    gas_ok = true;
                            }
                            if ( !pellet_minimum_ok )
                            {   // Pellet is at minimum only if last event is MINIMUM
                                last_pellet_minimum_on = event == LogItem::PELLET_MINIMUM;
                                // If we see a MINIMUM or MODULATION event we can stop checking pellet minimum.
                                if ( (event == LogItem::PELLET_MINIMUM) || (event == LogItem::PELLET_MODULATION ) )
                                    pellet_minimum_ok = true;
                            }
                            if ( !pellet_flameout_ok )
                            {
                                last_pellet_flameout = event == LogItem::PELLET_FLAMEOUT_ON;
                                if ( (event == LogItem::PELLET_FLAMEOUT_OFF) ||
                                     (event == LogItem::PELLET_FLAMEOUT_ON) ||
                                     (event == LogItem::PELLET_HOT ) )
                                    pellet_flameout_ok = true;
                            }
                            if ( !pellet_ok )
                            {
                                // Pellet is ON only if last event is PELLET_ON
                                last_pellet_on = event == LogItem::PELLET_ON;
                                // When we see a PELLET_ON or PELLET_OFF events, we are done with pellet
                                if ( (event == LogItem::PELLET_ON) || (event == LogItem::PELLET_OFF) )
                                    pellet_ok = true;
                            }
                            // Fetch previous item
                            pos -= item_size;
                        }
                        else
                            error = true;
                    }
                    else
                        error = true;
                }
                // Back to original position to read the rest of the file
                fseek(read_file, start_pos, SEEK_SET);
            }

            if ( !error )
            {
                // Let's read until last:
                while ( !end_found && item.isValid() && !feof(read_file) )
                {
                    if ( item.getTime() >= to )
                        end_found = true;
                    else
                        items.push_back( item );
                    item.read( read_file );
                }
                ret = true;
            }
        } // error looking for start!
        fclose(read_file);
    }
    return ret;
}

void Logger::_statsPelletOff( uint64_t& pellet_on_since,
                              uint32_t& pellet_on_time,
                              uint64_t& pellet_low_on_since,
                              uint32_t& pellet_low_time,
                              bool& pellet_was_on,
                              bool& pellet_on,
                              uint64_t event_time )
{
    if ( pellet_on_since > 0 )
    {
        pellet_on_time += event_time - pellet_on_since;
        pellet_on_since = 0;
    }
    if ( pellet_low_on_since > 0 ) // Where at minimum? Calculate the time too...
    {
        pellet_low_time += event_time - pellet_low_on_since;
        pellet_low_on_since = 0;
    }
    pellet_on = false;
}

bool Logger::calculateStats(uint64_t from, uint64_t to,
                            bool& on_are_valid,
                            bool& pellet_on,
                            bool& pellet_minimum_on,
                            bool& gas_on,
                            bool& pellet_flameout,
                            uint32_t &pellet_on_time,
                            uint32_t &pellet_low_time,
                            uint32_t &gas_on_time)
{
    bool ret = false;
    bool pellet_was_on = false;
    std::list<LogItem> items;
    uint64_t gas_on_since = 0;
    uint64_t pellet_on_since = 0;
    uint64_t pellet_low_on_since = 0;
    if ( !on_are_valid )
        ret = fetchInterval( from, to, items, true,
                             pellet_on, pellet_minimum_on, gas_on, pellet_flameout );
    else
        ret = fetchInterval( from, to, items );

    if ( pellet_on )
    {
        pellet_on_since = from;
        if ( pellet_minimum_on )
            pellet_low_on_since = from;
    }
    if ( gas_on )
        gas_on_since = from;

    if ( ret )
    {
        bool pellet_hot = false;
        // This is for the selected period.
        for ( std::list<LogItem>::iterator i = items.begin(); i != items.end(); ++i )
        {
            uint64_t event_time = (*i).getTime();
            switch ( (*i).getEvent() )
            {
            case  LogItem::GAS_ON:
                if ( (gas_on_since == 0) && !pellet_hot )
                    gas_on_since = event_time;
                gas_on = true;
                break;

            case LogItem::PELLET_MINIMUM:
                pellet_minimum_on = true;
                if ( !pellet_flameout )
                {
                    if ( ( pellet_on_since > 0 ) &&
                         ( pellet_low_on_since == 0 ) )
                        pellet_low_on_since = event_time;
                }
                break;

            case LogItem::PELLET_MODULATION:
                pellet_minimum_on = false;
                if ( !pellet_flameout )
                {
                    if ( pellet_low_on_since > 0 )
                    {
                        pellet_low_time += event_time - pellet_low_on_since;
                        pellet_low_on_since = 0;
                    }
                }
                break;

            case LogItem::PELLET_FLAMEOUT_OFF:
                pellet_flameout = false;
                if ( pellet_was_on )
                {
                    pellet_on_since = event_time;
                    // Pellet at minimum? Calculate the time for it too...
                    if ( pellet_minimum_on )
                        pellet_low_on_since = event_time;
                    pellet_on = true;
                }
                break;

            case  LogItem::PELLET_ON:
                // Under flameout conditions ignore start
                if ( !pellet_flameout )
                {
                    // Ignore subsequent pellet_on without an off
                    // Otherwise, times might be reset and some "on" time lost
                    if (pellet_on_since == 0)
                    {
                        pellet_on_since = event_time;
                        // Pellet at minimum? Calculate the time for it too...
                        // We ensure that the correct power timing is calculated
                        if ( pellet_minimum_on )
                            pellet_low_on_since = event_time;
                    }
                    pellet_on = true;
                }
                break;

            case LogItem::PELLET_FLAMEOUT_ON:
                pellet_was_on = pellet_on;
                pellet_flameout = true;
                // Stop calculating pellet stats now. There will be some discrepancy,
                // but we don't know exactly WHEN the flameout occurred.
                _statsPelletOff( pellet_on_since,
                                 pellet_on_time,
                                 pellet_low_on_since,
                                 pellet_low_time,
                                 pellet_was_on,
                                 pellet_on,
                                 event_time );
                break;

            case LogItem::PELLET_OFF:
                _statsPelletOff( pellet_on_since,
                                 pellet_on_time,
                                 pellet_low_on_since,
                                 pellet_low_time,
                                 pellet_was_on,
                                 pellet_on,
                                 event_time );
                pellet_was_on = false;
                break;

            case LogItem::PELLET_HOT:
                // If we where in flameout, this means pellet is back on now:
                if ( pellet_flameout )
                {
                    pellet_on_since = event_time;
                    // Pellet at minimum? Calculate the time for it too...
                    if ( pellet_minimum_on )
                        pellet_low_on_since = event_time;
                    pellet_on = true;
                    pellet_flameout = false;
                }
                pellet_hot = true;
                // PELLET HOT might indicate that GAS is off by hardware
            case LogItem::GAS_OFF:
                if ( gas_on_since > 0 )
                {
                    gas_on_time += event_time - gas_on_since;
                    gas_on_since = 0;
                }
                gas_on = false;
                break;

            case LogItem::PELLET_COLD:
                pellet_hot = false;
                break;
            }
        }
        if ( pellet_on )
        {
            pellet_on_time += to - pellet_on_since;
            if ( pellet_minimum_on )
                pellet_low_time += to - pellet_low_on_since;
        }
        if ( gas_on )
            gas_on_time += to - gas_on_since;
        on_are_valid = true;
        ret = true;
    }
    else
        on_are_valid = false;
    return ret;
}

void Logger::_printStamp(FILE *f)
{
    time_t now = static_cast<time_t>(PithermoTimer::getTimeEpoc());
    struct tm *tm_struct = localtime(&now);
    int y = tm_struct->tm_year + 1900;
    int M = tm_struct->tm_mon + 1;
    int d = tm_struct->tm_mday;
    int h = tm_struct->tm_hour;
    int m = tm_struct->tm_min;
    int s = tm_struct->tm_sec;
    fprintf(f, "%04d/%02d/%02d-%02d:%02d.%02d", y, M, d, h, m, s );
}
