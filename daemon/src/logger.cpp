#include "logger.h"

#include <frameworktimer.h>
#include <frameworkutils.h>

using namespace FrameworkLibrary;

Logger::Logger(const std::string &log_path):
    _log_filename(log_path + "/events"),
    _debug_filename(log_path + "/debug"),
    _debug(false),
    _valid(false),
    _log_update(false),
    _day(0)
{
    _valid = true;
    _day = _calculateDay( FrameworkTimer::getTimeEpoc() );
    FILE* f = fopen( _log_filename.c_str(), "rb" );
    if ( f != NULL )
    {
        bool keep_read = true;
        fseek( f, 0, SEEK_END );
        int32_t flen = ftell(f);
        int32_t size = LogItem::getSize();
        while ( keep_read && (flen >= size) )
        {   // Move to previous event:
            flen -= size;
            fseek( f, flen, SEEK_SET);
            LogItem evt( f );
            if ( evt.isValid() )
            {
                if ( _calculateDay( evt.getTime() ) == _day )
                    _today_logs.push_front( evt );
                else
                    keep_read = false;
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
    if ( f != NULL )
    {
        evt.write(f);
        fclose(f);
    }
}

void Logger::logMessage(const std::string &str)
{
    FILE* f = fopen(_debug_filename.c_str(), "a" );
    if ( f != NULL )
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

uint64_t Logger::_calculateDay(uint64_t t)
{
    uint64_t d = t / (3600*24);
    return d;
}

void Logger::_printStamp(FILE *f)
{
    time_t now = (time_t)FrameworkTimer::getTimeEpoc();
    struct tm *tm_struct = localtime(&now);
    int y = tm_struct->tm_year + 1900;
    int M = tm_struct->tm_mon + 1;
    int d = tm_struct->tm_mday;
    int h = tm_struct->tm_hour;
    int m = tm_struct->tm_min;
    int s = tm_struct->tm_sec;
    fprintf(f, "%04d/%02d/%02d-%02d:%02d.%02d", y, M, d, h, m, s );
}
