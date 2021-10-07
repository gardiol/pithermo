#include "logitem.h"

#include <endian.h>

#include <frameworktimer.h>
#include <frameworkutils.h>

using namespace FrameworkLibrary;

uint32_t LogItem::getSize()
{
    return sizeof(_time) +
            sizeof(_event);
}

LogItem::LogItem(LogItem::Event t):
    _time( FrameworkTimer::getTimeEpoc() ),
    _event(t),
    _valid(true)
{
}

LogItem::LogItem(FILE *file):
    _time(0),
    _event(0),
    _valid(false)
{
    read(file);
}

LogItem::LogItem(const LogItem &other):
    _time(other._time),
    _event(other._event),
    _valid(other._valid)
{
}

LogItem::LogItem():
    _time(0),
    _event(0),
    _valid(false)
{

}

void LogItem::read(FILE *file)
{
    _valid = false;
    if ( file != nullptr )
    {
        if ( fread( &_time, sizeof(_time), 1, file ) == 1 )
        {
            if ( fread( &_event, sizeof(_event), 1, file ) == 1 )
            {
                _time = le64toh( _time );
                _event = le64toh( _event );
                _valid = true;
            }
        }
    }
}

void LogItem::write(FILE *file)
{
    if ( file != nullptr )
    {
        uint64_t t64 = htole64( _time );
        fwrite( &t64, sizeof(t64), 1, file );
        t64 = htole64( _event );
        fwrite( &t64, sizeof(t64), 1, file );
    }
}

void LogItem::writeText(FILE *file)
{
    if ( file != nullptr )
        fprintf(file, "%llu %llu\n",
                static_cast<unsigned long long int>(_time),
                static_cast<unsigned long long int>(_event) );
}

void LogItem::operator=(const LogItem &other)
{
    _time = other._time;
    _event = other._event;
    _valid = other._valid;
}
