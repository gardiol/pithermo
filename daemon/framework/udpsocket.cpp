#include "udpsocket.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "socketcommdevice.h"
#include "frameworkutils.h"
#include "memorychecker.h"

using namespace FrameworkLibrary;

UdpSocket *UdpSocket::createFromAddressString(const std::string& name, const std::string &remote_config_string)
{
    UdpSocket* ret = NULL;
    // Ensure it's an UDP socket address string:
    if ( remote_config_string.substr(0, 6) == "udp://" )
    {
        std::vector<std::string> tokens = FrameworkUtils::string_split( remote_config_string.substr( 6 ), ":" );
        if ( tokens.size() >= 4 )
        {
            std::string la = tokens[0];
            std::string lp = tokens[1];
            std::string ra = tokens[2];
            std::string rp = tokens[3];
            ret = new UdpSocket( name, ra, la,
                                 rp == "" ? 0 : FrameworkUtils::string_toi( rp ),
                                 lp == "" ? 0 : FrameworkUtils::string_toi( lp ) );
        }
    }
    return ret;
}

UdpSocket::UdpSocket(const std::string &n,
                     const std::string &ra,
                     const std::string &la,
                     uint16_t rp,
                     uint16_t lp):
    BaseCommInterface( n ),
    _remote_address("")
{
    _socket = MEMORY_CHECKER_ADD( new SocketCommDevice( SocketCommDevice::inet,
                                    SocketCommDevice::dgram,
                                    SocketCommDevice::udpip,
                                    la, lp, ra, rp ), SocketCommDevice );
    if ( !setMainCommDevice( _socket ) )
    {
        delete MEMORY_CHECKER_DEL( _socket, SocketCommDevice );
        _socket = NULL;
    }
}

UdpSocket::~UdpSocket()
{
    cleanSkippedPackets();
}

void UdpSocket::skipFrame()
{
    if ( hasIncomingData() )
    {
        uint32_t size = 0;
        void* buffer = getFrame( size );
        _skipped_frames.push_back( std::pair<void*, uint32_t>( buffer, size ) );
    }
}

void UdpSocket::cleanSkippedPackets()
{
    while ( _skipped_frames.size() > 0 )
    {
        std::pair<void*, uint32_t> frame = _skipped_frames.front();
        _skipped_frames.pop_front();
        void* ptr = frame.first;
        delete [] (char*)ptr;
    }
}

std::list<std::pair<void *, uint32_t> > UdpSocket::getSkippedPackets()
{
    return _skipped_frames;
}

void *UdpSocket::getFrame(uint32_t &buffer_size)
{
    buffer_size = 0;
    if ( hasIncomingData() )
    {
        buffer_size = incomingDataSize();
        // Since this will be managed by the caller, this will NOT be protected by the MemoryChecker.
        void* buffer = (void*)new char[ buffer_size ];
        buffer_size = readData( buffer, buffer_size );
        return buffer;
    }
    return NULL;
}

bool UdpSocket::customSocketSetup()
{
    return true;
}

void UdpSocket::customSocketError()
{
}

bool UdpSocket::customRequestStateChange(BaseCommInterface::interface_states /*previous_state*/,
                                         BaseCommInterface::interface_states requested_state)
{
    bool ret = true;
    if ( requested_state == initialized )
    {
        // faccio bind solo se la porta locale è settata
        if ( _socket->getLocalPort() != 0 )
            ret = _socket->bind();
        if ( ret )
            ret = customSocketSetup();
    }
    else if ( requested_state == error )
        customSocketError();
    return ret;
}

std::string UdpSocket::customGetLastReadAddress() const
{
    return _remote_address;
}

std::string UdpSocket::customGetInterfaceAddress() const
{
    return "udp";
}

int UdpSocket::customRead( void *data, int length)
{
    return _socket->readRaw( (char*)data, length, _remote_address );
}

int UdpSocket::customWrite( const void *data, int length)
{
    return _socket->writeRaw( (const char*)data, length );
}

