#include "memorychecker.h"

#include "debugprint.h"
#include "frameworkutils.h"

#include <string>
#include <map>
#include <stdlib.h>
#include <stdio.h>

#include <assert.h>

using namespace FrameworkLibrary;

namespace FrameworkLibrary{

class __MemoryCheckerItem
{
public:
    __MemoryCheckerItem( const char* file, const char* func, int line, const std::string& msg ):
        _file(file),
        _func(func),
        _line(line),
        _msg(msg)
    {
    }

    __MemoryCheckerItem( const __MemoryCheckerItem& other ):
        _file(other._file),
        _func(other._func),
        _line(other._line),
        _msg(other._msg)
    {
    }

    __MemoryCheckerItem():
        _file(""),
        _func(""),
        _line(0),
        _msg("")
    {
    }

    std::string getFile() const
    {
        return _file;
    }

    std::string getFunc() const
    {
        return _func;
    }

    uint32_t getLine() const
    {
        return _line;
    }

    std::string getMsg() const
    {
        return _msg;
    }

private:
    std::string _file;
    std::string _func;
    uint32_t _line;
    std::string _msg;
};

class __MemoryCheckerPrivate
{
public:
    inline bool isAllocated( void* ptr ) const
    {
        return _allocated_pointers.find( ptr ) != _allocated_pointers.end();
    }

    inline bool isDeallocated( void* ptr ) const
    {
        return _deallocated_pointers.find( ptr ) != _deallocated_pointers.end();
    }

    inline void allocate( void* ptr, const __MemoryCheckerItem& item )
    {
        if ( isDeallocated( ptr ) )
            _deallocated_pointers.erase( ptr );
        _allocated_pointers[ ptr ] = item;
    }

    inline void deallocate( void* ptr, const __MemoryCheckerItem& item )
    {
        _allocated_pointers.erase( ptr );
        _deallocated_pointers[ ptr ] = item;
    }

    std::map<void*, __MemoryCheckerItem> listAllocatedItems() const
    {
        return _allocated_pointers;
    }

    std::map<void*, __MemoryCheckerItem> listDeallocatedItems() const
    {
        return _deallocated_pointers;
    }

    __MemoryCheckerItem getDeallocated( void* ptr )
    {
        return _deallocated_pointers[ ptr ];
    }

    bool _assert_on_error;

private:
    std::map<void*, __MemoryCheckerItem> _allocated_pointers;
    std::map<void*, __MemoryCheckerItem> _deallocated_pointers;
};

BaseMutex MemoryChecker::_private_mutex( "MemoryChecker" );
__MemoryCheckerPrivate* MemoryChecker::_private = NULL;

}

MemoryChecker::MemoryChecker()
{
}

MemoryChecker::~MemoryChecker()
{
}

bool MemoryChecker::initialize()
{
    if ( _private == NULL )
    {
        _private = new __MemoryCheckerPrivate();
        atexit( MemoryChecker::printOnExit );
        if ( FrameworkUtils::read_env( "FRAMEWORK_MEMORY_CHECK_ASSERT" ) != "" )
        {
            _private->_assert_on_error = true;
            debugPrint( "MemoryChecker", DebugPrint::MEMORYCHECKER_CLASS ) << "Will assert(0) on errors [FRAMEWORK_MEMORY_CHECK_ASSERT]\n";
        }
        else
            _private->_assert_on_error = false;
    }
    return true;
}

void MemoryChecker::printOnExit()
{
    _private_mutex.lock();
    std::map<void*, __MemoryCheckerItem> items = _private->listAllocatedItems();
    if ( items.size() > 0 )
    {
        debugPrintError( "MemoryChecker" ) << "***** The following pointers have NOT BEEN DEALLOCATED properly:\n";
        for ( std::map<void*, __MemoryCheckerItem>::iterator i = items.begin(); i != items.end(); ++i )
        {
            void* ptr = i->first;
            __MemoryCheckerItem item = i->second;
            debugPrintError( "MemoryChecker" ) << "       [" << (void*)ptr << "] allocated in file " << item.getFile() << "(" << item.getLine() << ") in function " << item.getFunc() << "." << item.getMsg() << "\n";
        }
        debugPrintError( "MemoryChecker" ) << "***** This is potentially an ERROR and should be fixed!\n";
    }
    _private_mutex.unlock();
    delete _private;
    _private = NULL;
}

void MemoryChecker::traceNew(void *ptr, const char *file, const char *func, int line, const std::string &msg)
{
    if ( ptr != NULL )
    {
        _private_mutex.lock();
        if ( initialize() )
        {
            if ( !_private->isAllocated( ptr ) )
            {
                _private->allocate( ptr, __MemoryCheckerItem( file, func, line, msg ) );
                debugPrint( "MemoryChecker", DebugPrint::MEMORYCHECKER_CLASS )
                        << "Allocated pointer " << (void*)ptr << " in file " << file << "(" << line << ") at function " << func << "\n";
            }
        }
        _private_mutex.unlock();
    }
}

void MemoryChecker::traceDelete(void *ptr, const char *file, const char *func, int line)
{
    if ( ptr != NULL )
    {
        _private_mutex.lock();
        if ( initialize() )
        {
            if ( _private->isAllocated( ptr ) )
            {
                _private->deallocate( ptr, __MemoryCheckerItem( file, func, line, "" ) );
                debugPrint( "MemoryChecker", DebugPrint::MEMORYCHECKER_CLASS )
                        << "Deallocated pointer " << (void*)ptr << " in file " << file << "(" << line << ") at function " << func << "\n";

            }
            else if ( _private->isDeallocated( ptr ) )
            {
                __MemoryCheckerItem item = _private->getDeallocated( ptr );
                debugPrintError( "MemoryChecker" ) << "ERROR: in file " << file << "(" << line << ") at function " << func << ", you are trying to DEALLOCATE an AREADY DEALLOCATED pointer! (deallocated at: " << item.getFile() << ":" << item.getLine() << ", function " << item.getFunc() << ")\n";
                if ( _private->_assert_on_error )
                    assert(0);
            }
            else
            {
                debugPrintError( "MemoryChecker" ) << "ERROR: in file " << file << "(" << line << ") at function " << func << ", you are trying to DEALLOCATE a NON ALLOCATED pointer!\n";
                if ( _private->_assert_on_error )
                    assert(0);
            }
        }
        _private_mutex.unlock();
    }
}
