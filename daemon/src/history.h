#ifndef HISTORY_H
#define HISTORY_H

#include <stdint.h>
#include <string>
#include <vector>

#include "historyitem.h"

class History
{
public:    
    History(const std::string& history_file,
            const std::string &exchange_path);
    ~History();

    void initialize(float &ext_temp,
                    float &ext_humidity);
    bool update( float last_temp,
                 float last_humidity,
                 float last_ext_temp,
                 float last_ext_humidity );

private:
    void _splitTime( uint64_t t, uint32_t& w, uint32_t& d, uint32_t& h, uint32_t& m );
    void _readNow();

    uint64_t _now;
    uint32_t _now_min;
    uint32_t _now_hour;
    uint32_t _now_day;
    uint32_t _now_week;
    uint32_t _now_year;

    std::string _history_filename;
    std::string _exchange_path;
    std::string _now_filename;
    FILE* _history_file;

};

#endif // HISTORY_H
