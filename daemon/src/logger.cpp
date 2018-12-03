#include "logger.h"

#include <string.h>

#include <frameworktimer.h>
#include <frameworkutils.h>

using namespace FrameworkLibrary;

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
            fprintf(f, "%s\n", str.c_str() );
            fclose(f);
        }
    }
}

bool Logger::fetchInterval(uint64_t from, uint64_t to, std::list<LogItem> &items)
{
    bool a,b,c;
    return fetchInterval(from, to, items, false, a, b, c);
}

bool Logger::fetchInterval(uint64_t from, uint64_t to, std::list<LogItem> &items, bool search_prev,
                           bool &last_pellet_on, bool &last_pellet_minimum_on, bool &last_gas_on)
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
                bool gas_ok = false;
                bool pellet_ok = false;
                bool pellet_minimum_ok = false;
                // Searching backwards for an unmatched "on" event.
                // Stop as soon as we find the firt "off" event or "on" event
                // for each generator;
                long int start_pos = ftell( read_file );
                // Go back at least TWO items (one is the start item already read)
                long int pos = start_pos - item_size*2;
                LogItem old_item;
                while ( !error && (pos >= 0) && (!gas_ok || !pellet_ok || !pellet_minimum_ok) )
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

bool Logger::calculateStats(uint64_t from, uint64_t to,
                            bool& prev_valid,
                            bool& prev_pellet_on,
                            bool& prev_pellet_minimum_on,
                            bool& prev_gas_on,
                            uint32_t &pellet_on_time,
                            uint32_t &pellet_low_time,
                            uint32_t &gas_on_time)
{
    bool ret = false;    
    std::list<LogItem> items;
    uint64_t gas_on_since = 0;
    uint64_t pellet_on_since = 0;
    uint64_t pellet_low_on_since = 0;    
    if ( !prev_valid )
        ret = fetchInterval( from, to, items, true,
                             prev_pellet_on, prev_pellet_minimum_on, prev_gas_on );
    else
        ret = fetchInterval( from, to, items );

    if ( prev_pellet_on )
        pellet_on_since = from;
    if ( prev_pellet_minimum_on )
        pellet_low_on_since = from;
    if ( prev_gas_on )
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
                break;

            case LogItem::PELLET_MINIMUM:
                if ( ( pellet_on_since > 0 ) &&
                     ( pellet_low_on_since == 0 ) )
                        pellet_low_on_since = event_time;
                break;

            case LogItem::PELLET_MODULATION:
                if ( pellet_low_on_since > 0 )
                {
                    pellet_low_time += event_time - pellet_low_on_since;
                    pellet_low_on_since = 0;
                }
                break;

            case  LogItem::PELLET_ON:
                if ( pellet_on_since == 0 )
                {
                    pellet_on_since = event_time;
                    if ( pellet_low_on_since > 0 )
                        pellet_low_on_since = event_time;
                }
                break;

            case LogItem::PELLET_OFF:
                if ( pellet_on_since > 0 )
                {
                    pellet_on_time += event_time - pellet_on_since;
                    pellet_on_since = 0;
                }
                if ( pellet_low_on_since > 0 )
                {
                    pellet_low_time += event_time - pellet_low_on_since;
                    pellet_low_on_since = 0;
                }
                break;

            case LogItem::PELLET_HOT:
                // PELLET HOT might indicate that GAS is off by hardware
                pellet_hot = true;
            [[fallthrough]];
            case LogItem::GAS_OFF:
                if ( gas_on_since > 0 )
                {
                    gas_on_time += event_time - gas_on_since;
                    gas_on_since = 0;
                }
                break;

            case LogItem::PELLET_COLD:
                pellet_hot = false;
                break;
            }
        }
        if ( pellet_on_since > 0 )
        {
            pellet_on_time += to - pellet_on_since;
            prev_pellet_on = true;
        }
        else
            prev_pellet_on = false;
        if ( pellet_low_on_since > 0 )
        {
            pellet_low_time += to - pellet_low_on_since;
            prev_pellet_minimum_on = true;
        }
        else
            prev_pellet_minimum_on = false;
        if ( gas_on_since > 0 )
        {
            gas_on_time += to - gas_on_since;
            prev_gas_on = true;
        }
        else
            prev_gas_on = false;
        prev_valid = true;
        ret = true;
    }
    else
        prev_valid = false;
    return ret;
}

/*
void Logger::_calculateSeason(uint64_t &start, uint64_t &end)
{
    time_t now = static_cast<time_t>(FrameworkTimer::getTimeEpoc());
    struct tm *tm_struct = localtime(&now);
    int today_year = tm_struct->tm_year + 1900;
    // We divide the season at 15 august of each year.
    // If today is BEFORE this date, we are in the "first" season of the year,
    // otherwise we are in the "second" season of the year.
    int today_month = tm_struct->tm_mon + 1;
    int today_day = tm_struct->tm_mday;

    bool first_season = (today_month < 8) ||
                        ( (today_month == 8) && (today_day < 15 ) );

    struct tm season_divider;
    memset( &season_divider, 0, sizeof(tm) );
    season_divider.tm_mon = 8 - 1; // august
    season_divider.tm_mday = 15; // fifteen
    season_divider.tm_hour = 0;
    season_divider.tm_min = 0;
    season_divider.tm_sec = 0;
    season_divider.tm_isdst = -1;

    season_divider.tm_year = today_year - 1900;
    if ( first_season )
        // First season: the season has started the divider date of LAST year
        season_divider.tm_year--;
    start = static_cast<uint64_t>(mktime(&season_divider));

    season_divider.tm_year = today_year - 1900;
    if ( !first_season )
        // Second season: the season will end on the divider date of NEXT year
        season_divider.tm_year++;
    end = static_cast<uint64_t>(mktime(&season_divider));
}*/

void Logger::_printStamp(FILE *f)
{
    time_t now = static_cast<time_t>(FrameworkTimer::getTimeEpoc());
    struct tm *tm_struct = localtime(&now);
    int y = tm_struct->tm_year + 1900;
    int M = tm_struct->tm_mon + 1;
    int d = tm_struct->tm_mday;
    int h = tm_struct->tm_hour;
    int m = tm_struct->tm_min;
    int s = tm_struct->tm_sec;
    fprintf(f, "%04d/%02d/%02d-%02d:%02d.%02d", y, M, d, h, m, s );
}
