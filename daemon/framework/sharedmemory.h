#ifndef SHAREDMEMORY_H
#define SHAREDMEMORY_H

#include <common_defs.h>
#include <stdint.h>
#include <string>

namespace FrameworkLibrary {

/** @brief the shared key
 */
typedef uint32_t SharedMemoryKey;

class __SharedMemoryPrivate;

/** @brief this class provides multi-platform support for shared memories.
 *
 * You need to provide a KEY (on 32bit, unsigned) which will be used to identify the shared memory portion.
 * After creating the memory, check with isready() if everything has been done properly, otherwise
 * assume that the memory HAS NOT been created for some reason. Check your program output to find out
 * exactly why.
 *
 * @warning this is NOT a memory area of the Framework! This is a generic shared memory on the operating system.
 *
 *  @include example_cpp_sharedmemory.cpp
 *
 * 
 *
 */
class DLLEXPORT SharedMemory
{
public:
    /** @brief Shared memory open modes
     */
    enum ShmMode { read_only_shm = 0, /**< Open a shared memory for read-only. If does not exist, it will not be created. */
                   read_write_create_shm = 1, /**< Open a shared memory for write, and create it if does not exist */
                   read_write_nocreate_shm = 2 /**< Open a shared memory for write but do not create it if does not exist */
                 };

    /** @brief Create a shared memory space on all the supported platforms.
     *
     * A shared memory can be opened as read-only or as read-write. If you request it in read-write, you can specify if you want
     * it to be created if it does not already exist, or not. Actual implementation of this modes can be platform-dependent.
     * @param key Key to uniquely identify the shared memory
     * @param size size in bytes of the shared memory
     * @param name user-defined name of the area
     * @param open_mode select opening mode (read only, read write, create...)
     *
     * @since 4.0.0 permission modes have been simplified.
     *
     * 
     *
     */
    SharedMemory( const std::string& name,
                  SharedMemoryKey key,
                  uint32_t size,
                  ShmMode open_mode );

    virtual ~SharedMemory();

    /** @brief return a read-only pointer for the shared memory
     * @return the pointer. NULL if the shared memory has not been created
     *
     * 
     *
     */
    const void* getReadPtr();

    /** @brief return a read-write pointer for the shared memory
     *
     * @warning you must make sure you have write access to use this pointer, otherwise the operating system will
     *       very likely kill your application depends on the underlying operating system)
     * @return the pointer. NULL if the shared memory has not been created
     *
     * 
     *
     */
    void* getWritePtr();

    /** @brief the shared memory has been created?
     * @return true if the shared exists, false otherwise.
     *
     * 
     *
     */
    bool isReady() const;

    /** @brief Get the shared memory name
     * @return the name
     * @since 4.0.0
     */
    std::string getSharedName() const;

    /** @brief Get the shared memory key
     * @return the key
     * @since 4.0.0
     */
    SharedMemoryKey getSharedKey() const;

    /** @brief the size of the shared memory
     *
     * The returned size always match the size requested in the constructor. Otherwise, the are is not created.
     * @return size in bytes
     * @since 3.3, renamed from getSize() which would collide with child classes.
     *
     * 
     *
     */
    uint32_t getSharedSize() const;

    /** @brief Delete the shared memory, unix style.
     *
     * The shared will be marked as "deleted" so no other process will be able to attach to it.
     * You CANNOT use this shared memory after this call.
     * @since 4.0 renames older closeShared()
     *
     * 
     *
     */
    void deleteShared();

    /** @brief Ensure a shared is deleted
     * @note the shared will NOT be opened, just removed if it exist
     * @note This method is static and shall be used to ensure a shared does not exist given it's key.
     * @since 3.4
     * @param key the key
     *
     * 
     *
     */
    static void purgeShared( SharedMemoryKey key );

private:
    FrameworkLibrary::__SharedMemoryPrivate* pdata;
};

}

#endif // SHAREDMEMORY_H
