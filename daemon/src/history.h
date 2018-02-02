#ifndef HISTORY_H
#define HISTORY_H

#include <stdint.h>
#include <string>
#include <vector>

#include "historyitem.h"

class History
{
public:    
    History(const std::string& history_file , const std::string &exchange_path);
    ~History();

    void initialize( const std::string& mode, uint32_t len );
    bool update( float last_temp, float last_humidity );

    void setMode(const std::string& mode);
    std::string getMode() const
    {
        return std::string(&_mode, 1);
    }
    uint32_t getLen() const
    {
        return _num_lines;
    }

private:
    static const uint32_t num_weeks = 20;
    static const uint32_t num_days = 7;
    static const uint32_t num_hours = 24;
    static const uint32_t num_mins = 60;

    void _writeJson();
    void _splitTime( uint64_t t, uint32_t& w, uint32_t& d, uint32_t& h, uint32_t& m );
    void _readNow();

    uint32_t _skip_minutes;
    uint32_t _num_lines;
    uint32_t _points_per_line;

    uint64_t _now;
    uint32_t _now_min;
    uint32_t _now_hour;
    uint32_t _now_day;
    uint32_t _now_week;

    std::string _history_filename;
    std::string _exchange_path;
    FILE* _history_file;

    std::vector<std::vector<std::string> > _time_strs;
    std::vector<std::vector<std::string> > _temp_strs;
    std::vector<std::vector<std::string> > _humi_strs;
    std::vector<std::vector<bool> > _valid_ptr;

    char _mode;

    std::vector<std::vector<std::vector<std::vector<HistoryItem> > > > _history_cache;
};

#endif // HISTORY_H
