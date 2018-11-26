#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <list>
#include <stdio.h>

#include "logitem.h"

class Logger
{
public:
    Logger(const std::string& events_path );
    ~Logger();

    void initializeStats();

    void enableDebug( bool d )
    {
        _debug = d;
    }

    bool getDebug() const
    {
        return _debug;
    }

    void logEvent( LogItem evt );
    void logDebug( const std::string& str );

    bool isValid() const
    {
        return _valid;
    }

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

    bool fetchInterval( uint64_t from, uint64_t to, std::list<LogItem>& items );

private:
    uint64_t _calculateDay( uint64_t t );
    void _calculateSeason( uint64_t& start, uint64_t &end );
    void _printStamp(FILE* f);

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

    bool _debug;
    bool _valid;
};

#endif // LOGGER_H
