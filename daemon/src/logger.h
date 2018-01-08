#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <stdio.h>

class Logger
{
public:
    Logger( const std::string& log_file );
    ~Logger();

    void logEvent( const std::string& event );
    void logTemp( float t, float h );

    bool isValid() const
    {
        return _log_file != NULL;
    }

private:
    void printStamp();

    std::string _log_filename;
    FILE* _log_file;
};

#endif // LOGGER_H
