#ifndef HISTORYITEM_H
#define HISTORYITEM_H

#include <stdio.h>
#include <stdint.h>

#include <string>

class HistoryItem
{
public:
    static uint32_t getSize();

    HistoryItem();
    HistoryItem( FILE* file );
    HistoryItem( uint64_t last_time,
                 float last_temp, float last_humidity,
                 float last_ext_temp,
                 float last_ext_humidity );
    HistoryItem( const HistoryItem& other );

    void operator=(const HistoryItem& other );

    bool isValid() const
    {
        return _valid;
    }

    uint64_t getTime() const
    {
        return _time;
    }

    float getTemp() const
    {
        return _temp;
    }

    float getExtTemp() const
    {
        return _ext_temp;
    }

    float getHumidity() const
    {
        return _humidity;
    }

    float getExtHumidity() const
    {
        return _ext_humidity;
    }

    void write( FILE* file );
    void writeNow( FILE* file );

private:
    uint64_t _time;
    float _temp;
    float _ext_temp;
    float _ext_humidity;
    float _humidity;
    bool _valid;
};

#endif // HISTORYITEM_H
