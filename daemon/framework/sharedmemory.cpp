#include "sharedmemory.h"

#include <sstream>
#include <map>
#include <stdio.h>
#include <stdlib.h>

#include "frameworkutils.h"
#include "debugprint.h"

using namespace FrameworkLibrary;

namespace FrameworkLibrary {

class __SharedMemoryPrivate
{
public:
    __SharedMemoryPrivate(const std::string &name,
                          uint32_t size,
                          SharedMemoryKey key):
        _key(key),
        _size(size),
        _ptr(nullptr),
        _canWrite(0),
        _name(name)
    {
    }

    virtual ~__SharedMemoryPrivate();

    virtual void deleteShm() = 0;

    bool canWrite() const
    {
        return _canWrite == 1;
    }

    bool isReady() const
    {
        return _ptr != nullptr;
    }

    void* getPtr() const
    {
        return _ptr;
    }

    std::string getName() const
    {
        return _name;
    }

    uint32_t getSize() const
    {
        return _size;
    }

    SharedMemoryKey getKey() const
    {
        return _key;
    }

protected:
    SharedMemoryKey _key;
    uint32_t _size;
    void* _ptr;
    uint32_t _canWrite;
    std::string _name;
};

__SharedMemoryPrivate::~__SharedMemoryPrivate()
{
}


#if defined(FRAMEWORK_PLATFORM_WINDOWS)

#include <tchar.h>
#include <windows.h>

class __SharedMemoryPrivateWindows:
        public __SharedMemoryPrivate
{
public:
    __SharedMemoryPrivateWindows(const std::string &name,
                                 SharedMemoryKey key,
                                 uint32_t size,
                                 SharedMemory::ShmMode mode):
        __SharedMemoryPrivate( name, size, key ),
        _area_id(0)
    {
        // On Windows read-only does not work:
        _canWrite = 1;
        // Build string key:
        std::stringstream ss;
        ss << key;
        std::string key_name = "LIBFRAMEWORK-" + ss.str();
        // Try to create the mapping:
        _area_id = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, size, LPSTR(key_name.c_str()));
        // Mapping is valid only if it ERROR_ALREADY_EXISTS or it can be created:
        DWORD last_error = GetLastError();
        if ( (last_error == ERROR_ALREADY_EXISTS) ||
             (mode == SharedMemory::read_write_create_shm) )
        {
            if ((_ptr = (uint32_t*)MapViewOfFile(_area_id, FILE_MAP_ALL_ACCESS, 0, 0, 0)) == NULL )
            {
                CloseHandle( _area_id );
                debugPrintError( _name ) << "Error attaching to mapped file: " << FrameworkUtils::get_errno_string("") << "\n";
            }
        }
        else
        {
            if ( _area_id != 0 )
                CloseHandle( _area_id );
            debugPrintError( _name ) << "Error opening mapped file: " << FrameworkUtils::get_errno_string("") << "\n";
        }
    }

    ~__SharedMemoryPrivateWindows()
    {
        if ( _ptr != NULL )
            CloseHandle( _area_id );
    }

    void deleteShm()
    {
        if ( _ptr != NULL )
        {
            CloseHandle( _area_id );
            _ptr = NULL;
        }
    }

    static void purgeShm(SharedMemoryKey key)
    {
        std::stringstream ss;
        ss << key;
        std::string key_name = "LIBFRAMEWORK-" + ss.str();
        HANDLE hndl = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 1, LPSTR(key_name.c_str()));
        if (hndl != NULL)
            CloseHandle(hndl);
    }

public:
    HANDLE _area_id;
};


#if defined(FRAMEWORK_PLATFORM_WINDOWS_RTX64)
#include "rtapi.h"
#if defined(UNDER_RTSS)
#define UNDER_RTSS_UNSUPPORTED_CRT_APIS
#include "rtssapi.h"
#endif

//******* WINDOWS RTX SPECIFIC CODE ***********

class __SharedMemoryPrivateRTX:
        public __SharedMemoryPrivate
{
public:
    __SharedMemoryPrivateRTX(const std::string &name,
                             SharedMemoryKey key,
                             uint32_t size,
                             SharedMemory::ShmMode mode):
        __SharedMemoryPrivate( name, size, key ),
        _shm_handle(NULL)
    {
        if ( mode != SharedMemory::read_only_shm )
            _canWrite = 1;

        std::stringstream ss;
        ss << key;
        std::string mapped_name = "LIBFRAMEWORK-" + ss.str();
        _shm_handle = RtOpenSharedMemory( _canWrite==1 ? SHM_MAP_WRITE : SHM_MAP_READ, 0, LPCSTR(mapped_name.c_str()), &_ptr );
        // check if shm area already exists
        if (_shm_handle == NULL)
        {
            // if not exist, verify if it can be created
            if (mode == SharedMemory::read_write_create_shm)
                _shm_handle = RtCreateSharedMemory( _canWrite==1 ? PAGE_READWRITE : PAGE_READONLY, 0, size, LPCSTR(mapped_name.c_str()), &_ptr );
        }

        if (_shm_handle != NULL)
        {
            if (_ptr == NULL)
            {
                RtCloseHandle(_shm_handle);
                _shm_handle = NULL;
                debugPrintError( _name ) << "Error attaching to shared memory: " << FrameworkUtils::get_errno_string("") << "\n";
            }
        }
        else
            debugPrintError( _name ) << "Error opening shared memory: " << FrameworkUtils::get_errno_string("") << "\n";
    }

    ~__SharedMemoryPrivateRTX()
    {
        if (_shm_handle != NULL)
        {
            RtCloseHandle(_shm_handle);
            _shm_handle = NULL;
        }
    }

    bool isReady() const
    {
        return _shm_handle != NULL ;
    }

    void deleteShm()
    {
        if ( _shm_handle != NULL )
        {
            RtCloseHandle(_shm_handle);
            _shm_handle = NULL;
        }
    }

    static void purgeShm(SharedMemoryKey key)
    {
        std::stringstream ss;
        ss << key;
        std::string mapped_name = "LIBFRAMEWORK-" + ss.str();
        void* _dummy = NULL;
        HANDLE hndl = RtOpenSharedMemory(SHM_MAP_WRITE, 0, LPCSTR(mapped_name.c_str()), &_dummy);
        if (hndl != NULL)
            RtCloseHandle(hndl);
    }

public:
    HANDLE _shm_handle;
};

#endif

#elif defined(FRAMEWORK_PLATFORM_LINUX)

#include <sys/shm.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

class __SharedMemoryPrivateLinux:
        public __SharedMemoryPrivate
{
public:
    __SharedMemoryPrivateLinux(const std::string &name,
                               SharedMemoryKey key,
                               uint32_t size,
                               SharedMemory::ShmMode mode ):
        __SharedMemoryPrivate( name, size, key ),
        _area_shm_id(-1)
    {
        int open_mode = 0666;
        if ( mode != SharedMemory::read_only_shm )
        {
            _canWrite = 1;
//            open_mode |= S_IWRITE;
            if ( mode == SharedMemory::read_write_create_shm )
                open_mode |= IPC_CREAT;
        }

        _area_shm_id = shmget( static_cast<key_t>(key), _size, open_mode );
        if (_area_shm_id != -1)
        {
            uint32_t existing_size = 0;
            shmid_ds shm_info;
            if ( shmctl( _area_shm_id, IPC_STAT, &shm_info ) >= 0 )
                existing_size = static_cast<uint32_t>(shm_info.shm_segsz);

            if ( (existing_size > 0) && (existing_size == _size ) )
            {
                if ((_ptr = static_cast<uint32_t*>(shmat( _area_shm_id, nullptr, 0 ))) == nullptr )
                    debugPrintError( _name ) << "Error attaching to shared memory: " << FrameworkUtils::get_errno_string("") << "\n";
            }
            else
                debugPrintError( _name ) << "The area already exist but has different size! (" << existing_size << " and not " << _size << ")!\n";
        }
        else
            debugPrintError( _name ) << "Error opening shared memory: " << FrameworkUtils::get_errno_string("") << "\n";
    }

    ~__SharedMemoryPrivateLinux()
    {
        if ( _ptr != nullptr )
            shmdt( _ptr );
    }

    void deleteShm()
    {
        if ( _ptr != nullptr )
        {
            shmctl( _area_shm_id, IPC_RMID, nullptr );
            _ptr = nullptr;
        }
    }

    // This will delete a shared even if it is not open. Use this to make sure a shared has been removed.
    static void purgeShm(SharedMemoryKey key)
    {
        int shm_id = shmget( static_cast<key_t>(key), 0, IPC_CREAT | 0 );
        shmctl( shm_id, IPC_RMID, nullptr );
    }

private:
    int _area_shm_id;
};

#endif

}
//******* END SPECIFIC CODE ***********


SharedMemory::SharedMemory(const std::string &name,
                           SharedMemoryKey key,
                           uint32_t size,
                           ShmMode open_mode)
{
#if defined(FRAMEWORK_PLATFORM_LINUX)

    pdata = new __SharedMemoryPrivateLinux( name, key, size, open_mode );
#elif defined(FRAMEWORK_PLATFORM_WINDOWS)

#if defined(FRAMEWORK_PLATFORM_WINDOWS_RTX64)

    pdata = new __SharedMemoryPrivateRTX( name, key, size, open_mode );
#else

    pdata = new __SharedMemoryPrivateWindows( name, key, size, open_mode );
#endif

#endif
    if ( !isReady() )
        debugPrintError( name ) << "Error: unable to create SharedMemory with size " << size << " and key " << key << " (mode: " << open_mode << ")\n";
}

SharedMemory::~SharedMemory()
{
    delete pdata;
    pdata = nullptr;
}

const void *SharedMemory::getReadPtr()
{
    if ( pdata->isReady() )
        return pdata->getPtr();
    else
        return nullptr;
}

void *SharedMemory::getWritePtr()
{
    if ( pdata->isReady() && pdata->canWrite() )
        return pdata->getPtr();
    else
        return nullptr;
}

bool SharedMemory::isReady() const
{
    return pdata->isReady();
}

std::string SharedMemory::getSharedName() const
{
    return pdata->getName();
}

SharedMemoryKey SharedMemory::getSharedKey() const
{
    return pdata->getKey();
}

uint32_t SharedMemory::getSharedSize() const
{
    if (pdata->isReady())
        return pdata->getSize();
    else
        return 0;
}

void SharedMemory::deleteShared()
{
    pdata->deleteShm();
}

void SharedMemory::purgeShared(SharedMemoryKey key)
{
#if defined(FRAMEWORK_PLATFORM_LINUX)
    __SharedMemoryPrivateLinux::purgeShm( key );
#elif defined(FRAMEWORK_PLATFORM_WINDOWS)

#if defined(  RTX  )
    __SharedMemoryPrivateRTX::purgeShm( key );
#else
    __SharedMemoryPrivateWindows::purgeShm( key );
#endif

#endif
}
