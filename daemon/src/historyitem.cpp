#include "historyitem.h"

#include <endian.h>

#include <frameworkutils.h>

using namespace FrameworkLibrary;

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
    _valid(false),
    _time_str("0"),
    _temp_str("0"),
    _ext_temp_str("0"),
    _ext_humidity_str("0"),
    _humidity_str("0")
{
}

HistoryItem::HistoryItem(FILE *file):
    _time(0),
    _temp(0.0),
    _ext_temp(0.0),
    _ext_humidity(0.0),
    _humidity(0.0),
    _valid(false),
    _time_str("0"),
    _temp_str("0"),
    _ext_temp_str("0"),
    _ext_humidity_str("0"),
    _humidity_str("0")
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
            fread( &_humidity, sizeof(_humidity), 1, file );
        if ( !feof( file )  )
        {
            if ( fread( &_ext_humidity, sizeof(_ext_humidity), 1, file ) == 1 )
            {
                _time = le64toh( _time );
                uint32_t t32 = le32toh( static_cast<uint32_t*>(static_cast<void*>(&_temp))[0] );
                _temp = static_cast<float*>(static_cast<void*>(&t32))[0];
                t32 = le32toh( static_cast<uint32_t*>(static_cast<void*>(&_ext_temp))[0] );
                _ext_temp = static_cast<float*>(static_cast<void*>(&t32))[0];
                t32 = le32toh( static_cast<uint32_t*>(static_cast<void*>(&_ext_humidity))[0] );
                _ext_humidity = static_cast<float*>(static_cast<void*>(&t32))[0];
                t32 = le32toh( static_cast<uint32_t*>(static_cast<void*>(&_humidity))[0] );
                _humidity = static_cast<float*>(static_cast<void*>(&t32))[0];
                _time_str = FrameworkUtils::utostring( _time );
                _temp_str = FrameworkUtils::ftostring( _temp );
                _ext_temp_str = FrameworkUtils::ftostring( _ext_temp );
                _ext_humidity_str = FrameworkUtils::ftostring( _ext_humidity );
                _humidity_str = FrameworkUtils::ftostring( _humidity );
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
    _valid(true),
    _time_str(FrameworkUtils::utostring( _time )),
    _temp_str(FrameworkUtils::ftostring( _temp )),
    _ext_temp_str(FrameworkUtils::ftostring( _ext_temp )),
    _ext_humidity_str(FrameworkUtils::ftostring( _ext_humidity )),
    _humidity_str(FrameworkUtils::ftostring( _humidity ))
{
}

HistoryItem::HistoryItem(const HistoryItem &other):
    _time(other._time),
    _temp(other._temp),
    _ext_temp(other._ext_temp),
    _ext_humidity(other._ext_humidity),
    _humidity(other._humidity),
    _valid(other._valid),
    _time_str(other._time_str),
    _temp_str(other._temp_str),
    _ext_temp_str(other._ext_temp_str),
    _ext_humidity_str(other._ext_humidity_str),
    _humidity_str(other._humidity_str)
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
    _time_str = other._time_str;
    _temp_str = other._temp_str;
    _ext_temp_str = other._ext_temp_str;
    _ext_humidity_str = other._ext_humidity_str;
    _humidity_str = other._humidity_str;
}

void HistoryItem::write(FILE *file)
{
    if ( file != nullptr )
    {
        uint64_t t64 = htole64( _time );
        fwrite( &t64, sizeof(t64), 1, file );        
        uint32_t t32 = htole32( static_cast<uint32_t*>(static_cast<void*>(&_temp))[0] );
        fwrite( &t32, sizeof(t32), 1, file );
        t32 = htole32( static_cast<uint32_t*>(static_cast<void*>(&_ext_temp))[0] );
        fwrite( &t32, sizeof(t32), 1, file );
        t32 = htole32( static_cast<uint32_t*>(static_cast<void*>(&_ext_humidity))[0] );
        fwrite( &t32, sizeof(t32), 1, file );
        t32 = htole32( static_cast<uint32_t*>(static_cast<void*>(&_humidity))[0] );
        fwrite( &t32, sizeof(t32), 1, file );
    }
}
