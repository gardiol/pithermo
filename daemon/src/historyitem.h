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
    HistoryItem( uint64_t last_time, float last_temp, float last_humidity, float last_ext_temp );
    HistoryItem( const HistoryItem& other );

    bool getStrings( std::string& ti, std::string& te, std::string&h, std::string& ext_te ) const
    {
        ti = _time_str;
        te = _temp_str;
        h = _humidity_str;
        ext_te = _ext_temp_str;
        return _valid;
    }

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

    std::string getTimeStr() const
    {
        return _time_str;
    }

    std::string getTempStr() const
    {
        return _temp_str;
    }


    std::string getExtTempStr() const
    {
        return _ext_temp_str;
    }

    std::string getHumidityStr() const
    {
        return _humidity_str;
    }

    void write( FILE* file );

private:
    uint64_t _time;
    float _temp;
    float _ext_temp;
    float _humidity;
    bool _valid;
    std::string _time_str;
    std::string _temp_str;
    std::string _ext_temp_str;
    std::string _humidity_str;

    void updateStr();
};

#endif // HISTORYITEM_H
