#ifndef BASEMUTEX_H
#define BASEMUTEX_H

#include "common_defs.h"

#include <string>

namespace FrameworkLibrary {

class __BaseMutexPrivate;
/** @brief Implements a mutex for thread synchronization
 *
 * @since 3.4 on all platforms mutexes are not non-recursive. Before 3.4, on Windows it was recursive.
 *
 * Creates a mutex, which is used for shared data access between parallel threads.
 *
 * @warning This is a NON RECURSIVE mutex, like pthread mutexes, not like broken Windows mutexes, which are
 * recursive by bad design. If you lock it twice in the same thread, the second lock will deadlock (unless you
 * unlock it from a different thread). Be aware of this.
 *
 * You can lock() it, unlock() it, check if it's locked with isLocked() and perform a non-blocking lock
 * with tryLock(), which will not block in case the lock cannot be acquired.
 *
 * 
 *
 */
class DLLEXPORT BaseMutex
{
public:
    /** @brief Create the mutex
     * 
     * @param name Name of the mutex
     */
    BaseMutex( const std::string& name );

    /** @brief delete the mutex
     */
    virtual ~BaseMutex();

    /** @brief set mutex name
     * @param name the new mutex name
     */
    void setName( const std::string& name );

    /** @brief lock the mutex. Blocks until the mutex can be locked
     * 
     */
    void lock();

    /** @brief unlock a locked mutex. Calling on non-locked mutex is undefined.
     * 
     */
    void unlock();

    /** @brief try to lock, but don't block if lock cannot be acquired
     * @return true if you got the lock, false otherwise.
     * 
     */
    bool tryLock();

private:
    __BaseMutexPrivate* _private;
};

}

#endif // BASEMUTEX_H
