#include "sharedmemory.h"

#include <sys/shm.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

class SharedMemory::__SharedMemoryPrivate
{
public:
    __SharedMemoryPrivate(const std::string &name,
                          SharedMemoryKey key,
                          uint32_t size,
                          SharedMemory::ShmMode mode ):
        _key(key),
        _size(size),
        _ptr(nullptr),
        _canWrite(0),
        _name(name),
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
                _ptr = static_cast<uint32_t*>(shmat( _area_shm_id, nullptr, 0 ));
        }
    }

    virtual ~__SharedMemoryPrivate()
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
    int _area_shm_id;
};


SharedMemory::SharedMemory(const std::string &name,
                           SharedMemoryKey key,
                           uint32_t size,
                           ShmMode open_mode)
{
    pdata = new __SharedMemoryPrivate( name, key, size, open_mode );
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
    __SharedMemoryPrivate::purgeShm( key );
}
