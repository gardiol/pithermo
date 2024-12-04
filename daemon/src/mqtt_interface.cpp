#include "mqtt_interface.h"

#include "frameworkutils.h"

#include <mosquitto.h>

using namespace FrameworkLibrary;

MQTT_Interface::MQTT_Interface(Logger *logger, MQTT_callback* callback, const std::string &host, const std::string &username, const std::string &password, int port)
    : _host( host )
    , _port( port )
    , _username( username )
    , _password( password )
    , _connected( false )
    , _looping( false )
    , _data( NULL )
    , _logger( logger )
    , _callback( callback )
{
    if ( mosquitto_lib_init() == MOSQ_ERR_SUCCESS )
    {
        _data = (void*)mosquitto_new( NULL, true, this );
        if ( _data != NULL )
        {
            mosquitto_message_callback_set((struct mosquitto *)_data, MQTT_Interface::_message_callback );
            if ( mosquitto_loop_start(	(struct mosquitto *)_data ) == MOSQ_ERR_SUCCESS )
            {
                _looping = true;
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
        if ( _connected )
        {
            mosquitto_disconnect( (struct mosquitto *)_data );
        }
        if ( _looping )
        {
            mosquitto_loop_stop( (struct mosquitto *)_data, true );
        }
        mosquitto_destroy( (struct mosquitto *)_data );
        _logger->logDebug( "MQTT all closed." );
    }
    mosquitto_lib_cleanup();
    _logger->logDebug( "MQTT cleanup done." );
}

void MQTT_Interface::publish(const std::string &topic, const std::string &data)
{
    if ( _data != NULL )
    {
        _check_connection();
        if ( _connected )
        {
            if ( int ret = mosquitto_publish( (struct mosquitto *)_data,
                                  NULL,
                                  topic.c_str(),
                                  data.length(),
                                  data.c_str(),
                                  0,
                                  true ) != MOSQ_ERR_SUCCESS )
            {
                _logger->logDebug( "MQTT publish failed for topic '" + topic + "' [" + FrameworkUtils::tostring( ret ) + "] (reconnecting)" );
                _connected = false;
            }
        }
        else
            _logger->logDebug( "MQTT publish failed for topic '" + topic + "' (not connected)" );
    }
}

void MQTT_Interface::subscribe(const std::string &topic)
{
    _subscribed_topics.insert( topic );
    if ( !_connected )
        _check_connection();
    else
        _subscribe_topic( topic );
}

void MQTT_Interface::_check_connection()
{
    if ( !_connected )
    {
        mosquitto_username_pw_set((struct mosquitto *)_data,
                                  _username.c_str(),
                                  _password.c_str() );
        int ret = mosquitto_connect( (struct mosquitto *)_data,
                                    _host.c_str(),
                                    _port,
                                    60 );
        if ( ret  == MOSQ_ERR_SUCCESS )
        {
            _logger->logDebug( "MQTT connected to " + _host + ":" + FrameworkUtils::tostring( _port ) );
            _connected = true;
            for ( std::set<std::string>::const_iterator t = _subscribed_topics.begin();
                 t != _subscribed_topics.end();
                 ++t )
            {
                std::string topic = *t;
                _subscribe_topic( topic );
            }
        }
        else
            _logger->logDebug( "MQTT connect to " + _host + ":" + FrameworkUtils::tostring( _port ) + " failed [" + FrameworkUtils::tostring( ret ) + "]");
    }
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

void MQTT_Interface::_message_callback(mosquitto *mosq,
                                       void *userdata,
                                       const mosquitto_message *message)
{
    MQTT_Interface* me = (MQTT_Interface*)userdata;
    std::string payload( (char*)message->payload, message->payloadlen );
    if ( me->_callback != NULL )
    {
        me->_callback->message_received( message->topic, payload );
    }
}

