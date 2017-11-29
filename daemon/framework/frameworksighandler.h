#ifndef FRAMEWORKSIGHANDLER_H
#define FRAMEWORKSIGHANDLER_H

#include "common_defs.h"
#include <string.h>

#include <map>
#include <set>

namespace FrameworkLibrary {

class FrameworkSignal;
/** @brief This class implements a platform-agnostic signal handler class
 *
 * Due to the nature of signals, this class is a bit unusual. It is implemented as a "singleton" with static only access.
 *
 * By default it will handle SIGINT (it's the only signal supported across all the platforms).
 * You can also handle any other signal you want (remember some signals cannot be catched, check your OS) by registering them
 * individually.
 *
 * If you need to register your signal handler, then you need to create your instance of the FrameworkSignal class and use the method:
 * - setHandler()
 *
 * The default implementation of the FrameworkSignal class will just count the number of signals received, but you can derive it and
 * reimplement the FrameworkSignal::customHandler() method.
 *
 * For advance use only, you can "inject" any signal by calling the method:
 * - injectSignal()
 *
 * @note This does NOT work on Windows at the moment.
 *
 * Please note this will work by actually sending a signal to your own process (using the system call "kill" on Linux, for example) so
 * it could mess with your debugger like a normal signal would.
 *
 * Signals are determined by the signal_type enumerative:
 * - signal_type::ANY: use this when calling signalReceivedCount() to check for any signal
 * - signal_type::SIGINT_SIGNAL: the INT signal
 * - signal_type::USER_DEFINED: custom signal, usually then you must specify the signal number separately.
 *
 * Signal handlers are managed as a list of handlers and, when the signal is received, will be called in the same order they have been
 * registered. If you need to "remove" a handler, you must call the unsetHandler() method passing the same handler class.
 *
 * @include example_cpp_sighandler.cpp
 *
 * 
 *
 */
class DLLEXPORT FrameworkSigHandler
{
    friend class __privateSighandler;
public:
    /** @brief type of signal */
    enum signal_type { SIGINT_SIGNAL, /**< SIGINT  */
                       USER_DEFINED /**< Specify a custom signal number (OS dependent)  */
                     };

    /** @brief Add a signal handler
     * @param handler the signal handler class
     * @param type which type of signal.
     * @param user_defined_signal if type is USER_DEFINED, specify here the signal number
     * @return true if registered properly, false if type or user_defined_signal is not valid.
     *
     * 
     *
     */
    static bool setHandler( FrameworkSignal* handler, signal_type type, int32_t user_defined_signal = -1 );

    /** @brief Remove a signal handler
     * @param handler the signal handler class
     * @param type which type of signal.
     * @param user_defined_signal if type is USER_DEFINED, specify here the signal number
     * @return true if the handler has been unregistered. False if the handler was not registered for the given signal.
     *
     * 
     *
     */
    static bool unsetHandler( FrameworkSignal* handler, signal_type type, int32_t user_defined_signal = -1 );

    /** @brief simulate the reception of a signal even if a signal has not been received.
     * @since 4.0.0 supported also on Windows
     *
     * You can call this to send a signal to yourself even if you do not use this class in any other way. This is used, for example,
     * by KeyboardRaw, to inject the CTRL+C combination.
     *
     * @param type which type of signal.
     * @param user_defined_signal if type is USER_DEFINED, specify here the signal number
     *
     * 
     *
     */
    static void injectSignal( signal_type type, int32_t user_defined_signal = -1 );

private:
    FrameworkSigHandler();
};

/** @brief Handle a signal
 *
 * Implement this class to add your custom signal handler.
 * To be used by FrameworkSigHandler class.
 *
 * 
 *
 */
class DLLEXPORT FrameworkSignal
{
    friend class __privateSighandler;

public:
    virtual ~FrameworkSignal()
    {
    }

protected:
    /** @brief Implement here your own signal handling
     * @param type which type of signal.
     * @param user_defined_signal this will be the signal number, most useful only if type is USER_DEFINED.
     *
     * 
     *
     */
    virtual void customHandler(FrameworkSigHandler::signal_type type, int32_t user_defined_signal ) = 0;

};

}

#endif // FRAMEWORKSIGHANDLER_H
