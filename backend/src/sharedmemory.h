#ifndef SHAREDMEMORY_H
#define SHAREDMEMORY_H

#include <stdint.h>
#include <string>

typedef uint32_t SharedMemoryKey;

class SharedMemory
{
public:
    enum ShmMode { read_only_shm = 0, /**< Open a shared memory for read-only. If does not exist, it will not be created. */
                   read_write_create_shm = 1, /**< Open a shared memory for write, and create it if does not exist */
                   read_write_nocreate_shm = 2 /**< Open a shared memory for write but do not create it if does not exist */
                 };

    SharedMemory( const std::string& name,
                  SharedMemoryKey key,
                  uint32_t size,
                  ShmMode open_mode );

    virtual ~SharedMemory();
    const void* getReadPtr();
    void* getWritePtr();
    bool isReady() const;
    std::string getSharedName() const;
    SharedMemoryKey getSharedKey() const;
    uint32_t getSharedSize() const;
    void deleteShared();
    static void purgeShared( SharedMemoryKey key );

private:
    class __SharedMemoryPrivate;
    __SharedMemoryPrivate* pdata;
};


#endif // SHAREDMEMORY_H
