#include "tcpclient.h"
#include "basecomminterface.h"
#include "socketcommdevice.h"
#include "memorychecker.h"
#include "frameworkutils.h"

using namespace FrameworkLibrary;

namespace FrameworkLibrary {

class TcpClient::__privateTcpClient
{

public:
    explicit __privateTcpClient(const std::string& n):
        _externally_connected(false),
        _server_address("")
    {
        _tcp_nodelay = true;
    }

    __privateTcpClient(const std::string &n, SocketCommDevice* existing_socket):
        _externally_connected(true),
        _tcp_nodelay(true)
    {
        _server_address = "tcp://" + existing_socket->getRemoteIP() + ":" +
                FrameworkUtils::tostring( existing_socket->getRemotePort()) + ":" +
                existing_socket->getLocalIP() + FrameworkUtils::tostring( existing_socket->getLocalPort() );
    }

    __privateTcpClient(const std::string &n, const std::string &ra, const std::string &la, uint16_t rp, uint16_t lp):
        _externally_connected(false),
        _tcp_nodelay(true)
    {
        _server_address = "tcp://" + ra + ":" + FrameworkUtils::tostring(rp) + ":" + la + FrameworkUtils::tostring(lp);
    }

    ~__privateTcpClient()
    {
    }

    void setTcpNodelay(bool t)
    {
        _tcp_nodelay = t;
    }

    bool getTcpNodelay() const
    {
        return _tcp_nodelay;
    }

    bool getExternallyConnected() const
    {
        return _externally_connected;
    }

    std::string getServerAddress() const
    {
        return _server_address;
    }

private:
     bool _externally_connected;
     bool _tcp_nodelay;
     std::string _server_address;
};
}


/*TcpClient::TcpClient( const std::string& n ):
    BaseCommInterface( n )
{
    _private = new __privateTcpClient(n);
    // ricorda... è necessario chiamare setup_channel nella classe derivata per usare questo metodo!
}*/

void TcpClient::channel_connected()
{

}

void TcpClient::channel_disconnected()
{

}

TcpClient::TcpClient(const std::string &n, SocketCommDevice* existing_socket ):
    BaseCommInterface( n )
{
    _private = new __privateTcpClient(n, existing_socket);
    _socket = MEMORY_CHECKER_ADD( existing_socket, SocketCommDevice );
    if ( !setMainCommDevice( _socket ) )
    {
        delete MEMORY_CHECKER_DEL( _socket, SocketCommDevice );
        _socket = NULL;
    }
}

TcpClient::TcpClient(const std::string &n, const std::string &ra, const std::string &la, uint16_t rp, uint16_t lp):
    BaseCommInterface( n )
{
    _private = new __privateTcpClient(n, ra, la, rp, lp);
    _socket = MEMORY_CHECKER_ADD( new SocketCommDevice(   SocketCommDevice::inet,
                                                          SocketCommDevice::stream,
                                                          SocketCommDevice::tcpip,
                                                          "", 0, ra, rp ),
                                  SocketCommDevice );
    if ( !setMainCommDevice( _socket ) )
    {
        delete MEMORY_CHECKER_DEL( _socket, SocketCommDevice );
        _socket = NULL;
    }
}

TcpClient::~TcpClient()
{
    delete _private;
    _private = NULL;
}

int TcpClient::customWrite(const void *data, int length)
{
    if ( _private->getTcpNodelay() )
        _socket->setTcpNodelay(1);
    int ret = _socket->writeLoop( (const char*)data, length );
    if ( _private->getTcpNodelay() )
        _socket->setTcpNodelay(0);
    return ret;
}

int TcpClient::customRead(void *data, int length)
{
    int ret = _socket->readLoop( (char*)data, length );
    return ret;
}

bool TcpClient::customRequestStateChange(BaseCommInterface::interface_states, BaseCommInterface::interface_states requested_state)
{
    bool ret = true;

/*    if ( (requested_state == error) || (requested_state == not_configured) )
        channel_disconnected();
    else*/ if ( requested_state == activated ) // questo serve per non effettuare la connect sui socket già connessi (nuove connessioni da un server)
    {
        if ( !_private->getExternallyConnected() )
        {
            if ( !_socket->connect() )
                ret = false;
        }
        else
            ret = true;
    }
    return ret;
}

void TcpClient::customStateChanged(BaseCommInterface::interface_states current_state)
{
    if ( current_state == activated )
    {
        channel_connected();
    }
    else if ( (current_state == error) || (current_state == not_configured) )
    {
        channel_disconnected();
    }
}

std::string TcpClient::customGetLastReadAddress() const
{
    return _private->getServerAddress();
}

std::string TcpClient::customGetInterfaceAddress() const
{
    return "tcp";
}

void TcpClient::setRemoteIP(const std::string &ra)
{
    _socket->setRemoteIP( ra );
}

void TcpClient::setRemotePort(uint16_t rp)
{
    _socket->setRemotePort( rp );
}

std::string TcpClient::getRemoteIP() const
{
    return _socket->getRemoteIP();
}

void TcpClient::setTcpNodelay(bool t)
{
    _private->setTcpNodelay(t);
}

bool TcpClient::getTcpNodelay() const
{
    return _private->getTcpNodelay();
}

