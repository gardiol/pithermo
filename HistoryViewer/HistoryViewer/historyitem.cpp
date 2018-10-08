#include "historyitem.h"

uint32_t HistoryItem::getSize()
{
    return sizeof(_time) +
            sizeof(_temp) +
            sizeof(_ext_temp) +
            sizeof(_humidity) +
            sizeof(_ext_humidity);
}

HistoryItem::HistoryItem():
    _time(0),
    _temp(0.0),
    _ext_temp(0.0),
    _ext_humidity(0.0),
    _humidity(0.0),
    _valid(false)
{
}

HistoryItem::HistoryItem(FILE *file):
    _time(0),
    _temp(0.0),
    _ext_temp(0.0),
    _ext_humidity(0.0),
    _humidity(0.0),
    _valid(false)
{
    if ( file != nullptr )
    {
        if ( !feof( file )  )
            fread( &_time, sizeof(_time), 1, file );
        if ( !feof( file )  )
            fread( &_temp, sizeof(_temp), 1, file );
        if ( !feof( file )  )
            fread( &_ext_temp, sizeof(_ext_temp), 1, file );
        if ( !feof( file )  )
            fread( &_ext_humidity, sizeof(_ext_humidity), 1, file );
        if ( !feof( file )  )
        {
            if ( fread( &_humidity, sizeof(_humidity), 1, file ) == 1 )
            {
                _valid = true;
            }
        }
    }
}

HistoryItem::HistoryItem(uint64_t last_time, float last_temp, float last_humidity, float last_ext_temp, float last_ext_humidity):
    _time(last_time),
    _temp(last_temp),
    _ext_temp(last_ext_temp),
    _ext_humidity(last_ext_humidity),
    _humidity(last_humidity),
    _valid(true)
{
}

HistoryItem::HistoryItem(const HistoryItem &other):
    _time(other._time),
    _temp(other._temp),
    _ext_temp(other._ext_temp),
    _ext_humidity(other._ext_humidity),
    _humidity(other._humidity),
    _valid(other._valid)
{
}

void HistoryItem::operator=(const HistoryItem &other)
{
    _time = other._time;
    _temp = other._temp;
    _ext_temp = other._ext_temp;
    _ext_humidity = other._ext_humidity;
    _humidity = other._humidity;
    _valid = other._valid;
}
