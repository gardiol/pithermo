#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <list>
#include <stdio.h>

#include "logitem.h"

class Logger
{
public:
    Logger( const std::string& log_path );
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
    const std::list<LogItem>* getLogs() const
    {
        return &_today_logs;
    }

private:
    uint64_t _calculateDay( uint64_t t );
    void _printStamp(FILE* f);

    std::list<LogItem> _today_logs;

    std::string _log_filename;
    std::string _debug_filename;

    bool _debug;
    bool _valid;
    bool _log_update;

    uint64_t _day;
};

#endif // LOGGER_H
