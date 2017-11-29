#ifndef GENERICCOMMDEVICE_H
#define GENERICCOMMDEVICE_H

#include <common_defs.h>
#include <string>

namespace FrameworkLibrary{

/** @brief the comm_device ID
 */
typedef uint32_t GenericCommDeviceId;

class __privateGenericCommDevice;

/** @brief Implement a generic communication device
 *
 * This class is part of the BaseCommInterface communication infrastructure and it abstacts all the priciples and states needed for a communication device.
 * A specific implementation is needed when you want to use it.
 *
 * This class defines all the basic states and methods (see SocketCommDevice for an example) to implement your device.
 * This class implement the communication channel but not the protocol and/or behaviour. The behaviour is coded in the BaseCommInterface class which will
 * use this device to perform actual I/O and states setting.
 *
 *
 * @review Implement requirements
 *
 */
class DLLEXPORT GenericCommDevice
{
public:
    virtual ~GenericCommDevice();

    /** @brief return string representing the device ddress
     * @since 3.3
     *  @return string with the address
     */
    std::string getDeviceAddress() const;

    /** @brief Return the unique channel ID
     * @return ID device
     */
    GenericCommDeviceId getId() const;

    /** @brief Return last error
     * @return error string (device dependent)
     */
    std::string getError() const;

    /** @brief This is for debug
     * @return return a device dependent debug string
     */
    virtual std::string print_debug() = 0;

    /** @brief Check if there are data ready to be read. Not blocking.
     *
     * Should return true if something needs to be read. Can return false if nothing is to be read or if an
     * error has eccurred or a signal has been delivered. It's usually a wrapper to waitIncomingData().
     * @return true if something is ready to be read, false if nothing is to be read
     */
    virtual bool hasIncomingData() const;

    /** @brief Waits until there are data to be read. Blocks only for specified time.
     *
     * Should return true if something needs to be read. Can return false if nothing is to be read or if an
     * error has eccurred or a signal has been delivered.
     * @param usecs microseconds to wait before the timeout expires. if set to 0, will not wait.
     * @return true if something is ready to be read, false if nothing is to be read
     */
    virtual bool waitIncomingData( int64_t usecs ) const = 0;

    /** @brief Return number of bytes ready to be read
     *
     * If you read up to the number of bytes returned by this method, the read will not be blocking.
     * @note TCP server sockets always return -1
     * @note UDP sockets are system dependent. On Linux, it will only return the size of the next packet, on Windows the total number of queue bytes, even if they cannot be read with a single read.
     * @warning UDP sockets are a bitch. Do not rely on this function.
     * @return number of bytes to be read.
     */
    virtual int32_t incomingDataSize() const = 0;

    /** @brief initialize the device
     * @return true if device is initialized.
     */
    bool initialize();

    /** @brief Close device
     */
    void shutdown();

    /**
     * @brief Check if the device is valid
     * @return true if valid
     */
    bool isValid() const;

    /** @brief Check if the channel is initialized (does not initialize if it is not)
     * @return true if it is initialized
     */
    bool isInitialized() const;

    /** @brief Check if the channel can read data
     * @return true if the channel can read data
     */
    bool canRead() const;

    /** @brief Check if the channel can send data
     * @return true if the channel can send data
     */
    bool canWrite() const;

protected:
    /** @brief Create a new generic comm device
     * @param can_read if the device can read
     * @param can_write if the device can write
     */
    GenericCommDevice( bool can_read = true, bool can_write = true );

    /** @brief Set an error string
     * @param err The error string
     */
    void setErrorString( const std::string& err );

    /** @brief perform your own initialization
     * @return true if initialization is ok, false otherwise
     */
    virtual bool customInitialize() = 0;

    /** @brief shutdown comm device
     */
    virtual void customShutdown() = 0;

    /** @brief check if comm device is valid
     * @return true if it's valid, false otherwise
     */
    virtual bool customIsValid() const = 0;

    /** @brief Get device address as human readable string
     * @return human readable address string
     */
    virtual std::string customGetDeviceAddress() const = 0;

    /** @brief change read capability
     * @param cr true if the comm device can read, false otherwise
     */
    void setCanRead( bool cr );

    /** @brief change write vapability
     * @param cw true if the comm device can write, false otherwise
     */
    void setCanWrite( bool cw );

private:
    __privateGenericCommDevice* _private;

};

}

#endif // GENERICCOMMDEVICE_H
