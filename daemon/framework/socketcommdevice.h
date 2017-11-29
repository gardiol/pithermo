#ifndef SOCKETCOMMDEVICE_H
#define SOCKETCOMMDEVICE_H

#include <common_defs.h>

#if defined(FRAMEWORK_PLATFORM_WINDOWS)
	#ifndef _WINSOCKAPI_
	#include <winsock2.h>
	#endif
    #define __genericSocket_ SOCKET
#else
    #define __genericSocket_ int
#endif

#include <set>
#include <string>
#include "genericcommdevice.h"

namespace FrameworkLibrary {

class __privateSocketCommDevice;

/** @brief Implement a socket (es: IP/TCP/UDP)
 *
 * This class implements, in a multiplatform approach, sockets. On windows it uses Winsock2, on linux, BSD sockets.
 *
 * @review Implement requirements
 * @review hide implementation
 *
 */
class SocketCommDevice :
        public GenericCommDevice
{
public:
    /** @brief protocol */
    enum proto { tcpip,  /**< TCP/IP */
                 udpip }; /** UDP/IP */
    /** @brief protocol family */
    enum family { inet }; /**< INET */
    /** @brief socket type */
    enum type { dgram,  /**< Datagram (UDP) */
                stream }; /**< Stream (TCP) */

    virtual ~SocketCommDevice();

    /** @brief print debug info
     * @return debug info
     */
    std::string print_debug();

    /** @brief Initialize socket (use this, normally!)
     * @param f Socket family
     * @param t Socket type
     * @param p Socket protocol
     * @param la Local address to which the bind will be done
     * @param lp Local port to which the bind will be done
     * @param ra Remote address to which the connect will be directed
     * @param rp Remote port to which the connect will be directed
     */
    SocketCommDevice(family f,
                      type t,
                      proto p,
                      const std::string& la,
                      uint16_t lp,
                      const std::string& ra,
                      uint16_t rp );

    /** @brief Initialize socket from already open socket
     *
     * Used, for example, after a connect
     * @param _existing_socket system ID of an already existing socket
     */
    SocketCommDevice( __genericSocket_ _existing_socket );

    /** @brief Change of set the remote socket address
     * @param ra new remote address
     * @return true if change is successful. False if socket is already connected
     * @note do not call this if the socket is already connected
     */
    bool setRemoteIP( const std::string& ra );

    /** @brief set the remote port for the socket
     * @since 3.4
     * @note If you call this after the socket is connected, it will be ignored.
     * @param rp the port to connect to
     * @return true if ok, false if already connected
     */
    bool setRemotePort(uint16_t rp );

    /** @brief Return local address
     * @return local address
     */
    std::string getLocalIP() const;

    /** @brief Return remote address
     * @return Remote address
     */
    std::string getRemoteIP() const;

    /** @brief Return local port
     * @return local port
     */
    int getLocalPort() const;

    /** @brief Return remote port
     * @return remote port
     */
    int getRemotePort() const;

    /** @brief Do an accept (system call on the socket)
     * @return the new socket, or NULL if accept has failed
     */
    SocketCommDevice* accept();

    /** @brief Bind a socket (system call on the socket)
     * @return false if bind failed, true otherwise
     */
    bool bind();

    /** @brief Do a listen on the socket (system call on the socket)
     * @return false if listen failed, true otherwise
     */
    bool listen();

    /** @brief Do a connect on the socket (system call on the socket)
     * @return false if failed, true otherwise.
     */
    bool connect();

    /** @brief Set the TCP_NODELAY option
     */
    void setTcpNodelay(bool s);

    /** @brief Set the SO_REUSEADDR option
     * @return true if the socket reuse can be set
     */
    bool setReuseAddr();

    /** @brief Set the SO_LINGER option on the socket
     * @param timeout SO_LINGER timeout in seconds
     */
    void setSoLinger( uint32_t timeout );

    /** @brief Receive a data block
     *
     * This is a single read, not looping, so it can return less than the requested data size.
     * @param data pointer to store the read data
     * @param max_lenght maximum size to read
     * @return byte actually read. -1 if errors, 0 on socket closed
     */
    int readRaw( char* data, int max_lenght );

    /** @brief Receive a data block, with remote address
     *
     * This is a single read, not looping, so it can return less than the requested data size.
     * @param data pointer to store the read data
     * @param max_lenght maximum size to read
     * @param remote_address [out] return remote socket address
     * @return byte actually read. -1 if errors, 0 on socket closed
     */
    int readRaw( char* data, int max_lenght, std::string& remote_address );

    /** @brief Send a data block
     *
     * This is a single write, not looping, so it can return less than the requested data size.
     * @param data pointer to the data to be sent
     * @param max_lenght maximum size to write
     * @return actual number of bytes sent, -1 on error and 0 on socket closed
     */
    int writeRaw( const char* data, int max_lenght );

    /** @brief Read a fixed amount of data
     *
     * Unless errors or socket closed, this function will stop until all the requested data is retrieved.
     * @param data pointer to store the read data
     * @param length maximum size to read
     * @return byte actually read. -1 if errors, 0 on socket closed
     */
    int readLoop(char* data, int length );

    /** @brief Send a fixed amount of data
     *
     * Unless errors or socket closed, this function will stop until all the requested data is sent.
     * @param data pointer to the data to be sent
     * @param length maximum size to write
     * @return actual number of bytes sent, -1 on error and 0 on socket closed
     */
    int writeLoop(const char* data, int length );

    /** @brief Multicast support, experimental
     */
    void joinMulticastGroup();

    /** @brief Multicast support, experimental
     * @param loopback_enabled if true, multicast loopback will be enabled
     * @return true if multicast is supported
     */
    bool setMulticastLoopback(bool loopback_enabled);

    /** @brief Multicast support, experimental
     * @param multicast_ttl the TTL
     * @return true if multicast is supported
     */
    bool setMulticastTTL(int multicast_ttl);

    /** @brief how many bytes are available to read
     * @return size of available data in bytes
     */
    int32_t incomingDataSize() const;

    /** @brief wait for incoming data for at most usecs microseconds
     * @param usecs how long to wait for
     * @return true if data is available, false if timeout is expired and no data is available
     */
    bool waitIncomingData( int64_t usecs ) const;

private:
    __privateSocketCommDevice* _private;

    bool customInitialize();
    void customShutdown();
    bool customIsValid() const;
    std::string customGetDeviceAddress() const;

    int getFamily();
    int getType();
    int getProto();

};

}

#endif // SOCKETCOMMDEVICE_H
