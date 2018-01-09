#include "historyitem.h"

#include <frameworkutils.h>

using namespace FrameworkLibrary;

uint32_t HistoryItem::getSize()
{
    return sizeof(_time) + sizeof(_temp) + sizeof(_humidity);
}

HistoryItem::HistoryItem():
    _time(0),
    _temp(0.0),
    _humidity(0.0),
    _valid(false),
    _time_str(),
    _temp_str(),
    _humidity_str()
{
}

HistoryItem::HistoryItem(FILE *file):
    _time(0),
    _temp(0.0),
    _humidity(0.0),
    _valid(false),
    _time_str(),
    _temp_str(),
    _humidity_str()
{
    if ( file != NULL )
    {
        if ( !feof( file )  )
            fread( &_time, sizeof(_time), 1, file );
        if ( !feof( file )  )
            fread( &_temp, sizeof(_temp), 1, file );
        if ( !feof( file )  )
        {
            if ( fread( &_humidity, sizeof(_humidity), 1, file ) == 1 )
            {
                _time_str = FrameworkUtils::tostring( _time );
                _temp_str = FrameworkUtils::ftostring( _temp );
                _humidity_str = FrameworkUtils::ftostring( _humidity );
                _valid = true;
            }
        }
    }
}

HistoryItem::HistoryItem(uint64_t last_time, float last_temp, float last_humidity):
    _time(last_time),
    _temp(last_temp),
    _humidity(last_humidity),
    _valid(true),
    _time_str(FrameworkUtils::tostring( _time )),
    _temp_str(FrameworkUtils::ftostring( _temp )),
    _humidity_str(FrameworkUtils::ftostring( _humidity ))
{
}

HistoryItem::HistoryItem(const HistoryItem &other):
    _time(other._time),
    _temp(other._temp),
    _humidity(other._humidity),
    _valid(other._valid),
    _time_str(other._time_str),
    _temp_str(other._temp_str),
    _humidity_str(other._humidity_str)
{
}

void HistoryItem::operator=(const HistoryItem &other)
{
    _time = other._time;
    _temp = other._temp;
    _humidity = other._humidity;
    _valid = other._valid;
    _time_str = other._time_str;
    _temp_str = other._temp_str;
    _humidity_str = other._humidity_str;
}

void HistoryItem::writeToFile(FILE *file)
{
    if ( file != NULL )
    {
        fwrite( &_time, sizeof(_time), 1, file );
        fwrite( &_temp, sizeof(_temp), 1, file );
        fwrite( &_humidity, sizeof(_humidity), 1, file );
    }
}
