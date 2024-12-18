#include "pithermosighandler.h"
#include "pithermomutex.h"

#include <sys/signal.h>
#include <sys/types.h>
#include <unistd.h>

#include <set>
#include <map>

#include <stdio.h>

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
        for ( std::map<int32_t, std::set<PithermoSignal*> >::iterator s = _handlers_map.begin(); s != _handlers_map.end(); ++s )
        {
            int32_t sig_no = s->first;
            signal(sig_no, SIG_DFL);
        }
        _handlers_map.clear();
        _mutex.unlock();
        _handler = NULL;
    }

    bool setHandler(PithermoSignal *handler,
                     int32_t signal_no)
    {
        bool error = true;
        _mutex.lock();
        std::map<int32_t, std::set<PithermoSignal*> >::iterator s = _handlers_map.find( signal_no );
        if ( s == _handlers_map.end() )
        {   // If no handlers yet, we also need to actually register the signal to us:
            if ( signal(signal_no, __privateSighandler::_signalHandler) != SIG_ERR )
            {
                std::set<PithermoSignal*> sl;
                _handlers_map[ signal_no ] = sl;
                s = _handlers_map.find( signal_no );
                error = false;
            }
            else
                printf("Unable to register signal n.%d\n", signal_no );
        }
        else
            error = false;
        if ( !error )
        {
            s->second.insert( handler );
        }
        _mutex.unlock();
        return !error;
    }

    bool unsetHandler( PithermoSignal* handler,
                       int32_t signal_no )
    {
        bool error = true;
        _mutex.lock();
        std::map<int32_t, std::set<PithermoSignal*> >::iterator s = _handlers_map.find( signal_no );
        if (  s != _handlers_map.end() )
        {
            std::set<PithermoSignal*>::iterator h = s->second.find( handler );
            if ( h != s->second.end() )
            {
                s->second.erase( h );
                if ( s->second.size() == 0 )
                {
                    signal(signal_no, SIG_DFL);
                    _handlers_map.erase( s );
                }
                error = false;
            }
            else
                printf("Unable to remove signal n.%d\n", signal_no );
        }
        else
            printf("Unable to deregister signal n.%d\n", signal_no);
        _mutex.unlock();
        return error;
    }

    static __privateSighandler* _handler;

private:

    PithermoMutex _mutex;
    std::map<int32_t, std::set<PithermoSignal*> > _handlers_map;

    static void _signalHandler( int s )
    {
        // Do not lock the mutex in here. Since this will pre-empt your threads, it might cause a deadlock!
        if ( _handler != NULL )
        {
            printf("Signal 'SIGINT' received!\n");

            // Call all handlers:
            uint32_t called_handlers = 0;
            std::map<int32_t, std::set<PithermoSignal*> >::iterator l = _handler->_handlers_map.find( s );
            if ( l != _handler->_handlers_map.end() )
            {
                for ( std::set<PithermoSignal*>::iterator h = l->second.begin(); h != l->second.end(); ++h )
                {
                    PithermoSignal* handler = *h;
                    if ( handler != NULL )
                    {
                        handler->customHandler( s );
                        called_handlers++;
                    }
                }
            }
        }
        else
            printf( "ERROR: sighandler initialization error?\n");
    }
};

__privateSighandler* __privateSighandler::_handler = NULL;

PithermoSigHandler::PithermoSigHandler()
{
}

bool PithermoSigHandler::setHandler(PithermoSignal *handler,
                                     int32_t user_defined_signal)
{
    __privateSighandler::initialize();
    int32_t signal_no = SIGINT;
    return __privateSighandler::_handler->setHandler( handler, signal_no );
}

bool PithermoSigHandler::unsetHandler(PithermoSignal *handler,
                                       int32_t user_defined_signal)
{
    __privateSighandler::initialize();
    int32_t signal_no = SIGINT;
    return __privateSighandler::_handler->unsetHandler( handler, signal_no );
}
