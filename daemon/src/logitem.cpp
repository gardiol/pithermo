#include "logitem.h"

#include <endian.h>

#include <frameworktimer.h>
#include <frameworkutils.h>

using namespace FrameworkLibrary;

LogItem::LogItem(LogItem::Event t):
    _time( FrameworkTimer::getTimeEpoc() ),
    _event(t),
    _time_str( FrameworkUtils::utostring( _time ) ),
    _event_str( FrameworkUtils::utostring( _event ) ),
    _valid(true)
{
}

LogItem::LogItem(FILE *file):
    _time(0),
    _event(0),
    _time_str(""),
    _event_str(""),
    _valid(false)
{
    if ( file != nullptr )
    {
        if ( fread( &_time, sizeof(_time), 1, file ) == 1 )
        {
            if ( fread( &_event, sizeof(_event), 1, file ) == 1 )
            {
                _time = le64toh( _time );
                _event = le64toh( _event );
                _time_str = FrameworkUtils::utostring( _time );
                _event_str = FrameworkUtils::utostring( _event );
                _valid = true;
            }
        }
    }
}

LogItem::LogItem(const LogItem &other):
    _time(other._time),
    _event(other._event),
    _time_str(other._time_str),
    _event_str(other._event_str),
    _valid(other._valid)
{
}

LogItem::LogItem():
    _time(0),
    _event(0),
    _time_str(""),
    _event_str(""),
    _valid(false)
{

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

void LogItem::operator=(const LogItem &other)
{
    _time = other._time;
    _event = other._event;
    _valid = other._valid;
    _time_str = other._time_str;
    _event_str = other._event_str;
}
