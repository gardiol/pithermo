#ifndef HISTORY_H
#define HISTORY_H

#include <stdint.h>
#include <string>
#include <vector>

#include "historyitem.h"
#include <list>

class History
{
public:    
    History(const std::string& history_file);
    ~History();

    void initialize(float &ext_temp,
                    float &ext_humidity);
    bool update( float last_temp,
                 float last_humidity,
                 float last_ext_temp,
                 float last_ext_humidity );

    bool fetchInterval( uint64_t from, uint64_t to, std::list<HistoryItem>& items );

private:
    std::string _history_filename;

};

#endif // HISTORY_H
