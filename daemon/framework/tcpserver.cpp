#include "tcpserver.h"
#include "configdata.h"
#include "tcpclient.h"
#include "basethread.h"
#include "debugprint.h"
#include "memorychecker.h"
#include "socketcommdevice.h"

#include <stdio.h>

using namespace FrameworkLibrary;

namespace FrameworkLibrary {


class _TcpServerThread: public BaseThread
{
public:
    _TcpServerThread( SocketCommDevice* s, TcpServer* t ):
        BaseThread( t->getName() + "_connector_thread" ),
        _server(t),
        _socket(s)
    {
        _active_timer.start();
        _active_timer.stop();
        _activated_count = 0;
        startThread();
    }

    ~_TcpServerThread()
    {
        requestTerminate();
        waitForEnd();
    }

private:
    TcpServer* _server;
    SocketCommDevice* _socket;
    FrameworkTimer _sleep_timer;
    FrameworkTimer _active_timer;

    void run()
    {
        _active_timer.stop();
        _sleep_timer.stop();
        _activated_count = 0;
        while ( !terminationRequested() && _socket->isValid() )
        {
            _sleep_timer.start();
            // Accetto tutti i socket in arrivo:
            while ( _socket->hasIncomingData() && _socket->isValid() )
            {
                _sleep_timer.stop();
                _activated_count++;
                _active_timer.start();
                SocketCommDevice* new_client = _socket->accept();
                if ( new_client != NULL )
                {
                    TcpClient* client = MEMORY_CHECKER_ADD( new TcpClient( _server->getName() + ":" + new_client->getRemoteIP(), new_client ), TcpClient );
                    if ( !_server->connectClient( client ) )
                    {
                        delete MEMORY_CHECKER_DEL( client, TcpClient );
                    }
                }
                _active_timer.stop();
            }
            // controllo tutti i socket connessi ed elimino quelli ormai chiusi...
            _server->checkClients();
            _socket->waitIncomingData( 1000*1000 );
        }
        _active_timer.stop();
        _sleep_timer.stop();
        _activated_count = 0;
    }

    void customPrintStatistics()
    {
        debugPrintUntagged() << "     -> Last active cycle took: " << FrameworkUtils::human_readable_number( _active_timer.elapsedTime(), "us", false, FrameworkUtils::DECIMAL_TYPE, 0 ) <<
                                ", Last sleep cycle lasted: " << FrameworkUtils::human_readable_number( _sleep_timer.elapsedTime(), "us", false, FrameworkUtils::DECIMAL_TYPE, 0 ) <<
                                ". Activated " << _activated_count <<
                                " times.\n";
    }

    uint64_t _activated_count;

};

class TcpServer::__privateTcpServer
{
    friend class TcpServer;

public:
    __privateTcpServer(const std::string& a):
        _socket(NULL),
        max_clients(32766),
        _connectorThread(NULL),
        _last_client_address(""),
        _auth(a)
    {
    }

    ~__privateTcpServer()
    {
        if ( _connectorThread != NULL )
        {
            _connectorThread->requestTerminate();
            _connectorThread->waitForEnd();
            delete MEMORY_CHECKER_DEL( _connectorThread, _TcpServerThread );
        }
        _connectorThread = NULL;
    }

    void setMaxClients(int mc)
    {
        max_clients = mc;
    }

    int getMaxClients() const
    {
        return max_clients;
    }

    std::string getAuth() const
    {
        return _auth;
    }

private:
    SocketCommDevice* _socket;

    int32_t max_clients;
    _TcpServerThread* _connectorThread;
    std::string _last_client_address;
    std::string _auth;

};

}

TcpServer::TcpServer(const std::string &n, const std::string &ra, const std::string &la, uint16_t rp, uint16_t lp, const std::string &auth):
    BaseCommInterface( n )
{
    _private = new __privateTcpServer( auth );
    _private->_socket = MEMORY_CHECKER_ADD( new SocketCommDevice( SocketCommDevice::inet, SocketCommDevice::stream, SocketCommDevice::tcpip,
                                                                  la, lp, ra, rp ), SocketCommDevice);
    if ( !setMainCommDevice( _private->_socket ) )
    {
        delete MEMORY_CHECKER_DEL( _private->_socket, SocketCommDevice );
        _private->_socket = NULL;
    }
    // It's meaningless to acquire statistics on TCP server sockets, since they do not send/receive any data.
    hideStatistics();
}

TcpServer::~TcpServer()
{
    delete _private;
    _private = NULL;

    // Scollega tutti i client
    std::set<BaseCommInterface*> copy = getSecondaryInterfaces();
    for ( std::set<BaseCommInterface*>::const_iterator i  = copy.begin(); i != copy.end(); ++i )
        removeSecondaryInterface( *i );

    FrameworkTimer timer;
    timer.usleep(500*1000);
}

bool TcpServer::customRequestStateChange(interface_states, interface_states requested_state )
{
    bool ret = true;
    if ( requested_state == initialized )
    {
        if ( _private->_socket->bind() )
        {
            if ( _private->_socket->listen() )
                ret = customSocketSetup();
            else
            {
                debugPrintError( getName() ) << "Listen error!\n";
                ret = false;
            }
        }
        else
        {
            debugPrintError( getName() ) << "Bind error: " << _private->_socket->getError() << ". (" << _private->_socket->getLocalIP() << ":" << _private->_socket->getLocalPort() << ")\n";
            ret = false;
        }
    }
    return ret;
}

void TcpServer::customStateChanged(BaseCommInterface::interface_states current_state)
{
    if ( current_state == activated )
    {
        if ( _private->_connectorThread == NULL )
        {
            _private->_connectorThread = MEMORY_CHECKER_ADD( new _TcpServerThread( _private->_socket, this ), _TcpServerThread );
        }
    }
    else if ( current_state == error )
    {
        delete MEMORY_CHECKER_DEL( _private->_connectorThread, _TcpServerThread );
        _private->_connectorThread = NULL;
    }
}

bool TcpServer::customSocketSetup()
{
    return true;
}

bool TcpServer::customCanRead() const
{
    return true;
}

bool TcpServer::customCanWrite() const
{
    return true;
}

bool TcpServer::customHasIncomingData()
{
    return false;
}

std::string TcpServer::customGetLastReadAddress() const
{
    return _private->_last_client_address;
}

std::string TcpServer::customGetInterfaceAddress() const
{
    return "tcp_server";
}

void TcpServer::setMaxClients(int mc)
{
    _private->setMaxClients( mc );
}

bool TcpServer::connectClient(BaseCommInterface * client)
{
    bool ret = false;
    if ( countSecondaryInterfaces() < _private->getMaxClients() )
    {
        bool auth_valid = true;
        client->activateInterface();
        if ( _private->_auth != "" )
        {
            auth_valid = false;
            uint32_t auth_len = _private->getAuth().length();
            char* buffer = new char[ auth_len+1 ];
            buffer[ auth_len ] = '\0';
            if ( client->readData( buffer, auth_len ) > 0 )
            {
                if ( _private->getAuth() == buffer )
                    auth_valid = true;
            }
            delete [] buffer;
        }
        if ( auth_valid )
        {
            _private->_last_client_address = client->getAddressString();
            addSecondaryInterface( client );
            ret = true;
        }
    }
    else
        debugPrintNotice( getName() ) << "Unable to accept a new client: maximum limit reached! (" << _private->getMaxClients() << ")\n";
    return ret;
}

void TcpServer::checkClients()
{
    std::set<BaseCommInterface*> copy = getSecondaryInterfaces();
    for ( std::set<BaseCommInterface*>::const_iterator i  = copy.begin(); i != copy.end(); ++i )
    {
        BaseCommInterface* client = *i;
        if ( !client->deviceValid() )
            removeSecondaryInterface( client );
    }
}
