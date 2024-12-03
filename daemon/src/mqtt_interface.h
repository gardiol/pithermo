#ifndef MQTT_INTERFACE_H
#define MQTT_INTERFACE_H

#include "logger.h"

#include <string>

class MQTT_callback
{
public:
    virtual void message_received( const std::string& topic, const std::string& payload) = 0;
};

class MQTT_Interface
{
public:
    MQTT_Interface(Logger* logger,
                   MQTT_callback* callback,
                   const std::string& host,
                   const std::string &username,
                   const std::string &password,
                   int port = 1883 );
    virtual ~MQTT_Interface();

    void publish( const std::string& topic, const std::string &data );
    void subscribe( const std::string& topic );

    void check_network();

private:
    std::string _host;
    int _port;
    std::string _username;
    std::string _password;

    bool _connected;
    void *_data;

    Logger* _logger;

    MQTT_callback* _callback;

    static void _message_callback(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message);
};

#endif // MQTT_INTERFACE_H
