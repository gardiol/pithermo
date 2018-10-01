#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <list>
#include <stdio.h>

#include "logitem.h"

class Logger
{
public:
    Logger(const std::string& log_path , const std::string &exchange_path);
    ~Logger();

    void enableDebug( bool d )
    {
        _debug = d;
    }

    bool getDebug() const
    {
        return _debug;
    }

    void logEvent( LogItem evt );
    void logMessage( const std::string& str );
    void logDebug( const std::string& str );

    bool isValid() const
    {
        return _valid;
    }

    bool logsChanged();

    uint64_t getSeasonPelletOnTime() const
    {
        return _season_pellet_on_time;
    }

    uint64_t getSeasonPelletLowTime() const
    {
        return _season_pellet_low_time;
    }

    uint64_t getSeasonGasOnTime() const
    {
        return _season_gas_on_time;
    }

    uint64_t getTodayGasOnTime() const
    {
        return _today_gas_on_time;
    }

    uint64_t getTodayPelletOnTime() const
    {
        return _today_pellet_on_time;
    }

    uint64_t getTodayPelletLowTime() const
    {
        return _today_pellet_low_time;
    }

    uint64_t getTodayGasOnSince() const
    {
        return _today_gas_on_since;
    }

    uint64_t getTodayPelletOnSince() const
    {
        return _today_pellet_on_since;
    }

    uint64_t getTodayPelletLowOnSince() const
    {
        return _today_pellet_low_on_since;
    }

    void updateEventsJson();

private:
    uint64_t _calculateDay( uint64_t t );
    void _calculateSeason( uint64_t& start, uint64_t &end );
    void _printStamp(FILE* f);

    std::list<LogItem> _today_logs;

    uint64_t _season_pellet_on_time;
    uint64_t _season_pellet_low_time;
    uint64_t _season_gas_on_time;

    uint64_t _today_gas_on_time;
    uint64_t _today_pellet_on_time ;
    uint64_t _today_pellet_low_time;
    uint64_t _today_gas_on_since;
    uint64_t _today_pellet_on_since;
    uint64_t _today_pellet_low_on_since;

    std::string _log_filename;
    std::string _debug_filename;
    std::string _events_json_filename;

    bool _debug;
    bool _valid;
    bool _log_update;

    uint64_t _day;
};

#endif // LOGGER_H
