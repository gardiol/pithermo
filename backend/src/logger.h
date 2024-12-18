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

    bool fetchInterval( uint64_t from, uint64_t to, std::list<LogItem>& items );
    bool fetchInterval(uint64_t from, uint64_t to, std::list<LogItem>& items,
                       bool search_prev,
                       bool& last_pellet_on,
                       bool& last_pellet_minimum_on,
                       bool& last_gas_on,
                       bool& last_pellet_flameout);

    bool calculateStats(uint64_t from,
                        uint64_t to, bool &prev_valid,
                        bool &prev_pellet_on,
                        bool &prev_pellet_minimum_on,
                        bool &prev_gas_on,
                        bool &pellet_flameout,
                        uint32_t& pellet_on_time,
                        uint32_t& pellet_low_time,
                        uint32_t& gas_on_time );

private:
    void _statsPelletOff( uint64_t& pellet_on_since,
                          uint32_t& pellet_on_time,
                          uint64_t& pellet_low_on_since,
                          uint32_t& pellet_low_time,
                          bool& pellet_was_on,
                          bool& pellet_on,
                          uint64_t event_time );

    void _printStamp(FILE* f);

    std::string _log_filename;
    std::string _debug_filename;

    bool _debug;
    bool _valid;
};

#endif // LOGGER_H
