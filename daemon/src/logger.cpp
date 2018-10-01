#include "logger.h"

#include <string.h>

#include <frameworktimer.h>
#include <frameworkutils.h>

using namespace FrameworkLibrary;

Logger::Logger(const std::string &log_path, const std::string& exchange_path):
    _season_pellet_on_time(0),
    _season_pellet_low_time(0),
    _season_gas_on_time(0),
    _today_gas_on_time(0),
    _today_pellet_on_time(0),
    _today_pellet_low_time(0),
    _today_gas_on_since(0),
    _today_pellet_on_since(0),
    _today_pellet_low_on_since(0),
    _log_filename(log_path + "/events"),
    _debug_filename(log_path + "/debug"),
    _events_json_filename(exchange_path + "/_events"),
    _debug(false),
    _valid(false),
    _log_update(false),
    _day(0)
{
    _valid = true;
    _day = _calculateDay( FrameworkTimer::getTimeEpoc() );
    FILE* f = fopen( _log_filename.c_str(), "rb" );

    uint64_t season_start = 0;
    uint64_t season_stop = 0;
    _calculateSeason( season_start, season_stop );

    if ( f != nullptr )
    {
        uint64_t gas_on_since = 0;
        uint64_t pellet_on_since = 0;
        uint64_t pellet_low_on_since = 0;
        while ( !feof(f) )
        {
            LogItem evt( f );
            if ( evt.isValid() )
            {
                uint64_t event_time = evt.getTime();
                bool in_season = (event_time >= season_start ) && (event_time <= season_stop );
                bool is_today = _calculateDay( event_time ) == _day;
                if ( in_season || is_today )
                {
                    if ( is_today )
                        _today_logs.push_front( evt );
                    switch ( evt.getEvent() )
                    {
                    case  LogItem::GAS_ON:
                        if ( in_season && (gas_on_since == 0) )
                            gas_on_since = event_time;
                        if ( is_today && (_today_gas_on_since == 0) )
                            _today_gas_on_since = event_time;
                        break;

                    case LogItem::PELLET_MINIMUM:
                        if ( in_season &&
                             ( pellet_on_since > 0 ) &&
                             ( pellet_low_on_since == 0 ) )
                                pellet_low_on_since = event_time;
                        if ( is_today &&
                             ( _today_pellet_on_since > 0 ) &&
                             ( _today_pellet_low_on_since == 0 ) )
                                _today_pellet_low_on_since = event_time;
                        break;

                    case LogItem::PELLET_MODULATION:
                        if ( in_season && (pellet_low_on_since > 0) )
                        {
                            _season_pellet_low_time += event_time - pellet_low_on_since;
                            pellet_low_on_since = 0;
                        }
                        if ( is_today && (_today_pellet_low_on_since > 0 ) )
                        {
                            _today_pellet_low_time += event_time - _today_pellet_low_on_since;
                            _today_pellet_low_on_since = 0;
                        }
                        break;

                    case  LogItem::PELLET_ON:
                        if ( in_season && (pellet_on_since == 0) )
                            pellet_on_since = event_time;
                        if ( is_today && (_today_pellet_on_since == 0) )
                            _today_gas_on_since = event_time;
                        break;

                    case LogItem::PELLET_OFF:
                        if ( in_season && (pellet_on_since > 0) )
                        {
                            _season_pellet_on_time += event_time - pellet_on_since;
                            pellet_on_since = 0;
                        }
                        if ( is_today && (_today_pellet_on_since > 0 ) )
                        {
                            _today_pellet_on_time += event_time - _today_pellet_on_since;
                            _today_pellet_on_since = 0;
                        }
                        if ( in_season && (pellet_low_on_since > 0) )
                        {
                            _season_pellet_low_time += event_time - pellet_low_on_since;
                            pellet_low_on_since = 0;
                        }
                        if ( is_today && (_today_pellet_low_on_since > 0 ) )
                        {
                            _today_pellet_low_time += event_time - _today_pellet_low_on_since;
                            _today_pellet_low_on_since = 0;
                        }
                        break;

                    case LogItem::GAS_OFF:
                        if ( is_today && (_today_gas_on_since > 0 ) )
                        {
                            _today_gas_on_time += event_time - _today_gas_on_since;
                            _today_gas_on_since = 0;
                        }
                        if ( in_season && (gas_on_since > 0) )
                        {
                            _season_gas_on_time += event_time - gas_on_since;
                            gas_on_since = 0;
                        }
                        break;
                    }
                }
                else
                {
                    gas_on_since = 0;
                    pellet_on_since = 0;
                    pellet_low_on_since = 0;
                }
            }
        }
        fclose(f);
    }
}

Logger::~Logger()
{
}

void Logger::logEvent(LogItem evt)
{
    uint64_t new_day = _calculateDay( FrameworkTimer::getTimeEpoc() );
    if ( new_day != _day )
    {
        _today_logs.clear();
        _day = new_day;
    }
    _today_logs.push_back( evt );
    _log_update = true;
    FILE* f = fopen(_log_filename.c_str(), "ab" );
    if ( f != nullptr )
    {
        evt.write(f);
        fclose(f);
    }
}

void Logger::logMessage(const std::string &str)
{
    FILE* f = fopen(_debug_filename.c_str(), "a" );
    if ( f != nullptr )
    {
        _printStamp(f);
        fprintf(f, " %s\n", str.c_str() );
        fclose(f);
    }
}

void Logger::logDebug(const std::string &str)
{
    if ( _debug )
        logMessage( " -debug- " + str );
}

bool Logger::logsChanged()
{
    if ( _log_update )
    {
        _log_update = false;
        return true;
    }
    return false;
}

void Logger::updateEventsJson()
{
    if ( logsChanged() )
    {
        FILE* event_file = fopen( _events_json_filename.c_str(), "w" );
        if ( event_file )
        {
            fwrite( "[", 1, 1, event_file );
            for ( std::list<LogItem>::const_iterator i = _today_logs.begin(); i != _today_logs.end(); ++i )
            {
                if ( i != _today_logs.begin() )
                    fwrite(",", 1, 1, event_file );
                fwrite( "{\"t\":", 5, 1, event_file );
                std::string time_str = i->getTimeStr();
                std::string evt_str = i->getEventStr();
                fwrite( time_str.c_str(), time_str.length(), 1, event_file );
                fwrite(",\"e\":", 5, 1, event_file );
                fwrite( evt_str.c_str(), evt_str.length(), 1, event_file );
                fwrite("}", 1, 1, event_file );
            }
            fwrite( "]", 1, 1, event_file );
            fclose( event_file );
        }
    }
}

uint64_t Logger::_calculateDay(uint64_t t)
{
    uint64_t d = t / (3600*24);
    return d;
}

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
}

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
