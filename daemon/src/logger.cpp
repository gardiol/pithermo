#include "logger.h"

#include <frameworktimer.h>
#include <frameworkutils.h>

using namespace FrameworkLibrary;

Logger::Logger(const std::string &log_file):
    _log_filename(log_file),
    _log_file(NULL),
    _debug(false)
{
    _log_file = fopen( _log_filename.c_str(), "a" );
    if ( _log_file != NULL )
        logEvent( "Logger started" );
}

Logger::~Logger()
{
    if ( _log_file != NULL )
    {
        logEvent( "Logger closed" );
        fclose( _log_file );
    }
}

void Logger::logEvent(const std::string &event)
{
    if ( _log_file != NULL )
    {
        printStamp();
        fprintf(_log_file, " -- %s\n", event.c_str() );
        fflush(_log_file);
    }
}

void Logger::logDebug(const std::string &str)
{
    if ( _debug )
    {
        printStamp();
        fprintf(_log_file, " -debug- %s\n", str.c_str() );
        fflush(_log_file);
    }
}

void Logger::logTemp(float t, float h)
{
    if ( _log_file != NULL )
    {
        printStamp();
        fprintf(_log_file, " -- Temp: %f Humidity: %f\n", t, h );
        fflush(_log_file);
    }
}

void Logger::printStamp()
{
    time_t now = (time_t)FrameworkTimer::getTimeEpoc();
    struct tm *tm_struct = localtime(&now);
    int y = tm_struct->tm_year + 1900;
    int M = tm_struct->tm_mon + 1;
    int d = tm_struct->tm_mday;
    int h = tm_struct->tm_hour;
    int m = tm_struct->tm_min;
    int s = tm_struct->tm_sec;
    fprintf(_log_file, "%04d/%02d/%02d-%02d:%02d.%02d", y, M, d, h, m, s );
}
