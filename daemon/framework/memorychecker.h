#ifndef MEMORYCHECKER_H
#define MEMORYCHECKER_H

/**
 * @file
 */

#include <common_defs.h>
#include <basemutex.h>

#include <string>

namespace FrameworkLibrary {

class __MemoryCheckerPrivate;

/**
  @defgroup DEBUG_HELPERS Debug Helpers tools
  These tools are provided to help debugging your application.

  @{
  */

/** @brief Verify heap memory usage
 * @since 3.4
 *
 * This multiplatform class provides interfaces to protect and check your pointers for common memory allocation/deallocation
 * issues.
 *
 * At this time, the following checks are performed:
 * - (1) double-free, deallocate a pointer twice
 * - (2) unallocated-free, deallocate a non-allocated pointer
 * - (3) missed-free, an allocated pointer is not deallocated (lost memory)
 *
 * In order to protect your pointers and enable the checks on them, you need to modify your
 * code and add the two calls MEMORY_CHECKER_ADD and MEMORY_CHECKER_DEL. These are macros which
 * will perform the appropriate call to the MemoryChecker class.
 *
 * Checks (1) and (2) are performed every time you call the MEMORY_CHECKER_DEL macro (usually, when calling delete on the pointer)
 * while check (3) is performed at program shutdown, after the main returns.
 *
 * How to use
 * ==========
 *
 * Anytime you want to add checks to a pointes, you call the MEMORY_CHECKER_ADD macro. You can call this macro multiple times on the
 * same pointer safely, so it's safe to call it on pointers returned from other code or from the Simulation Framework itself.
 *
 * When you are done with the pointed, you call MEMORY_CHECKER_DEL to notify the Memory Checker that you plan to immediately dispose
 * of the pointer. You must call this macro exactly ONCE per pointer.
 *
 * The usual scenario looks like this:
 * @code
 * MyClass* ptr = MEMORY_CHECKER_ADD( new MyClass(), MyClass );
 * ... your code...
 * delete MEMORY_CHECKER_DEL( class, MyClass );
 * @endcode
 *
 * Also, you usually want to call MEMORY_CHECKER_ADD everytime a new pointer is owned by your code and call MEMORY_CHECKER_DEL every time
 * a pointer you own shall be deleted by you.
 *
 * Do not call MEMORY_CHECKER_DEL on pointers owned by the Simulation Framework (it's usually documented).
 *
 * There is also another macro: MEMORY_CHECKER_ADD_MSG which lets you specify a custom message to print when an error is detected.
 *
 *
 * Debugging errors
 * ================
 *
 * The MemoryChecker will obey the FRAMEWORK_DEBUG_MEMOCYCHECKER class. If you set it to 1 it will print on screen each time
 * a pointer is added or deleted.
 *
 * Sometimes it's difficult to debug where an error is occurred. It's much easier if you can get a proper stack trace.
 * If you need to, you can specify the environment variable:
 * @code
 * export FRAMEWORK_MEMORY_CHECK_ASSERT=1
 * @endcode
 *
 * and the MemoryChecker will generate an invalid assert when an error is detected. You can then catch this with your debugger
 * and proceed from there.
 *
 *
 * False positives
 * ===============
 *
 * Unfortunately, sometimes the MemoryChecker will generate false-positives.
 *
 * Most common false-positives are when for some reason
 * you call MEMORY_CHECKER_ADD on a pointer but forget to call MEMORY_CHECKER_DEL on the same pointer even if you properly delete it.
 *
 * Another common false-positive is related to static globals. Since the MemoryChecker uses atExit() to register it's shutdown handler
 * and verify pointer there, if you have other global static objects they might get destroyed AFTER the MemoryChecker has completed
 * it's checks. This will cause two false positives on all pointers owned by the static global objects:
 * - pointers will result non-deallocated first,
 * - then will result as deallocated while not-allocated
 *
 * The best solution in this case is to delete your pointers before the destructor of the static global classes is called.
 *
 * 
 *
 */
class DLLEXPORT MemoryChecker
{
public:
    /** @brief Add a new pointer to the checker class
     * @param ptr Pointer to check
     * @param file name of file where the pointer is created
     * @param func name of function where the pointer is created
     * @param msg string to print when a missing-delete is detected
     * @param line line number where the pointer is created
     *
     * 
     *
     */
    static void traceNew(void* ptr, const char* file, const char* func, int line , const std::string& msg);

    /** @brief Remove a pointer from the checker class
     * @param ptr Pointer not to check anymore
     * @param file name of file where the pointer is created
     * @param func name of function where the pointer is created
     * @param line line number where the pointer is created
     *
     * 
     *
     */
    static void traceDelete( void* ptr, const char* file, const char* func, int line );


private:
    /** Constructor is private, this class cannot be allocated.
     */
    MemoryChecker();
    /** Destructor is private, this class cannot be allocated.
     */
    ~MemoryChecker();

    /** @brief itnernally called to initialize the checker
     */
    static bool initialize();
    /** @brief Called at program exit for cleanup and printouts
     */
    static void printOnExit();

    static __MemoryCheckerPrivate* _private;
    static BaseMutex _private_mutex;
};

/** @brief add a pointer to the check system
 * @param ptr the pointer to protect
 * @param type the return-type
 *
 * 
 *
 */
#define MEMORY_CHECKER_ADD( ptr, type ) __memoryCheckerNew<type>( ptr, __FILE__, ____func_def__, __LINE__, "" )

/** @brief add a pointer to the check system, with custom message
 * @param ptr the pointer to protect
 * @param msg the custom message to print in case of error
 * @param type the return-type
 *
 * 
 *
 */
#define MEMORY_CHECKER_ADD_MSG( ptr, type, msg ) __memoryCheckerNew<type>( ptr, __FILE__, ____func_def__, __LINE__, msg )

/** @brief remove a pointer from the check system
 * @param ptr the protected pointer
 * @param type the return-type
 *
 * 
 *
 */
#define MEMORY_CHECKER_DEL( ptr, type ) __memoryCheckerDelete<type>( ptr, __FILE__, ____func_def__, __LINE__ )

/**
  @}
  */

/** @brief Helper method for the memory checker macros (add)
 * @warning Do not call this yourself, use the MEMORY_CHECKER_ADD() and MEMORY_CHECKER_ADD_MSG() macros!
 * @param ptr The pointer to add to the checker
 * @param file current file
 * @param func current function
 * @param line current line
 * @param msg additional message to print in case of errors on this pointer
 * @return the same ptr passed ad input
 *
 * 
 *
 */
template<typename T> DLLEXPORT
T *__memoryCheckerNew(T *ptr, const char *file, const char *func, int line, const std::string& msg)
{
    MemoryChecker::traceNew( ptr, file, func, line, msg );
    return ptr;
}

/** @brief Helper method for the memory checker macros (del)
 * @warning Do not call this yourself, use the MEMORY_CHECKER_DEL() macro!
 * @param ptr The pointer to add to the checker
 * @param file current file
 * @param func current function
 * @param line current line
 * @return the same ptr passed ad input
 *
 * 
 *
 */
template<typename T> DLLEXPORT
T *__memoryCheckerDelete(T *ptr, const char *file, const char *func, int line)
{
    MemoryChecker::traceDelete( ptr, file, func, line );
    return ptr;
}

}

#endif // MEMORYCHECKER_H
