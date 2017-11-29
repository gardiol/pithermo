#ifndef BASECOMMINTERFACE_H
#define BASECOMMINTERFACE_H

#include "common_defs.h"

#include "basethread.h"
#include "basemutex.h"

#include "transmissionstats.h"
#include "genericcommdevice.h"

#include <set>

namespace FrameworkLibrary {

/** @brief Get callback notifications from BaseCommInterface
 *
 * This is the callback interface for the BaseCommInterface class, needed to receive callback events to your class.
 *
 * Implement this interface in your class to track when a secondary interface has been added or removed:
 * - customSecondaryInterfaceAdded()
 * - customSecondaryInterfaceLost()
 *
 * Then call the BaseCommInterface::setCallbacks() method passing a pointer to your class.
 *
 */
class DLLEXPORT BaseCommInterfaceCallbacks
{
    friend class BaseCommInterface;

public:
    virtual ~BaseCommInterfaceCallbacks();

protected:
    /** @brief called when a secondary interface has disconnected
     * @param interface_ the interface already disconnected
     *
     *
     *
     */
    virtual void customSecondaryInterfaceLost( BaseCommInterface* interface_) = 0;

    /** @brief called when a secondary interface has sconnected
     * @param interface_ the interface already connected
     *
     *
     *
     */
    virtual void customSecondaryInterfaceAdded( BaseCommInterface* interface_) = 0;
};

class __private_BaseCommInterface;

/** @brief Implements an I/O channel over a generic device. It is thread-safe.
  *
  * # Description #
  * This class implements, by abstraction, a generic I/O channel. Using this classs you can derive specific implementations for any kind of sequential I/O.
  * For example, there is an implementation for UDP and TCP sockets, but it can be used for serial ports or other protocols.
  * The main mechanism is a read/write interface, so any protocol or hardware which can be abstracted as a read/write channel can be
  * implemented with this class.
  *
  * The overall approach is based on two components:
  * - BaseCommInterface is the base class which provides the generic interfaces for read/write, initialize, status check and whatnot. This is where
  * the protocol is implemented.
  * - GenericCommDevice is the base class which implements the hardware access interface, this is where the actual interaction happens.
  *
  * These two classes interacts quite tightly, and by deriving them, you can complete the circle. For example, the UdpSocket class derives from
  * BaseCommInterface, which implements the peculiarities of the UDP protocol, while the SocketCommDevice derives from GenericCommDevice and
  * implements the specifics for accessing the UDP/IP stack of the operating system.
  *
  * # Composites interfaces #
  *
  * There are some circumstances in which various interfaces depend on a single interface, call it the master or the parent interface. A good
  * example is a TCP server, where the server TCP socket is used only to accept new connection, then dedicated TCP sockets are created
  * for each connected client. In this case, the "master" interface has "secondary" interfaces connected to it and can be managed directly by
  * the BaseCommInterface. The TCP server will then register a new secondary interface for each connected client.
  *
  * # States and initialization #
  *
  * The interface lifecycle goes between a series of states:
  * - interface_states::not_configured: the interface is not configured (yet), this is the initial state.
  * - interface_states::configured: the interface has been configured, and is ready to be connected (usually after calling setCommDevice()).
  * - interface_states::initialized: the interface is connected (usually, after calling initializeInterface())
  * - interface_states::activated: the interface is ready to receive / transmit data (usually, after calling activateInterface())
  * - interface_states::error: an error has occurred, the interface is not connected or ready anymore. You must assume the GenericCommDevice item is not
  * valid anymore. To get out of this state, you must call setCommDevice() again. An error state can be "forced" with setError(). You can try to call resetCommDevice() to
  * try to reinitialize the GenericCommDevice, but it is not guaranteed to succeed. If it does, the next state will be configured.
  *
  * States life_ interface_states::not_configured -> interface_states::configured -> interface_states::initialized -> interface_states::activated -> interface_states::error -> interface_states::configured
  *
  * # Usage #
  *
  * After the interface is in activated state, you can use it to read and write data.
  *
  *
  *
  */
class DLLEXPORT BaseCommInterface
{
    friend class __private_BaseCommInterface;
public:
    /** @brief print statistics on all the interfaces
     */
    static void printStatistics();

public: // Properties access

    /** @brief Initialize the callbacks
     * @param callback The interface to use as callback
     *
     *
     *
     */
    void setCallbacks( BaseCommInterfaceCallbacks* callback );

    /** @brief Get interface name
     * @return name of interface
     *
     *
     *
     */
    std::string getName() const;

    /** @brief Check if this interface can read data
     * @return true is the interface can be read
     *
     *
     *
     */
    bool canRead() const;

    /** @brief Check if this interface can send data
     * @return true if the channel can be written
     *
     *
     *
     */
    bool canWrite() const;

    /** @brief Check if the interface is valid
     * @return true if the interface is valid
     *
     *
     *
     */
    bool deviceValid() const;

    /** @brief Reset the device on which the interface operates.
     * @since 3.4
     *
     * Call this after the interface is in error, tipically, which will cause the device to reset.
     * If reset is successful, the interface state goes to configured.
     * @return reset successful
     *
     *
     *
     */
    bool resetInterface();

public: // Gestione del ciclo di vita
    /** @brief possible states of the interface
     */
    enum interface_states { not_configured = 0, /**< Interface not initialized  */
                            configured = 1,  /**< Interface ready to be initialized (es IP set, socket created)  */
                            initialized = 2,  /**< Interface initialized (es bind)  */
                            activated = 3, /**< Interface ready to use (es connect done)  */
                            error = 4}; /**< Interface error  */

protected:
    /** @brief Create a new interface
     * @param n - Name of the interface
     *
     *
     *
     */
    BaseCommInterface( const std::string& n );

    /** @brief Set the device on which the interface operates.
     * @param comm_device device to use for this interface
     * @return if false, you must take care to delete comm_device. If true, you MUST NOT delete comm_device, will be managed internally.
     */
    bool setMainCommDevice( GenericCommDevice* comm_device );

public:
    /** @brief Initialize interface operations
     * @return true if initialization has been done and state is now initialized
     *
     *
     *
     */
    bool initializeInterface();

    /** @brief Activate interface
     * @return true if activation is successful and state is now activated
     *
     *
     *
     **/
    bool activateInterface();

    /** @brief Deactivate the interface
     *
     * Deactivate/close an interface. It will go to not_configured state.
     *
     *
     *
     */
    void deactivateInterface();

    /** @brief Force an error on the interface
     *
     * It is useful to force a disconnestion or notify external entities that this interface is not valid.
     *
     *
     *
     */
    void setError();

    /** @brief Get current interface state
     * @return current state
     *
     *
     *
     */
    interface_states getState() const;

    /** @brief Check if the interface is ready to communicate
     * @return true if interface is ready, which means state is activated
     *
     *
     *
     */
    bool isActive() const;

    /** @brief Get the "address" string. The meaning depends on the device and interface.
     * @since 3.3
     * @return String representing the address for the device and interface
     *
     *
     *
     */
    std::string getAddressString() const;

    virtual ~BaseCommInterface();

    /** @brief Enable / disable statistics reporting for this interface.
     *
     * Use this in case statistics reported by your interface is meaningless.
     * @param hide true by default, set false to re-enable statistic reports.
     *
     *
     *
     */
    void hideStatistics( bool hide = true );


public: // Operazioni sull'interfaccia (read/write e accessorie)
    /**
     * @brief Check (non blocking) is there is any data to read on the main interface
     * @return true if data is available on the main interface
     *
     *
     *
     *
     */
    bool hasIncomingData();

    /**
     * @brief Block until there is any data to read on the main interface
     * @param usecs how long to wait for (-1 for unlimited, default)
     * @return true if data is available on the main interface
     *
     *
     *
     *
     *
     */
    bool waitForIncomingData( int64_t usecs = -1);

    /**
     * @brief Checks (non blocking) all the seconday interfaces if they have any data to read
     * @return List of seconday interfaces with data to be read
     *
     *
     *
     *
     *
     */
    std::set<BaseCommInterface*> hasIncomingDataSecondary();

    /**
      * @brief Set the automatic deletion of secondary interfaces
      *
      * If auto delete is set to true (default), any secondary interface is deleted when
      * it is removed (call to removeSecondaryInterface()). If it is set to false,
      * it is moved to a list of "removed" interfaces after being deactivated. You can
      * then get that list with purgeRemovedSecondaryInterfaces().
      * In this case, please remember to delete the interfaces yourself and periodically purge the removed list.
      * Please note that switching this from false to true will NOT purge the removed interfaces list nor delete them.
      * Also, please note that when this interface is destroyed all secondary (removed or not) interfaces are deleted!
      * So you don't need to call purgeRemovedSecondaryInterfaces() and delete manually on interface deletion.
      * @param d if true, secondary interfaces gets deleted automatically or you shall delete them.
      *
      *
      *
      */
    void setAutoDelete( bool d );

    /** @brief Get the list of all removed secondary interfaces. Each one is already disactivated.
     *
     * Note that the main interface destructor will clear this list and delete all secondary removed interfaces, so
     * you don't have to worry about memory leaks, at least in this case.
     * @return list of interfaces which should be deleted as soon as possibile to free memory.
     *
     *
     *
     *
     */
    std::set<BaseCommInterface*> purgeRemovedSecondaryInterfaces();

    /**
      * @brief List all secondary interfaces
      * @return list of all secondary interfaces
     *
     *
     *
      */
    std::set<BaseCommInterface*> getSecondaryInterfaces();

    /**
     * @brief Return number of bytes ready for the next read on the main interface
     * @return number of bytes to be read, on 32bits. -1 in caso di errore.
     *
     *
     *
     */
    int32_t incomingDataSize();

    /**
     * @brief Write (raw) data on the main interface
     * @param data puntatore ai dati da inviare
     * @param length lunghezza in byte dei dai da inviare
     * @return the number of bytes actually written
     *
     *
     *
     */
    int writeData( const void* data, int length );

    /**
     * @brief Write a bool
     * @param data value to send
     * @return the number of bytes actually written
     *
     *
     *
     */
    int writeDataBool( bool data );
    /**
     * @brief Write a uint32_t
     * @param data value to send
     * @return the number of bytes actually written
     *
     *
     *
     */
    int writeData( uint32_t data );
    /**
     * @brief Write a int32_t
     * @param data value to send
     * @return the number of bytes actually written
     *
     *
     *
     */
    int writeData( int32_t data );
    /**
     * @brief Write a uint64_t
     * @param data value to send
     * @return the number of bytes actually written
     *
     *
     *
     */
    int writeData( uint64_t data );
    /**
     * @brief Write a int64_t
     * @param data value to send
     * @return the number of bytes actually written
     *
     *
     *
     */
    int writeData( int64_t data );

    /**
     * @brief Write a string
     * @note You must use the simmetrical readData( std::string& ) to read it!
     *
     * The format is: [uint32_t:len][string]. first the string length on a unsigned 32bit integer, then the string.
     * The terminator is NOT sent and it is NOT counted in the len.
     * @param data string to send
     * @return the number of bytes actually written
     *
     *
     *
     */
    int writeData( const std::string& data );
    /**
     * @brief Write a uint16_t
     * @param data value to send
     * @return the number of bytes actually written
     *
     *
     *
     */
    int writeData( uint16_t data );
    /**
     * @brief Write a int16_t
     * @param data value to send
     * @return the number of bytes actually written
     *
     *
     *
     */
    int writeData( int16_t data );
    /**
     * @brief Write a uint8_t
     * @since 3.4
     * @param data value to send
     * @return the number of bytes actually written
     *
     *
     *
     */
    int writeData( uint8_t data );
    /**
     * @brief Write a int8_t
     * @since 3.4
     * @param data value to send
     * @return the number of bytes actually written
     *
     *
     *
     */
    int writeData( int8_t data );

    /**
     * @brief Write a float
     * @since 3.4
     * @param data value to send
     * @return the number of bytes actually written
     *
     *
     *
     */
    int writeData( float data );

    /**
     * @brief Write a double
     * @since 3.4
     * @param data value to send
     * @return the number of bytes actually written
     *
     *
     *
     */
    int writeData( double data );

    /** @brief Discard data from the main interface
     * @since 3.4
     *
     * The data will be read and discarded. This will consume all the requested data, if possible.
     * It might happen that less than the requested data is consumes if the interface is
     * deactiveted or goes into error state.
     * @param size how many bytes discard
     * @return the number of discarded bytes.
     *
     *
     *
     */
    int discardData( uint32_t size );

    /**
     * @brief Read (raw) data from the main interface
     * @param data pointer to store the data to
     * @param length length if data to receive
     * @return the number of bytes actually read
     *
     *
     *
     */
    int readData( void* data, int length );

    /**
     * @brief Read an uint32_t
     * @param data the value to read
     * @return the number of bytes actually read
     *
     *
     *
     */
    int readData( uint32_t& data );
    /**
     * @brief Read an int32_t
     * @param data the value to read
     * @return the number of bytes actually read
     *
     *
     *
     */
    int readData( int32_t& data );
    /**
     * @brief Read an uint64_t
     * @param data the value to read
     * @return the number of bytes actually read
     *
     *
     *
     */
    int readData( uint64_t& data );
    /**
     * @brief Read an int64_t
     * @param data the value to read
     * @return the number of bytes actually read
     *
     *
     *
     */
    int readData( int64_t& data );
    /**
     * @brief Read an uint16_t
     * @param data the value to read
     * @return the number of bytes actually read
     *
     *
     *
     */
    int readData( uint16_t& data );
    /**
     * @brief Read an int16_t
     * @param data the value to read
     * @return the number of bytes actually read
     *
     *
     *
     */
    int readData( int16_t& data );
    /**
     * @brief Read an uint8_t
     * @since 3.4
     * @param data the value to read
     * @return the number of bytes actually read
     *
     *
     *
     */
    int readData( uint8_t& data );
    /**
     * @brief Read an int8_t
     * @since 3.4
     * @param data the value to read
     * @return the number of bytes actually read
     *
     *
     *
     */
    int readData( int8_t& data );
    /**
     * @brief Read a string
     * @note Use this to read string written by writeData( const std::string& ).
     *
     * The format is: <4 bytes, unsigned length><string chars, unterminated>
     * First the string length on 4 bytes as unsigned 32bit integer, then the string characters, unterminated.
     * @param data the string to read
     * @return the number of bytes actually read
     *
     *
     *
     */
    int readData( std::string& data );
    /**
     * @brief Read a bool
     * @param data the value to read
     * @return the number of bytes actually read
     *
     *
     *
     */
    int readDataBool( bool& data );

    /**
     * @brief Read a float
     * @since 3.4
     * @param data the value to read
     * @return the number of bytes actually read
     *
     *
     *
     */
    int readData( float& data );

    /**
     * @brief Read a double
     * @since 3.4
     * @param data the value to read
     * @return the number of bytes actually read
     *
     *
     *
     */
    int readData( double& data );

    /** @brief Return the remote socket address from last read
     * @since 4.0.3
     *
     * This function will return the address string associated to the last read operation.
     * The meaning of this depends on the underlying protocol.
     * For example, for TCP clients, this is the IP+port of the server side of the connection, if connected, and will not
     * change until the socket is disconnected/reconnected. For TCP server, it is the IP+port of the last connected client.
     * For UDP, is the IP+port of the last client who sent us a packet, which can vary after each read
     * is performed.
     */
    std::string getLastReadAddress() const;

    /**
      * @brief Remove an secondary interface from this interface (and delete it if autoDelete is true)
      *
      * The removed interface is deleted if autoDelete is set to true (see BaseCommInterface::setAutoDelete())
      * @param _interface interface to remove
      * @return true if the interface has been removed, false if it's not a valid secondary interface.
     *
     *
     *
      */
    bool removeSecondaryInterface( BaseCommInterface* _interface );

    /**
      * @brief Number of secondary interfaces
      * @return number of secondary interfaces
     *
     *
     *
      */
    int32_t countSecondaryInterfaces();



protected:
    /** @brief read data from the interface
     *
     * This method will read up to the requested number of bytes.
     * How much is actually read depends on the underlying comm device.
     * This will call internally customRead()
     * @param data pointer to store the data
     * @param length size in bytes to read
     * @return the number of bytes read
     *
     *
     *
     */
    int _read(void* data, int length );

    /** @brief write data to the interface
     *
     * This method will write up to the requested number of bytes.
     * How much is actually writted depends on the underlying comm device.
     * This will call internally customWrite()
     * @param data pointer to source data
     * @param length size in bytes to write
     * @return the number of bytes written
     *
     *
     *
     */
    int _write(const void* data, int length );

    /**
      * @brief Add an secondary interface to this interface
      * @param _interface interface to add
      * @return true if the secondary interface has been added, false if it's duplicated.
     *
     *
     *
      */
    bool addSecondaryInterface( BaseCommInterface* _interface );

protected: // Metodi virtuali e virtuali puri da ridefinire nelle classi figlie

    /** @brief Perform a read
     *
     * This method is implemented as a failed read (return -1), reimplement in your derived class to support read operations.
     * @param data where to store the read data
     * @param length size in bytes of data to read
     * @return bytes actually read, 0 or -1 on errors
     *
     *
     *
     */
    virtual int customRead( void* data, int length);

    /** @brief Perform a write
     *
     * This method is implemented as a failed write (return -1), reimplement in your derived class to support write operations.
     * @param data where to source the data from
     * @param length size in bytes of data to write
     * @return bytes actually written, 0 or -1 on errors
     *
     *
     *
     */
    virtual int customWrite( const void* data, int length);

    /** @brief Get ready to change state of the interface
     *
     * This is called whenever the interface must change it's state. Derive in your class to accept or refuse the
     * state change.
     * @param previous_state the current state of the interface
     * @param requested_state the state the interface should switch to
     * @return return true to accept and confirm the state change, return false to reject it and remain in the current state.
     *
     *
     *
     */
    virtual bool customRequestStateChange( interface_states previous_state, interface_states requested_state ) = 0;

    /** @brief do your stuff after the state of the interface has changed.
     *
     * This is called after each successful state change (after a call to customRequestStateChange() which returned true).
     * @param current_state the new state of the interface
     *
     *
     *
     */
    virtual void customStateChanged( interface_states current_state );

    /** @brief check if the interface has the capability to read data
     *
     * This method return whether the interface can read data. This is to be intended as a general capability (like a TCP socket
     * can read data) and not if the interface can read <b>right now</b> (like a connected TCP socket).
     * By default the interface cannot read data.
     * @return true if the interface can read data, false otherwise
     *
     *
     *
     */
    virtual bool customCanRead() const;

    /** @brief check if the interface has the capability to write data
     *
     * This method return whether the interface can write data. This is to be intended as a general capability (like a TCP socket
     * can write data) and not if the interface can write <b>right now</b> (like a connected TCP socket).
     * By default the interface cannot write data.
     * @return true if the interface can write data, false otherwise
     *
     *
     *
     */
    virtual bool customCanWrite() const;

    /** @brief check if the interface has incoming data ready to be read
     *
     * This method shall NOT be blocking and shall return immediately.
     * @return true if data is available to be read right now, false otherwise.
     * @warning The default implementation should be fine for most interfaces. Derive this ONLY if you really need it.
     *
     *
     *
     *
     *
     */
    virtual bool customHasIncomingData();

    /** @brief wait for data to be avilable to be read for a given amount of time
     *
     * This method shall block for maximum usecs microseconds.
     * @param usecs how many microseconds to wait, at most.
     * @return true if data is available to be read right now, false if the timeout has expired and no data is available to be read.
     * @warning The default implementation should be fine for most interfaces. Derive this ONLY if you really need it.
     *
     *
     *
     *
     *
     */
    virtual bool customWaitForIncomingData(int64_t usecs);

    /** @brief get the interface address as a human-readable string
     *
     * This is mostly required only for debug and statistics.
     * @return the interface address in a human readable form.
     * @warning The default implementation should be fine for most interfaces. Derive this ONLY if you really need it.
     *
     *
     *
     */
    virtual std::string customGetInterfaceAddress() const;

    /** @brief get the address associated to the last read operation
     *
     * It's up to you to implement this.
     */
    virtual std::string customGetLastReadAddress() const;

    // This is to provide a callback interface:
    BaseCommInterfaceCallbacks* _callbacks; /**< Callback interface-class for this interface  */

private:
    __private_BaseCommInterface* _private;

};

}

#endif // BASECOMMINTERFACE_H
