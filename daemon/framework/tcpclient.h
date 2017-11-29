#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <common_defs.h>
#include "basecomminterface.h"

namespace FrameworkLibrary {

class SocketCommDevice;

/** @brief Implement a Tcp client
 *
 * This class implements a generic TCP client using a multi-platform approach.
 *
 * You can either create a new socket or attach to an already existing system socket. The first approach
 * is more tipical, but the second approach is needed for example when you accept() a new client socket on
 * the server-side and you need to wrap that existing socket with this class without creating a new socket.
 *
 * @include example_cpp_tcp.cpp
 *
 * @review This class needs to be refactored to hide the implementation
 * @review Implement requirements
 *
 */
class DLLEXPORT TcpClient : public BaseCommInterface
{
public:
    /**
     * @brief Create a client TCP connecting to an existing socket
     * @param n name
     * @param existing_socket already existing socket
     */
    TcpClient(const std::string& n, SocketCommDevice *existing_socket );

    /**
     * @brief Create a new TCP client
     * @param n name
     * @param ra remote address
     * @param la local address
     * @param rp remote port
     * @param lp local port
     */
    TcpClient( const std::string& n,
               const std::string& ra,
               const std::string& la,
               uint16_t rp,
               uint16_t lp);

    virtual ~TcpClient();

    /** @brief set remote IP
     * @param ra remote address
     */
    void setRemoteIP( const std::string& ra );

    /** @brief set remote port
     * @param rp remote port
     */
    void setRemotePort( uint16_t rp );

    /** @brief get remote ip
     * @return remote address
     */
    std::string getRemoteIP() const;

    /** @brief set the TCP NODELAY option
     * @param t true or false
     */
    void setTcpNodelay( bool t );

    /** @brief get the TCP NODELAY setting
     * return true or false
     */
    bool getTcpNodelay() const;

protected:
    /**
     * @brief Create a uninitialized TCP client
     * @param n name
     *
    TcpClient(const std::string &n );*/

    /** @brief called when it's connected
     */
    virtual void channel_connected();

    /** @brief called when it's disconnected
     */
    virtual void channel_disconnected();

    SocketCommDevice* _socket; /**< Socket */

private:
    class __privateTcpClient;
    __privateTcpClient* _private;

    int customRead( void * data, int lenght);
    int customWrite( const void *data, int length);
    bool customRequestStateChange(interface_states, interface_states requested_state );
    void customStateChanged( interface_states current_state );
    std::string customGetLastReadAddress() const;
    std::string customGetInterfaceAddress() const;
};

}

#endif // TCPCLIENT_H
