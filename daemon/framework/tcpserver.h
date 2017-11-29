#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <common_defs.h>

#include "basecomminterface.h"
#include <set>
#include <map>
#include <string.h>

namespace FrameworkLibrary {

class SocketCommDevice;
class BaseCommInterface;

class _TcpServerThread;

/** @brief Implement a Tcp server
 *
 * This class implements a TCP server. It will open a TCP listening socket and wait for clients to connect.
 * Upon client connection, a new TcpClient object which wraps the new connected client will be returned.
 *
 * You can specify how many clients you want to accept, after the maximum number has been reached, new clients will be
 * denied connection.
 *
 * @include example_cpp_tcp.cpp
 *
 * @review This class needs to be refactored to hide the implementation
 * @review Implement requirements
 *
 */
class DLLEXPORT TcpServer : public BaseCommInterface
{
    friend class _TcpServerThread;

public:
    TcpServer(const std::string& n, const std::string& ra, const std::string& la, uint16_t rp,
              uint16_t lp, const std::string& auth);
    virtual ~TcpServer();

    /** @brief limit max number of clients
     * @param mc max number of clients
     */
    void setMaxClients( int mc);

private:
    class __privateTcpServer;
    __privateTcpServer* _private;

    bool connectClient(BaseCommInterface* client);
    void checkClients();

    bool customRequestStateChange(interface_states, interface_states requested_state );
    virtual void customStateChanged( interface_states current_state );

    virtual bool customSocketSetup();
    bool customCanRead() const;
    bool customCanWrite() const;
    // The TCP server never return incoming data (is processed in the internal thread)
    bool customHasIncomingData();
    std::string customGetLastReadAddress() const;
    std::string customGetInterfaceAddress() const;

};

}

#endif // TCPSERVER_H
