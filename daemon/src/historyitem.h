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
    HistoryItem( uint64_t last_time, float last_temp, float last_humidity );
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

    float getHumidity() const
    {
        return _humidity;
    }

    std::string getTimeStr() const
    {
        return _time_str;
    }

    std::string getTempStr() const
    {
        return _temp_str;
    }

    std::string getHumidityStr() const
    {
        return _humidity_str;
    }

    void writeToFile( FILE* file );

private:
    uint64_t _time;
    float _temp;
    float _humidity;
    bool _valid;
    std::string _time_str;
    std::string _temp_str;
    std::string _humidity_str;

    void updateStr();
};

#endif // HISTORYITEM_H
