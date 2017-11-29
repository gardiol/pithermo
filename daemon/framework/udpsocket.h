#ifndef UDPSERVER_H
#define UDPSERVER_H

#include <common_defs.h>

#include <string>
#include "basecomminterface.h"
#include <list>

namespace FrameworkLibrary {

class SocketCommDevice;

/** @brief Implement an UDP socket
 *
 * This class implement an UDP socket.
 *
 * @include example_cpp_udp.cpp
 *
 * @review This class needs to be refactored to hide the implementation
 * @review Implement requirements
 *
 */
class DLLEXPORT UdpSocket : public BaseCommInterface
{
public:
    static UdpSocket* createFromAddressString(const std::string &name, const std::string& remote_config_string );

    /** @brief Initialize an UDP socket
      *
      * To disable the server part (bind) set `la = ""` and `lp = 0`.
      * To disable the client part (send data), set `ra = ""` and `rp = 0`.
      * @param n channel name
      * @param ra remote address
      * @param la local address
      * @param rp remote port
      * @param lp local port
      */
    UdpSocket(const std::string& n,
              const std::string& ra,
              const std::string& la,
              uint16_t rp,
              uint16_t lp);

    virtual ~UdpSocket();

    /** @brief Read and discard the next UDP packet received.
     * @since 3.3
     *
     * This will drop the packet from the queue letting you access the next packet.
     * Use this when you need to skip a packet you don't want to read right now.
     * The packet will be placed in the "skip" list which can be read with getSkippedPackets()
     * and MUST be cleaned with cleanSkippedPackets().
     */
    void skipFrame();

    /** @brief Release all the memory associated with skipped packets. This will invalidate any pointer from getSkippedPackets().
     * @since 3.3
     */
    void cleanSkippedPackets();

    /** @brief Return a list of all the "slipped" packets.
     * @since 3.3
     *
     * Remember to free them with cleanSkippedPackets()! But note that will free all the returner pointers.
     * @return a list containing a map of data / size of data values
     */
    std::list<std::pair<void*, uint32_t> > getSkippedPackets();

    /** @brief Read the next packet from the socket and return it
     *
     * @warning you are in charge to delete (with delete[]) the returned buffer! Don't forget it.
     * @param buffer_size of the returned buffer, it's an output.
     * @return pointer to the data buffer. Remember to free it yourself! NULL is returned if no frame is ready to be read.
     * @since 3.3
     */
    void* getFrame( uint32_t& buffer_size );

protected:
    /** @brief initialize the socket
     * @return true if ok
     */
    virtual bool customSocketSetup();

    /** @brief set the socket to error
     */
    virtual void customSocketError();

    SocketCommDevice* _socket; /**< the socket */

private:
    int customRead( void * data, int lenght);
    int customWrite( const void *data, int length);
    bool customRequestStateChange(interface_states, interface_states requested_state );
    std::string customGetLastReadAddress() const;

    virtual std::string customGetInterfaceAddress() const;

    std::string _remote_address;
    std::list<std::pair<void*, uint32_t> > _skipped_frames;
};

}

#endif // UDPSERVER_H
