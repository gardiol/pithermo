#ifndef MQTT_INTERFACE_H
#define MQTT_INTERFACE_H

#include "logger.h"

#include <basemutex.h>

#include <string>
#include <set>

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

    bool isConnected();

private:
    void _subscribe_topic( const std::string& topic );

    bool _connected;
    void *_data;

    Logger* _logger;

    MQTT_callback* _callback;

    std::set<std::string> _subscribed_topics;

    FrameworkLibrary::BaseMutex _mutex;

    static void _connect_callback(struct mosquitto *mosq, void *obj, int rc);
    static void _disconnect_callback(struct mosquitto *mosq, void *obj, int rc);
    static void _message_callback(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message);
};

#endif // MQTT_INTERFACE_H
