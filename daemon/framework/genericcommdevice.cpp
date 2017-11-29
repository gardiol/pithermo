#include "genericcommdevice.h"
#include <stdio.h>
#include "debugprint.h"
#include <basemutex.h>

using namespace FrameworkLibrary;

namespace FrameworkLibrary{

class __privateGenericCommDevice
{
    friend class GenericCommDevice;

    bool _canRead;
    bool _canWrite;
    bool _isInitialized;
    std::string _error_string;
    GenericCommDeviceId _id;

    static BaseMutex _mutex;
    static GenericCommDeviceId base_id;

};

BaseMutex __privateGenericCommDevice::_mutex("GenericCommDeivce_global_mutex");
GenericCommDeviceId __privateGenericCommDevice::base_id = 1;

}

GenericCommDevice::GenericCommDevice(bool can_read, bool can_write )
{
    _private = new __privateGenericCommDevice;
    _private->_canRead = can_read;
    _private->_canWrite = can_write;
    _private->_isInitialized = false;
    _private->_error_string = "no errors";
    _private->_id = _private->base_id++;
    __privateGenericCommDevice::_mutex.lock();
    _private->_id = __privateGenericCommDevice::base_id++;
    __privateGenericCommDevice::_mutex.unlock();
}

GenericCommDevice::~GenericCommDevice()
{
    delete _private;
    _private = NULL;
}

std::string GenericCommDevice::getDeviceAddress() const
{
    return customGetDeviceAddress();
}

GenericCommDeviceId GenericCommDevice::getId() const
{
    return _private->_id;
}

std::string GenericCommDevice::getError() const
{
    return _private->_error_string;
}

bool GenericCommDevice::hasIncomingData() const
{
    return waitIncomingData(0);
}

bool GenericCommDevice::initialize()
{
    if ( !_private->_isInitialized )
        if ( customInitialize() )
            _private->_isInitialized = true;
    return _private->_isInitialized;
}

void GenericCommDevice::shutdown()
{
    if ( _private->_isInitialized )
    {
        customShutdown();
        _private->_isInitialized = false;
    }
}

bool GenericCommDevice::isValid() const
{
    if ( _private->_isInitialized )
        return customIsValid();
    return false;
}

bool GenericCommDevice::isInitialized() const
{
    return _private->_isInitialized;
}

bool GenericCommDevice::canRead() const
{
    return _private->_canRead;
}

bool GenericCommDevice::canWrite() const
{
    return _private->_canWrite;
}

void GenericCommDevice::setErrorString(const std::string &err)
{
    _private->_error_string = err;
    debugPrint( "GenericCommDevice", DebugPrint::COMMS_CLASS ) << "CommDeviceError: " << err << "\n";
}

void GenericCommDevice::setCanRead(bool cr)
{
    _private->_canRead = cr;
}

void GenericCommDevice::setCanWrite(bool cw)
{
    _private->_canWrite = cw;
}
