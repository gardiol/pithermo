#include "mqtt_interface.h"

#include "frameworkutils.h"

#include <mosquitto.h>

using namespace FrameworkLibrary;

MQTT_Interface::MQTT_Interface(Logger *logger, MQTT_callback* callback, const std::string &host, const std::string &username, const std::string &password, int port)
    : _connected( false )
    , _data( NULL )
    , _logger( logger )
    , _callback( callback )
    , _mutex( "MQTT Callbacks Mutex" )
{
    if ( mosquitto_lib_init() == MOSQ_ERR_SUCCESS )
    {
        _data = (void*)mosquitto_new( NULL, true, this );
        if ( _data != NULL )
        {
            mosquitto_message_callback_set((struct mosquitto *)_data, MQTT_Interface::_message_callback );
            mosquitto_connect_callback_set((struct mosquitto *)_data, MQTT_Interface::_connect_callback );
            mosquitto_disconnect_callback_set((struct mosquitto *)_data, MQTT_Interface::_disconnect_callback );
            if ( mosquitto_loop_start(	(struct mosquitto *)_data ) == MOSQ_ERR_SUCCESS )
            {
                mosquitto_username_pw_set((struct mosquitto *)_data,
                                          username.c_str(),
                                          password.c_str() );
                mosquitto_connect( (struct mosquitto *)_data,
                                  host.c_str(),
                                  port,
                                  60 );
                _logger->logDebug( "MQTT created & loop started" );
            }
            else
                _logger->logDebug( "MQTT iloop start failed!" );
        }
        else
            _logger->logDebug( "MQTT instantiation error!" );
    }
    else
        _logger->logDebug( "MQTT lib init error!" );
}

MQTT_Interface::~MQTT_Interface()
{
    if ( _data != NULL )
    {
        if ( isConnected() )
            mosquitto_disconnect( (struct mosquitto *)_data );
        mosquitto_loop_stop( (struct mosquitto *)_data, true );
        mosquitto_destroy( (struct mosquitto *)_data );
        _data = NULL;
        _logger->logDebug( "MQTT all closed." );
    }
    mosquitto_lib_cleanup();
    _logger->logDebug( "MQTT cleanup done." );
}

void MQTT_Interface::publish(const std::string &topic, const std::string &data)
{
    if ( _data != NULL )
    {
        if ( isConnected() )
        {
            if ( int ret = mosquitto_publish( (struct mosquitto *)_data,
                                            NULL,
                                            topic.c_str(),
                                            data.length(),
                                            data.c_str(),
                                            0,
                                            true ) != MOSQ_ERR_SUCCESS )
                _logger->logDebug( "MQTT publish failed for topic '" + topic + "' [" + FrameworkUtils::tostring( ret ) + "]" );
        }
        else
            _logger->logDebug( "MQTT publish failed for topic '" + topic + "' (not connected)" );
    }
}

void MQTT_Interface::subscribe(const std::string &topic)
{
    bool is_new = true;
    _mutex.lock();
    if ( _subscribed_topics.find( topic ) == _subscribed_topics.end() )
        _subscribed_topics.insert( topic );
    else
        is_new =false;
    _mutex.unlock();
    if ( is_new )
        if ( isConnected() )
            _subscribe_topic( topic );
}

bool MQTT_Interface::isConnected()
{
    bool connected = false;
    _mutex.lock();
    connected = _connected;
    _mutex.unlock();
    return connected;
}

void MQTT_Interface::_subscribe_topic(const std::string &topic)
{
    if ( _connected )
    {
        if ( int ret = mosquitto_subscribe( (struct mosquitto *)_data, NULL, topic.c_str(), 0 ) != MOSQ_ERR_SUCCESS )
            _logger->logDebug( "MQTT subscribe failed for topic '" + topic + "' [" + FrameworkUtils::tostring( ret ) + "]" );
        else
            _logger->logDebug( "MQTT subscribed to topic '" + topic + "'" );
    }
}

void MQTT_Interface::_connect_callback(mosquitto *mosq, void *obj, int rc)
{
    MQTT_Interface* me = (MQTT_Interface*)obj;
    if ( rc == 0 )
    {
        me->_mutex.lock();
        me->_connected = true;
        for ( std::set<std::string>::const_iterator t = me->_subscribed_topics.begin();
             t != me->_subscribed_topics.end();
             ++t )
        {
            std::string topic = *t;
            me->_subscribe_topic( topic );
        }
        me->_mutex.unlock();
        me->_logger->logDebug( "MQTT connected." );
    }
    else
        me->_logger->logDebug( "MQTT connect failed (" + FrameworkUtils::tostring(rc) + "'" );
}

void MQTT_Interface::_disconnect_callback(mosquitto *mosq, void *obj, int rc)
{
    MQTT_Interface* me = (MQTT_Interface*)obj;
    me->_logger->logDebug( "MQTT disconnected." );
    // Do not reconnect if disconnect was called by us
    if ( rc != 0 )
    {
        me->_mutex.lock();
        me->_connected = false;
        me->_mutex.unlock();
        me->_logger->logDebug( "MQTT reconnection in progress..." );
        mosquitto_reconnect( (struct mosquitto *)me->_data );
    }
}

void MQTT_Interface::_message_callback(mosquitto *mosq,
                                       void *userdata,
                                       const mosquitto_message *message)
{
    MQTT_Interface* me = (MQTT_Interface*)userdata;
    std::string payload( (char*)message->payload, message->payloadlen );
    if ( me->_callback != NULL )
        me->_callback->message_received( message->topic, payload );
}

