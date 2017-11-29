#include "basecomminterface.h"
#include "debugprint.h"

#include <sys/types.h>
#include <string.h>
#include <stdio.h>

#include "frameworkutils.h"
#include "memorychecker.h"
#include "genericcommdevice.h"

using namespace FrameworkLibrary;

namespace FrameworkLibrary {

class __private_BaseCommInterface
{
    friend class BaseCommInterface;

    __private_BaseCommInterface( BaseCommInterface* intf, const std::string& name ):
        _statistics(intf),
        _secondaryInterfacesMutex( name + "_sec_interfs_mutex" ),
        _name( name )
    {
        _name = name;
        _interface = intf;
        _hide_stats = false;
        _autoDeleteSecondaryInterfaces = true;
        _state = BaseCommInterface::not_configured;
        _mainCommDevice = NULL;
    }

    std::string _printState() const
    {
        switch ( _state )
        {
        case BaseCommInterface::not_configured:
            return "not_configured";
        case BaseCommInterface::configured:
            return "configured";
        case BaseCommInterface::initialized:
            return "initialized";
        case BaseCommInterface::error:
            return "error";
        default:
            return "unk_state";
        }
    }

    bool _switchStateRequest( BaseCommInterface::interface_states new_state )
    {
        if ( _interface->customRequestStateChange( _state, new_state ) )
        {
            _state = new_state;
            _interface->customStateChanged( _state );
            return true;
        }
        return false;
    }

    BaseCommInterface* _interface;
    TransmissionStats _statistics;

    std::set<BaseCommInterface*> _secondaryInterfaces;
    std::set<BaseCommInterface*> _removedSecondaryInterfaces;
    bool _autoDeleteSecondaryInterfaces;
    BaseMutex _secondaryInterfacesMutex;

    BaseCommInterface::interface_states _state;
    bool _hide_stats;

    std::string _name;
    GenericCommDevice* _mainCommDevice;

    static BaseMutex _global_list_mutex;
    static std::set<BaseCommInterface*> _global_list_of_comm_interfaces;

};

}

std::set<BaseCommInterface*> __private_BaseCommInterface::_global_list_of_comm_interfaces;
BaseMutex __private_BaseCommInterface::_global_list_mutex( "BaseCommInterface_global_list_mutex" );

BaseCommInterface::BaseCommInterface( const std::string& n )
{
    _private = new __private_BaseCommInterface(this, n);
    __private_BaseCommInterface::_global_list_mutex.lock();
    __private_BaseCommInterface::_global_list_of_comm_interfaces.insert( this );
    __private_BaseCommInterface::_global_list_mutex.unlock();
    _callbacks = NULL;
}

void BaseCommInterface::setCallbacks(BaseCommInterfaceCallbacks *callback)
{
    _callbacks = callback;
}

BaseCommInterface::~BaseCommInterface()
{
    _private->_state = not_configured;
    __private_BaseCommInterface::_global_list_mutex.lock();
    __private_BaseCommInterface::_global_list_of_comm_interfaces.erase( this );
    __private_BaseCommInterface::_global_list_mutex.unlock();
    _private->_secondaryInterfacesMutex.lock();
    for ( std::set<BaseCommInterface*>::iterator i = _private->_secondaryInterfaces.begin(); i != _private->_secondaryInterfaces.end(); ++i )
    {
        BaseCommInterface* _interface = *i;
        delete MEMORY_CHECKER_DEL( _interface, BaseCommInterface );
    }
    _private->_secondaryInterfaces.clear();
    for ( std::set<BaseCommInterface*>::iterator i = _private->_removedSecondaryInterfaces.begin(); i != _private->_removedSecondaryInterfaces.end(); ++i )
    {
        BaseCommInterface* _interface = *i;
        delete MEMORY_CHECKER_DEL(_interface, BaseCommInterface );
    }
    _private->_removedSecondaryInterfaces.clear();
    if ( _private->_mainCommDevice != NULL )
    {
        _private->_mainCommDevice->shutdown();
        delete MEMORY_CHECKER_DEL( _private->_mainCommDevice, GenericCommDevice );
        _private->_mainCommDevice = NULL;
    }
    delete _private;
    _private = NULL;
}

void BaseCommInterface::hideStatistics(bool hide)
{
    _private->_hide_stats = hide;
}

void BaseCommInterface::printStatistics()
{
    uint64_t biggest_time_s = 0;
    uint64_t tot_r = 0;
    uint64_t tot_s = 0;

    debugPrintUntagged() << "**** BaseCommInterface: data transmitted statistics: *****\n";
    __private_BaseCommInterface::_global_list_mutex.lock();
    for (std::set<BaseCommInterface*>::iterator i = __private_BaseCommInterface::_global_list_of_comm_interfaces.begin(); i != __private_BaseCommInterface::_global_list_of_comm_interfaces.end(); ++i )
    {
        BaseCommInterface* ci = *i;
        if ( !ci->_private->_hide_stats )
        {
            uint64_t time_s = 0;
            uint64_t sec_r = 0;
            uint64_t sec_s = 0;
            uint64_t r = 0;
            uint64_t s = 0;
            uint64_t wrst_r = 0;
            uint64_t wrst_s = 0;
            ci->_private->_statistics.getStats(time_s, sec_r, sec_s, r, s, wrst_r, wrst_s );
            tot_r += r;
            tot_s += s;
            if ( time_s > biggest_time_s )
                biggest_time_s = time_s;

            std::string time_s_str = FrameworkUtils::human_readable_time( time_s );
            std::string sec_r_str = FrameworkUtils::human_readable_number( sec_r, "B/s", true, FrameworkUtils::BINARY_TYPE );
            std::string sec_s_str = FrameworkUtils::human_readable_number( sec_s, "B/s", true, FrameworkUtils::BINARY_TYPE );
            std::string wrst_r_str = FrameworkUtils::human_readable_number( wrst_r, "B/s", true, FrameworkUtils::BINARY_TYPE );
            std::string wrst_s_str = FrameworkUtils::human_readable_number( wrst_s, "B/s", true, FrameworkUtils::BINARY_TYPE );
            std::string r_str = FrameworkUtils::human_readable_number( r, "B", true, FrameworkUtils::BINARY_TYPE );
            std::string s_str = FrameworkUtils::human_readable_number( s, "B", true, FrameworkUtils::BINARY_TYPE );
            debugPrintUntagged() << "Interface [" << ci->getName() << "] running for " << time_s_str <<
                                  " - last second: sent " << sec_s_str << ", received " << sec_r_str <<
                                  " - worst case: sent " << wrst_s_str << ", received " << wrst_r_str <<
                                  " - total byes sent " << s_str << ", received " << r_str << "\n";
        }
    }
    __private_BaseCommInterface::_global_list_mutex.unlock();

    double time_sec = (double)biggest_time_s;
    double send_speed = (time_sec > 0.0 ) ? (double)(tot_s/1024)  / time_sec : 0.0;
    double recv_speed = (time_sec > 0.0 ) ? (double)(tot_r/1024)  / time_sec : 0.0;

    debugPrintUntagged() << "**** overall data statistics for all interfaces: *****\n";

    debugPrintUntagged() << "Data sent: " << FrameworkUtils::human_readable_number(tot_s, "B", true, FrameworkUtils::BINARY_TYPE) <<
                          " (average: " << FrameworkUtils::human_readable_number(send_speed, "B/s", true, FrameworkUtils::BINARY_TYPE) << ") " <<
                          " Data received: " << FrameworkUtils::human_readable_number(tot_r, "B", true, FrameworkUtils::BINARY_TYPE) <<
                          " (average: " << FrameworkUtils::human_readable_number(recv_speed, "B/s",  true, FrameworkUtils::BINARY_TYPE) << ") " <<
                          "Total time: " << FrameworkUtils::human_readable_time( biggest_time_s ) << "\n";
}

std::string BaseCommInterface::getName() const
{
    return _private->_name;
}

int BaseCommInterface::_read(void* data, int length )
{
    int ret = 0;
    if ( length > 0 )
    {
        ret = customRead( data, length );
        if ( ret <= 0 )
            setError();
    }
    return ret;
}

int BaseCommInterface::_write(const void* data, int length )
{
    int ret = 0;
    if ( length > 0 )
    {
        ret = customWrite( data, length );
        if ( ret <= 0 )
            setError();
    }
    return ret;
}

bool BaseCommInterface::canRead() const
{
    return customCanRead();
}

bool BaseCommInterface::canWrite() const
{
    return customCanWrite();
}

bool BaseCommInterface::deviceValid() const
{
    return (_private->_mainCommDevice != NULL ) ? _private->_mainCommDevice->isValid() : false;
}

bool BaseCommInterface::resetInterface()
{
    if ( _private->_mainCommDevice != NULL )
    {
        _private->_switchStateRequest( not_configured );
        if ( _private->_mainCommDevice != NULL )
            _private->_mainCommDevice->shutdown();
        _private->_state = configured;
        return activateInterface();
    }
    return false;
}

int BaseCommInterface::writeDataBool(bool data)
{
    if ( data )
    {
        char t[] = "T";
        return writeData( (void*)t, 1);
    }
    else
    {
        char f[] = "F";
        return writeData( (void*)f, 1);
    }
}

int BaseCommInterface::writeData(uint32_t data)
{
    return writeData( (void*)&data, sizeof(data) );
}

int BaseCommInterface::writeData(int32_t data)
{
    return writeData( (void*)&data, sizeof(data) );
}

int BaseCommInterface::writeData(uint64_t data)
{
    return writeData( (void*)&data, sizeof(data) );
}

int BaseCommInterface::writeData(int64_t data)
{
    return writeData( (void*)&data, sizeof(data) );
}

int BaseCommInterface::writeData(uint16_t data)
{
    return writeData( (void*)&data, sizeof(data) );
}

int BaseCommInterface::writeData(int16_t data)
{
    return writeData( (void*)&data, sizeof(data) );
}

int BaseCommInterface::writeData(uint8_t data)
{
    return writeData( (void*)&data, sizeof(data) );
}

int BaseCommInterface::writeData(int8_t data)
{
    return writeData( (void*)&data, sizeof(data) );
}

int BaseCommInterface::writeData(float data)
{
    return writeData( (void*)&data, sizeof(data) );
}

int BaseCommInterface::writeData(double data)
{
    return writeData( (void*)&data, sizeof(data) );
}

int BaseCommInterface::discardData(uint32_t size)
{
    char buffer [1024];
    uint32_t discarded_data = 0;

    while ( (discarded_data < size) && isActive() )
        discarded_data += readData( buffer, FrameworkUtils_min<uint32_t>( size - discarded_data, 1024 ) );

    return discarded_data;
}

int BaseCommInterface::readData(void *data, int length)
{
    if ( _private->_state == activated )
        return _private->_statistics.addReceivedBytes( _read( data, length ));
    return -1;
}

int BaseCommInterface::writeData(const std::string &data)
{
    uint32_t len = data.length();
    if ( writeData( len ) > 0 )
    {
        if ( len > 0 )
        {
            if ( writeData( (void*)data.c_str(), len ) > 0 )
                return sizeof(len)+len;
        }
        else
            return sizeof(len);
    }
    return -1;
}

int BaseCommInterface::readData(uint32_t &data)
{
    return readData( (void*)&data, sizeof(data) );
}

int BaseCommInterface::readData(int32_t &data)
{
    return readData( (void*)&data, sizeof(data) );
}

int BaseCommInterface::readData(uint64_t &data)
{
    return readData( (void*)&data, sizeof(data) );
}

int BaseCommInterface::readData(int64_t &data)
{    
    return readData( (void*)&data, sizeof(data) );
}

int BaseCommInterface::readData(uint16_t &data)
{
    return readData( (void*)&data, sizeof(data) );
}

int BaseCommInterface::readData(int16_t &data)
{
    return readData( (void*)&data, sizeof(data) );
}

int BaseCommInterface::readData(uint8_t &data)
{
    return readData( (void*)&data, sizeof(data) );
}

int BaseCommInterface::readData(int8_t &data)
{
    return readData( (void*)&data, sizeof(data) );
}

int BaseCommInterface::readData(std::string &data)
{
    int ret = -1;
    uint32_t len = 0;
    if ( readData( len ) > 0 )
    {
        if ( len > 0 )
        {
            // dont use the memory checked for speed reason
            char *tmp = new char[ len ];
            if ( (ret = readData( (void*)tmp, len )) > 0 )
            {
                data.assign(tmp, len);
                delete [] tmp;
                return ret+sizeof(len);
            }
            delete [] tmp;
        }
        else
        {
            data = "";
            ret = sizeof( len );
        }
    }
    return ret;
}

int BaseCommInterface::readDataBool(bool &data)
{
    char b[] = "F";
    if ( readData( (void*)b, 1 ) )
    {
        data = (b[0] == 'T');
        return 1;
    }
    else
        return -1;
}

int BaseCommInterface::readData(float &data)
{
    return readData( (void*)&data, sizeof(data) );
}

int BaseCommInterface::readData(double &data)
{
    return readData( (void*)&data, sizeof(data) );
}

std::string BaseCommInterface::getLastReadAddress() const
{
    return customGetLastReadAddress();
}

void BaseCommInterface::setError()
{
    if ( _private->_state == activated )
        if ( _private->_switchStateRequest( error ) && (_private->_mainCommDevice != NULL ) )
            _private->_mainCommDevice->shutdown();
}

BaseCommInterface::interface_states BaseCommInterface::getState() const
{
    return _private->_state;
}

bool BaseCommInterface::isActive() const
{
    return _private->_state == activated;
}

std::string BaseCommInterface::getAddressString() const
{
    std::string addr = customGetInterfaceAddress();
    if ( _private->_mainCommDevice != NULL )
        addr += "://" + _private->_mainCommDevice->getDeviceAddress();
    return addr;
}

bool BaseCommInterface::setMainCommDevice(GenericCommDevice *comm_device)
{
    if ( (_private->_state == error) || (_private->_state == not_configured) )
    {
        if ( _private->_mainCommDevice == NULL )
        {
            _private->_mainCommDevice = MEMORY_CHECKER_ADD( comm_device, GenericCommDevice );
            _private->_state = configured;
            return true;
        }
        else
            debugPrint( _private->_name, DebugPrint::COMMS_CLASS ) << "The commDevice has already been set!\n";
    }
    else
        debugPrint( _private->_name, DebugPrint::COMMS_CLASS ) << "The interface has already been configured!\n";
    return false;
}

bool BaseCommInterface::initializeInterface()
{
    if ( (_private->_state == initialized) || (_private->_state == activated) )
        return true;

    if ( _private->_state == configured )
    {
        if ( _private->_mainCommDevice->initialize() )
        {
            if ( _private->_switchStateRequest( initialized ) )
                return true;
        }
        else
            debugPrintError( _private->_name ) << "Error: unable to initialize the commDevice [" << _private->_mainCommDevice->getError() << "]\n";
    }
    else
        debugPrintError( _private->_name ) << "Unable to initialize interface (state: " << _private->_printState() << "/" << _private->_state << ")\n";
    return false;
}

bool BaseCommInterface::activateInterface()
{
    if ( _private->_state == activated )
        return true;
    else if ( _private->_state == configured )
        initializeInterface();
    if ( _private->_state == initialized )
        return _private->_switchStateRequest( activated );
    return false;
}

void BaseCommInterface::deactivateInterface()
{
    if ( _private->_state == activated )
    {
        _private->_switchStateRequest( not_configured );
        if ( _private->_mainCommDevice != NULL )
            _private->_mainCommDevice->shutdown();
    }
}


bool BaseCommInterface::hasIncomingData()
{
    return customHasIncomingData();
}

bool BaseCommInterface::waitForIncomingData(int64_t usecs)
{
    return customWaitForIncomingData( usecs );
}

int32_t BaseCommInterface::incomingDataSize()
{
    if ( _private->_state == activated )
        return _private->_mainCommDevice->incomingDataSize();
    return -1;
}

int BaseCommInterface::writeData(const void *data, int length)
{
    if ( _private->_state == activated )
        return _private->_statistics.addSentBytes( _write( data, length ));
    return -1;
}

std::set<BaseCommInterface*> BaseCommInterface::hasIncomingDataSecondary()
{
    std::set<BaseCommInterface*> ret;
    _private->_secondaryInterfacesMutex.lock();
    for ( std::set<BaseCommInterface*>::iterator i = _private->_secondaryInterfaces.begin(); i != _private->_secondaryInterfaces.end(); ++i )
    {
        BaseCommInterface* _interface = *i;
        if ( _interface->hasIncomingData() )
            ret.insert( _interface );
    }
    _private->_secondaryInterfacesMutex.unlock();
    return ret;
}

void BaseCommInterface::setAutoDelete(bool d)
{
    _private->_autoDeleteSecondaryInterfaces = d;
}

std::set<BaseCommInterface*> BaseCommInterface::purgeRemovedSecondaryInterfaces()
{
    _private->_secondaryInterfacesMutex.lock();
    std::set<BaseCommInterface*> ret = _private->_removedSecondaryInterfaces;
    _private->_removedSecondaryInterfaces.clear();
    _private->_secondaryInterfacesMutex.unlock();
    return ret;
}

std::set<BaseCommInterface *> BaseCommInterface::getSecondaryInterfaces()
{
    std::set<BaseCommInterface *> ret;
    _private->_secondaryInterfacesMutex.lock();
    ret = _private->_secondaryInterfaces;
    _private->_secondaryInterfacesMutex.unlock();
    return ret;
}

bool BaseCommInterface::removeSecondaryInterface(BaseCommInterface *_interface)
{
    bool ret = false;
    _private->_secondaryInterfacesMutex.lock();
    if ( _private->_secondaryInterfaces.find( _interface ) != _private->_secondaryInterfaces.end() )
    {
        _private->_secondaryInterfaces.erase( _interface );
        if ( _callbacks != NULL )
            _callbacks->customSecondaryInterfaceLost( _interface );
        if ( _private->_autoDeleteSecondaryInterfaces )
            delete MEMORY_CHECKER_DEL( _interface, BaseCommInterface );
        else
            _private->_removedSecondaryInterfaces.insert( _interface );
        _private->_secondaryInterfacesMutex.unlock();
        ret = true;
    }
    else
        _private->_secondaryInterfacesMutex.unlock();
    return ret;
}

int32_t BaseCommInterface::countSecondaryInterfaces()
{
    int32_t ret = 0;
    _private->_secondaryInterfacesMutex.lock();
    ret = _private->_secondaryInterfaces.size();
    _private->_secondaryInterfacesMutex.unlock();
    return ret;
}

int BaseCommInterface::customRead(void *, int)
{
    return -1;
}

int BaseCommInterface::customWrite(const void *, int)
{
    return -1;
}

void BaseCommInterface::customStateChanged(BaseCommInterface::interface_states)
{
}

bool BaseCommInterface::customCanRead() const
{
    return (_private->_mainCommDevice != NULL ) ? _private->_mainCommDevice->canRead() : false;
}

bool BaseCommInterface::customCanWrite() const
{
    return (_private->_mainCommDevice != NULL ) ? _private->_mainCommDevice->canWrite() : false;
}

bool BaseCommInterface::customHasIncomingData()
{
    if ( _private->_state == activated )
        return _private->_mainCommDevice->hasIncomingData();
    return false;
}

bool BaseCommInterface::customWaitForIncomingData(int64_t usecs)
{
    if ( _private->_state == activated )
        return _private->_mainCommDevice->waitIncomingData( usecs );
    return false;
}

std::string BaseCommInterface::customGetInterfaceAddress() const
{
    return "address-not-known";
}

std::string BaseCommInterface::customGetLastReadAddress() const
{
    return "";
}

bool BaseCommInterface::addSecondaryInterface(BaseCommInterface *_interface)
{
    bool ret = false;
    _private->_secondaryInterfacesMutex.lock();
    if ( _private->_secondaryInterfaces.find( _interface ) == _private->_secondaryInterfaces.end() )
    {
        _private->_secondaryInterfaces.insert( MEMORY_CHECKER_ADD(_interface, BaseCommInterface) );
        _private->_secondaryInterfacesMutex.unlock();
        if ( _callbacks != NULL )
            _callbacks->customSecondaryInterfaceAdded( _interface );
        ret = true;
    }
    else
        _private->_secondaryInterfacesMutex.unlock();
    return ret;
}



BaseCommInterfaceCallbacks::~BaseCommInterfaceCallbacks()
{
}
