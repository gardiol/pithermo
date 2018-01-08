#include "historyitem.h"

uint32_t HistoryItem::getSize()
{
    return sizeof(_time) + sizeof(_temp) + sizeof(_humidity);
}

HistoryItem::HistoryItem():
    _time(0),
    _temp(0.0),
    _humidity(0.0),
    _valid(false)
{
}

HistoryItem::HistoryItem(FILE *file):
    _time(0),
    _temp(0.0),
    _humidity(0.0),
    _valid(false)
{
    if ( file != NULL )
    {
        fread( &_time, sizeof(_time), 1, file );
        fread( &_temp, sizeof(_temp), 1, file );
        fread( &_humidity, sizeof(_humidity), 1, file );
    }
}

HistoryItem::HistoryItem(uint64_t last_time, float last_temp, float last_humidity):
    _time(last_time),
    _temp(last_temp),
    _humidity(last_humidity),
    _valid(true)
{
}

HistoryItem::HistoryItem(const HistoryItem &other):
    _time(other._time),
    _temp(other._temp),
    _humidity(other._humidity),
    _valid(other._valid)
{
}

void HistoryItem::operator=(const HistoryItem &other)
{
    _time = other._time;
    _temp = other._temp;
    _humidity = other._humidity;
    _valid = other._valid;
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
