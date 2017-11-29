#include "frameworksighandler.h"
#include "frameworkutils.h"
#include "debugprint.h"
#include "basemutex.h"

#if defined(FRAMEWORK_PLATFORM_WINDOWS)
#include <signal.h>
#elif defined(FRAMEWORK_PLATFORM_LINUX)
#include <sys/signal.h>
#include <sys/types.h>
#include <unistd.h>
#endif

using namespace FrameworkLibrary;

namespace FrameworkLibrary {

class __privateSighandler
{
public:
    static void initialize()
    {
         static __privateSighandler sig_handler;
    }

    __privateSighandler():
        _mutex("SigHandler_mutex")
    {
        _handler = this;
    }

    ~__privateSighandler()
    {
        _mutex.lock();
        for ( std::map<int32_t, std::set<FrameworkSignal*> >::iterator s = _handlers_map.begin(); s != _handlers_map.end(); ++s )
        {
            int32_t sig_no = s->first;
            signal(sig_no, SIG_DFL);
        }
        _handlers_map.clear();
        _mutex.unlock();
        _handler = NULL;
    }

    bool setHandler(FrameworkSignal *handler,
                     int32_t signal_no)
    {
        bool error = true;
        _mutex.lock();
        std::map<int32_t, std::set<FrameworkSignal*> >::iterator s = _handlers_map.find( signal_no );
        if ( s == _handlers_map.end() )
        {   // If no handlers yet, we also need to actually register the signal to us:
            if ( signal(signal_no, __privateSighandler::_signalHandler) != SIG_ERR )
            {
                debugPrint("FrameworkSigHandler", DebugPrint::SIGNAL_CLASS ) << "Registered signal n. " << signal_no << "\n";
                std::set<FrameworkSignal*> sl;
                _handlers_map[ signal_no ] = sl;
                s = _handlers_map.find( signal_no );
                error = false;
            }
            else
                debugPrintError("FrameworkSigHandler" ) << "Unable to register signal n. " << signal_no << ": '" << FrameworkUtils::get_errno_string("") << "'\n";
        }
        else
            error = false;
        if ( !error )
        {
            s->second.insert( handler );
            debugPrint("FrameworkSigHandler", DebugPrint::SIGNAL_CLASS ) << "Handler added to signal n. " << signal_no << "\n";
        }
        _mutex.unlock();
        return !error;
    }

    bool unsetHandler( FrameworkSignal* handler,
                       int32_t signal_no )
    {
        bool error = true;
        _mutex.lock();
        std::map<int32_t, std::set<FrameworkSignal*> >::iterator s = _handlers_map.find( signal_no );
        if (  s != _handlers_map.end() )
        {
            std::set<FrameworkSignal*>::iterator h = s->second.find( handler );
            if ( h != s->second.end() )
            {
                s->second.erase( h );
                debugPrint("FrameworkSigHandler", DebugPrint::SIGNAL_CLASS ) << "Handler removed from signal n. " << signal_no << "\n";
                if ( s->second.size() == 0 )
                {
                    signal(signal_no, SIG_DFL);
                    debugPrint("FrameworkSigHandler", DebugPrint::SIGNAL_CLASS ) << "Unregistered signal n. " << signal_no << "\n";
                    _handlers_map.erase( s );
                }
                error = false;
            }
            else
                debugPrintError("FrameworkSigHandler" ) << "Unable to remove signal n. " << signal_no << ": handler not registered.\n";
        }
        else
            debugPrintError("FrameworkSigHandler" ) << "Unable to deregister signal n. " << signal_no << ": signal not registered.\n";
        _mutex.unlock();
        return error;
    }

    static __privateSighandler* _handler;

private:

    BaseMutex _mutex;
    std::map<int32_t, std::set<FrameworkSignal*> > _handlers_map;

    static void _signalHandler( int s )
    {
        // Do not lock the mutex in here. Since this will pre-empt your threads, it might cause a deadlock!
        if ( _handler != NULL )
        {
            if ( s == SIGINT )
                debugPrintNotice("FrameworkSigHandler" ) << "Signal 'SIGINT' received!\n";
            else
                debugPrintNotice("FrameworkSigHandler" ) << "Other signal '" << s << "' received!\n";

            // Call all handlers:
            uint32_t called_handlers = 0;
            std::map<int32_t, std::set<FrameworkSignal*> >::iterator l = _handler->_handlers_map.find( s );
            if ( l != _handler->_handlers_map.end() )
            {
                for ( std::set<FrameworkSignal*>::iterator h = l->second.begin(); h != l->second.end(); ++h )
                {
                    FrameworkSignal* handler = *h;
                    if ( handler != NULL )
                    {
                        debugPrint("FrameworkSigHandler", DebugPrint::SIGNAL_CLASS ) << "Calling handler...\n";
                        handler->customHandler(
                                    s == FrameworkSigHandler::SIGINT_SIGNAL ? FrameworkSigHandler::SIGINT_SIGNAL : FrameworkSigHandler::USER_DEFINED,
                                    s );
                        called_handlers++;
                    }
                }
            }
            debugPrint("FrameworkSigHandler", DebugPrint::SIGNAL_CLASS ) << "Signal '" << s << "' processed by " << called_handlers << " handlers.\n";
        }
        else
            debugPrintError( "FrameworkSigHandler" ) << "ERROR: sighandler initialization error?\n";
    }
};

__privateSighandler* __privateSighandler::_handler = NULL;

}

FrameworkSigHandler::FrameworkSigHandler()
{
}

bool FrameworkSigHandler::setHandler(FrameworkSignal *handler,
                                     FrameworkSigHandler::signal_type type,
                                     int32_t user_defined_signal)
{
    __privateSighandler::initialize();
    int32_t signal_no = (type == FrameworkSigHandler::SIGINT_SIGNAL) ?
                SIGINT :
                user_defined_signal;
    return __privateSighandler::_handler->setHandler( handler, signal_no );
}

bool FrameworkSigHandler::unsetHandler(FrameworkSignal *handler,
                                       FrameworkSigHandler::signal_type type,
                                       int32_t user_defined_signal)
{
    __privateSighandler::initialize();
    int32_t signal_no = (type == FrameworkSigHandler::SIGINT_SIGNAL) ?
                SIGINT :
                user_defined_signal;
    return __privateSighandler::_handler->unsetHandler( handler, signal_no );
}

void FrameworkSigHandler::injectSignal(FrameworkSigHandler::signal_type type, int32_t user_defined_signal)
{
    __privateSighandler::initialize();
    int32_t signal_no = (type == FrameworkSigHandler::SIGINT_SIGNAL) ?
                SIGINT :
                user_defined_signal;
#if defined(FRAMEWORK_PLATFORM_WINDOWS)
    raise( signal_no );
#elif defined(FRAMEWORK_PLATFORM_LINUX)
    kill( getpid(), signal_no );
#endif
}
