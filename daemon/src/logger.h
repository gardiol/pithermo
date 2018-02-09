#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <stdio.h>

class Logger
{
public:
    Logger( const std::string& log_file );
    ~Logger();

    void enableDebug( bool d )
    {
        _debug = d;
    }

    bool getDebug() const
    {
        return _debug;
    }

    void logEvent( const std::string& event );
    void logDebug( const std::string& str );
    void logTemp(float t, float h , float x);

    bool isValid() const
    {
        return _log_file != NULL;
    }

private:
    void printStamp();

    std::string _log_filename;
    FILE* _log_file;
    bool _debug;
};

#endif // LOGGER_H
