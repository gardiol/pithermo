#ifndef MQTT_INTERFACE_H
#define MQTT_INTERFACE_H

#include "logger.h"

#include <string>

class MQTT_Interface
{
public:
    MQTT_Interface( Logger* logger, const std::string& host, int port = 1883 );
    virtual ~MQTT_Interface();

    void publish( const std::string& topic, const std::string &data );

private:
    std::string _host;
    int _port;
    bool _connected;
    void *_data;

    Logger* _logger;

};

#endif // MQTT_INTERFACE_H
