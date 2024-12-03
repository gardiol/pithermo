#include "mqtt_interface.h"

#include "frameworkutils.h"

#include <mosquitto.h>

using namespace FrameworkLibrary;

MQTT_Interface::MQTT_Interface(Logger *logger, const std::string &host, const std::string &username, const std::string &password, int port)
    : _host( host )
    , _port( port )
    , _username( username )
    , _password( password )
    , _connected( false )
    , _data( NULL )
    , _logger( logger )
{
    mosquitto_lib_init();
    _data = (void*)mosquitto_new( NULL, true, this );
    if ( _data != NULL )
        _logger->logDebug( "MQTT created" );
    _logger->logDebug( "MQTT init" );
}

MQTT_Interface::~MQTT_Interface()
{
    if ( _data != NULL )
    {
        mosquitto_destroy( (struct mosquitto *)_data );
        _logger->logDebug( "MQTT destroied" );
    }
    mosquitto_lib_cleanup();
    _logger->logDebug( "MQTT cleanup" );
}

void MQTT_Interface::publish(const std::string &topic, const std::string &data)
{
    if ( _data != NULL )
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
            }
            else
                _logger->logDebug( "MQTT connect to " + _host + ":" + FrameworkUtils::tostring( _port ) + " failed [" + FrameworkUtils::tostring( ret ) + "]");
        }
        if ( _connected )
        {
            int ret = mosquitto_publish( (struct mosquitto *)_data,
                                        NULL,
                                        topic.c_str(), data.length(), data.c_str(), 0, true );
            if ( ret != MOSQ_ERR_SUCCESS )
            {
                _logger->logDebug( "MQTT publish failed for topic '" + topic + "' [" + FrameworkUtils::tostring( ret ) + "] (reconnecting)" );
		int ret = mosquitto_reconnect( (struct mosquitto *)_data );
		if ( ret != MOSQ_ERR_SUCCESS ) {
                	_logger->logDebug( "MQTT unable to reconnect" );
			_connected = false;
		}
            }
        }
    }
}

//struct mosquitto *
