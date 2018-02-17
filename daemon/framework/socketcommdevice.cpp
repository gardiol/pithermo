#include "socketcommdevice.h"

#if defined(FRAMEWORK_PLATFORM_WINDOWS)
	#include <WS2tcpip.h>
	#include <MSWSock.h>
	typedef int socklen_t;
	#define MSG_NOSIGNAL 0
#elif defined(FRAMEWORK_PLATFORM_LINUX)
    #include <arpa/inet.h>
    #include <sys/socket.h>
    #include <sys/types.h>
    #include <netinet/in.h>
    #include <netdb.h>
    #include <fcntl.h>
    #include <sys/ioctl.h>
    #include <unistd.h>
    #include <ifaddrs.h>
    #include <netinet/tcp.h>
#define INVALID_SOCKET -1
#endif

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <sstream>

#include "memorychecker.h"
#include "frameworkutils.h"

using namespace FrameworkLibrary;

namespace FrameworkLibrary {

class __privateSocketCommDevice
{
    friend class SocketCommDevice;

    void updateAddresses()
    {
        struct sockaddr_in remote_addr;
        struct sockaddr_in local_addr;
        int len = sizeof( remote_addr );
        if ( getpeername( _socket, (struct sockaddr*)&remote_addr, (socklen_t*)&len) == 0 )
        {
            _remotePort = (uint16_t)(ntohs( remote_addr.sin_port ));
            _remoteAddress = std::string(inet_ntoa( remote_addr.sin_addr ));
        }
        if ( getsockname( _socket, (struct sockaddr*)&local_addr, (socklen_t*)&len) == 0 )
        {
            _localPort = ntohs( local_addr.sin_port );
            _localAddress = std::string(inet_ntoa( local_addr.sin_addr ));
        }
    }

    uint16_t _localPort;
    uint16_t _remotePort;
    SocketCommDevice::family _family;
    SocketCommDevice::type _type;
    SocketCommDevice::proto _proto;
#if defined(FRAMEWORK_PLATFORM_WINDOWS)
    SOCKET _socket;
#elif defined(FRAMEWORK_PLATFORM_LINUX)
    int _socket;
#endif

    std::string _localAddress;
    std::string _remoteAddress;

#if defined(FRAMEWORK_PLATFORM_WINDOWS)
    static bool winsock_initialized;
#endif

};

#if defined(FRAMEWORK_PLATFORM_WINDOWS)
bool __privateSocketCommDevice::winsock_initialized  = false;
#endif

}

SocketCommDevice::~SocketCommDevice()
{
    shutdown();
    delete _private;
    _private = NULL;
}

std::string SocketCommDevice::print_debug()
{
    char x[20];
    sprintf(x, "{Socket: %d}", _private->_socket );
    return x;
}

SocketCommDevice::SocketCommDevice(SocketCommDevice::family f,
                                   SocketCommDevice::type t,
                                   SocketCommDevice::proto p,
                                   const std::string &la,
                                   uint16_t lp,
                                   const std::string &ra,
                                   uint16_t rp):
    GenericCommDevice()
{
#if defined(FRAMEWORK_PLATFORM_WINDOWS)
    if ( !__privateSocketCommDevice::winsock_initialized )
    {
        WSADATA WsaDat;
        WSAStartup( MAKEWORD(2,2),&WsaDat);
        __privateSocketCommDevice::winsock_initialized = true;
    }
#endif
    _private = new __privateSocketCommDevice;
    _private->_localPort = lp;
    _private->_remotePort = rp;
    _private->_family = f;
    _private->_type = t;
    _private->_proto = p;
    _private->_socket = INVALID_SOCKET;

    _private->_localAddress = _private->_remoteAddress = "";
    if ( la != "" )
    {
        _private->_localAddress = FrameworkUtils::resolve_hostname(la);
        if ( _private->_localAddress == "" )
            _private->_localAddress = la;
    }
    if ( ra != "" )
    {
        _private->_remoteAddress = FrameworkUtils::resolve_hostname(ra);
        if ( _private->_remoteAddress == "" )
            _private->_remoteAddress = ra;
    }
    // Nessun socket può trasmettere se non ho settato un remote ip/port:
    if ( (_private->_remoteAddress == "") || !(_private->_remotePort > 0 ) )
        setCanWrite( false );

    // Verifico il protocollo:
    switch (_private->_proto )
    {
    case udpip:
        // non può ricevere se non ho settato una porta locale:
        if ( !(_private->_localPort > 0) )
            setCanRead( false );
        break;

    case tcpip:
        break;

    default:
        setCanWrite(false);
        setCanRead(false);
        setErrorString( "invalid protocol" );
    }
}

SocketCommDevice::SocketCommDevice(
#if defined(FRAMEWORK_PLATFORM_WINDOWS)
        SOCKET _existing_socket
#elif defined(FRAMEWORK_PLATFORM_LINUX)
        int _existing_socket
#endif
        ):
    GenericCommDevice()
{
    _private = new __privateSocketCommDevice;
    _private->_localPort = 0;
    _private->_remotePort = 0;
    _private->_socket = _existing_socket;
#if defined(FRAMEWORK_PLATFORM_WINDOWS)
    if ( !__privateSocketCommDevice::winsock_initialized )
	{
		WSADATA WsaDat;
		WSAStartup( MAKEWORD(2,2),&WsaDat);
        __privateSocketCommDevice::winsock_initialized = true;
	}
#endif
    _private->updateAddresses();
}

bool SocketCommDevice::setRemoteIP(const std::string &ra)
{
    if ( _private->_socket == INVALID_SOCKET )
    {
        _private->_remoteAddress = ra;
        return true;
    }
    return false;
}

bool SocketCommDevice::setRemotePort(uint16_t rp)
{
    if ( _private->_socket == INVALID_SOCKET )
    {
        _private->_remotePort = rp;
        return true;
    }
    return false;
}

std::string SocketCommDevice::getLocalIP() const
{
    return _private->_localAddress;
}

std::string SocketCommDevice::getRemoteIP() const
{
    return _private->_remoteAddress;
}

int SocketCommDevice::getLocalPort() const
{
    return _private->_localPort;
}

int SocketCommDevice::getRemotePort() const
{
    return _private->_remotePort;
}

SocketCommDevice* SocketCommDevice::accept()
{
    SocketCommDevice* ret = NULL;
    struct sockaddr_in client_address;
    socklen_t size = sizeof( client_address );
#if defined(FRAMEWORK_PLATFORM_WINDOWS)
    SOCKET new_client = -1;
#elif defined(FRAMEWORK_PLATFORM_LINUX)
    int new_client = -1;
#endif
    if ( ( new_client = ::accept( _private->_socket, (struct sockaddr*)&client_address, &size)) > 0 )
    {
        ret = MEMORY_CHECKER_ADD( new SocketCommDevice( new_client ), SocketCommDevice );
    }
    else
        setErrorString( FrameworkUtils::get_errno_string("accept") );
    return ret;
}

bool SocketCommDevice::bind()
{
    bool ret = false;
    int local_port = _private->_localPort;
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons( local_port );
    if ( (_private->_localAddress == "") || (_private->_localAddress == "0.0.0.0") )
        server_address.sin_addr.s_addr = htonl( INADDR_ANY );
    else
        server_address.sin_addr.s_addr = inet_addr( _private->_localAddress.c_str() );
    if ( ::bind( _private->_socket, (struct sockaddr*)&server_address, sizeof(server_address) ) > -1 )
        ret = true;
    else
        setErrorString( FrameworkUtils::get_errno_string("bind") );
    return ret;
}

bool SocketCommDevice::listen()
{
    bool ret = false;
    if ( ::listen( _private->_socket, 1 ) != -1 )
    {
        _private->updateAddresses();
        ret = true;
    }
    else
        setErrorString( FrameworkUtils::get_errno_string("listen") );
    return ret;
}


bool SocketCommDevice::setReuseAddr()
{
    bool ret = false;
    int reuse = 1;
    if ( setsockopt( _private->_socket,SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) > -1 )
        ret = true;
    else
        setErrorString( FrameworkUtils::get_errno_string("reuse_addr") );
    return ret;
}

bool SocketCommDevice::connect()
{
    bool ret = false;
    struct sockaddr_in server_address;
    int size = sizeof( server_address );
    memset( &server_address, 0, sizeof( server_address ) );
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr( _private->_remoteAddress.c_str() );
    server_address.sin_port = htons( _private->_remotePort );
    if ( ::connect( _private->_socket, (struct sockaddr*)&server_address, size) != -1 )
    {
        ret = true;
        _private->updateAddresses();
    }
    else
        setErrorString( FrameworkUtils::get_errno_string("connect") );
    return ret;
}

void SocketCommDevice::setTcpNodelay( bool s )
{
    if ( _private->_socket !=  INVALID_SOCKET )
    {
        int flag = s ? 1 : 0;
        setsockopt( _private->_socket, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(flag) );
    }
}

void SocketCommDevice::setSoLinger(uint32_t timeout)
{
    struct linger so_linger;
    so_linger.l_onoff = true;
    so_linger.l_linger = timeout;
    setsockopt( _private->_socket, SOL_SOCKET, SO_LINGER, (char*)&so_linger, sizeof(so_linger) );
}


int SocketCommDevice::readRaw( char* data, int max_lenght )
{
    struct sockaddr_in remote;
    socklen_t size = sizeof( remote );
    memset( &remote, 0, sizeof(remote) );
    int ret = recvfrom( _private->_socket, (char*)data, max_lenght, 0, (struct sockaddr*)&remote, &size );
    if ( ret == -1 )
        setErrorString( FrameworkUtils::get_errno_string("readRaw") );
    return ret;
}

int SocketCommDevice::readRaw(char *data, int max_lenght, std::string &remote_address)
{
    remote_address = "";
    struct sockaddr_in remote_addr;
    socklen_t size = sizeof( remote_addr );
    memset( &remote_addr, 0, sizeof(remote_addr) );
    int ret = recvfrom( _private->_socket, (char*)data, max_lenght, 0, (struct sockaddr*)&remote_addr, &size );
    if ( ret == -1 )
        setErrorString( FrameworkUtils::get_errno_string("readRaw") );
    else
    {
        uint16_t remote_port = (uint16_t)(ntohs( remote_addr.sin_port ));
        remote_address = "udp://::" + std::string(inet_ntoa( remote_addr.sin_addr )) + ":" + FrameworkUtils::tostring( remote_port );
    }
    return ret;
}

int SocketCommDevice::writeRaw( const char* data, int max_lenght )
{
    struct sockaddr_in server_address;
    int size = sizeof( server_address );
    memset( &server_address, 0, sizeof( server_address ) );
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr( _private->_remoteAddress.c_str() );
    server_address.sin_port = htons( _private->_remotePort );
    int ret = sendto( _private->_socket, data, max_lenght, MSG_NOSIGNAL, (const sockaddr*)&server_address, size );
    if ( ret == -1 )
        setErrorString( FrameworkUtils::get_errno_string("writeRaw") );
    return ret;
}

int SocketCommDevice::readLoop( char* data, int length )
{
    int tot = 0;
    while ( tot < length )
    {
        int ret = recv( _private->_socket, &((char*)data)[tot], length-tot, 0 );
        if ( ret <= 0 )
        {
            setErrorString( FrameworkUtils::get_errno_string("readLoop") );
            return -1;
        }
        tot += ret;
    }
    return tot;
}

int SocketCommDevice::writeLoop( const char* data, int length )
{
    int tot = 0;
    while ( tot < length )
    {
        int ret = send( _private->_socket, &((char*)data)[tot], length-tot, MSG_NOSIGNAL );
        if ( ret <= 0 )
        {
            setErrorString( FrameworkUtils::get_errno_string("writeLoop") );
            return -1;
        }
        tot += ret;
    }
    return tot;
}

void SocketCommDevice::joinMulticastGroup()
{
#if defined(FRAMEWORK_PLATFORM_WINDOWS)
	struct ip_mreq_source group;
#elif defined(FRAMEWORK_PLATFORM_LINUX)
    struct ip_mreq group;
#endif
    group.imr_multiaddr.s_addr = inet_addr( _private->_remoteAddress.c_str() );
    group.imr_interface.s_addr = htonl( INADDR_ANY );
    if ( setsockopt(_private->_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&group, sizeof(group) ) != 0 )
        FrameworkUtils::print_errno_string( "AddMembership warning: " );
}

bool SocketCommDevice::setMulticastLoopback(bool loopback_enabled)
{
    bool ret = false;
    bool set_loop = false;
    u_char loop;
    socklen_t size = sizeof( loop );
    if ( getsockopt( _private->_socket, IPPROTO_IP, IP_MULTICAST_LOOP, (char*)&loop, &size ) == 0 )
    {
        if ( loopback_enabled && (loop == 0) )
            set_loop = true;
        else if ( !loopback_enabled && (loop == 1) )
            set_loop = true;
    }
    if ( set_loop )
    {
        loop = loopback_enabled ? 1 : 0;
        if ( setsockopt( _private->_socket, IPPROTO_IP, IP_MULTICAST_LOOP, (char*)&loop, sizeof(loop) ) == 0 )
            ret = true;
        else
            setErrorString( FrameworkUtils::get_errno_string("setLoopback") );
    }
    else
        ret = true;
    return ret;
}

bool SocketCommDevice::setMulticastTTL( int multicast_ttl )
{
    bool ret = false;
    bool set_ttl = false;
    u_char ttl;
    socklen_t size = sizeof( ttl );
    if ( getsockopt( _private->_socket, IPPROTO_IP, IP_MULTICAST_TTL, (char*)&ttl, &size ) == 0 )
        if ( multicast_ttl != ttl )
            set_ttl = true;
    if ( set_ttl )
    {
        ttl = multicast_ttl;
        if ( setsockopt( _private->_socket, IPPROTO_IP, IP_MULTICAST_TTL, (char*)&ttl, sizeof(ttl) ) == 0 )
            ret = true;
        else
            setErrorString( FrameworkUtils::get_errno_string("setMulticastTTL") );
    }
    else
        ret = true;
    return ret;
}

int32_t SocketCommDevice::incomingDataSize() const
{
#if defined(FRAMEWORK_PLATFORM_WINDOWS)
    u_long n = 0;
    u_long ret = 0;
    int iret = 0;
    iret = WSAIoctl(_private->_socket, FIONREAD, NULL, 0, (char*)&ret, sizeof(ret), &n, NULL, NULL);
    if( iret == 0 )
        return (int32_t)ret;
    else
        return 0;
#elif defined(FRAMEWORK_PLATFORM_LINUX)
    int ret = 0;
    if ( ioctl( _private->_socket, FIONREAD, &ret ) < 0 )
        ret = -1;
    return ret;
#endif
}

bool SocketCommDevice::waitIncomingData( int64_t usecs) const
{
    // Three operating modes:
    // 1- Dont' wait, just check ( usecs = 0 )
    // 2- Wait for defined time ( usecs > 0 )
    struct timeval tv;
    fd_set set_read;
    tv.tv_sec  = (long)(usecs/1000000);
    tv.tv_usec = usecs%1000000;
    FD_ZERO( &set_read );
    FD_SET( _private->_socket, &set_read );
    if ( select( _private->_socket+1, &set_read, NULL, NULL, &tv ) > 0 )
        return true; // Descriptors ready! Always return true.
    return false;
}

bool SocketCommDevice::customInitialize()
{
    if ( _private->_socket == INVALID_SOCKET )
        _private->_socket = socket( getFamily(), getType(), getProto() );

#if defined(FRAMEWORK_PLATFORM_LINUX)
    if ( _private->_socket != -1 )
        // CLOSE ON EXEC serve almeno su linux per assicurasi che il socket venga chiuso, nel figlio, quando si forka un processo figlio...
        fcntl( _private->_socket, F_SETFD, fcntl( _private->_socket, F_GETFD ) | FD_CLOEXEC );
#endif
    // così ritorna true se il socket è già stato creato esternamente:
    return _private->_socket != INVALID_SOCKET;
}

void SocketCommDevice::customShutdown()
{
    setSoLinger(0);
    if ( _private->_socket != INVALID_SOCKET )
#if defined(FRAMEWORK_PLATFORM_WINDOWS)
        closesocket(_private->_socket);
#elif defined(FRAMEWORK_PLATFORM_LINUX)
        close(_private->_socket);
#endif
    _private->_socket = INVALID_SOCKET;
}

bool SocketCommDevice::customIsValid() const
{
    bool ret = false;
    if ( _private->_socket != INVALID_SOCKET )
    {
        fd_set sets;
        struct timeval tv;
        FD_ZERO( &sets );
        FD_SET( _private->_socket, &sets );
        tv.tv_sec = 0;
        tv.tv_usec = 0;
        if ( select( _private->_socket+1, &sets, &sets, &sets, &tv ) >= 0 )
            ret = true;
    }
    return ret;
}


std::string SocketCommDevice::customGetDeviceAddress() const
{
    std::stringstream ss;
    ss << _private->_localAddress << ":" << _private->_localPort <<
          ":" << _private->_remoteAddress << ":" << _private->_remotePort;
    return ss.str();
}

int SocketCommDevice::getFamily()
{
    switch (_private->_family )
    {
    case inet:
        return AF_INET;
    default:
        return -1;
    }
}

int SocketCommDevice::getType()
{
    switch (_private->_type)
    {
    case stream:
        return SOCK_STREAM;
    case dgram:
        return SOCK_DGRAM;
    default:
        return -1;
    }
}

int SocketCommDevice::getProto()
{
    switch (_private->_proto)
    {
    case tcpip:
        return IPPROTO_TCP;
    case udpip:
        return IPPROTO_UDP;
    default:
        return -1;
    }
}
