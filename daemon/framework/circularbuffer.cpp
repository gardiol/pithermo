#include "circularbuffer.h"
#include <string.h>

#include "debugprint.h"
#include "memorychecker.h"

using namespace FrameworkLibrary;

namespace FrameworkLibrary {

class __private_CircularBuffer
{
    friend class CircularBuffer;

    __private_CircularBuffer(uint32_t initial_size, uint32_t max_size):
        mutex( "CircularBuffer_mutex" )
    {
        _max_size = initial_size > max_size ? initial_size : max_size;
        start_index = 0;
        end_index = 0;
        buffer_size = 0;
        buffer = NULL;
        if ( initial_size > 0 )
        {
            buffer_size = _sizeStepper( initial_size );
            buffer = MEMORY_CHECKER_ADD( new char[ buffer_size ], char );
        }

    }

    const static uint32_t buffer_step = 4 * 1024;

    uint32_t buffer_size;
    uint32_t _max_size;
    char* buffer;

    uint32_t start_index;
    uint32_t end_index;

    BaseMutex mutex;

    uint32_t _sizeStepper( uint32_t size ) const
    {
        uint32_t stepped_size = ((size/__private_CircularBuffer::buffer_step)+1) * __private_CircularBuffer::buffer_step;
        if ( stepped_size > _max_size )
            return _max_size;
        else
            return stepped_size;
    }

    uint32_t _getSizeNoLock()
    {
        return end_index - start_index;
    }
};

}

CircularBuffer::CircularBuffer(uint32_t initial_size, uint32_t max_size)
{
    _private = new __private_CircularBuffer( initial_size, max_size );
}

CircularBuffer::~CircularBuffer()
{
    if ( _private->buffer != NULL )
    {
        delete [] MEMORY_CHECKER_DEL( _private->buffer, char );
    }
    _private->buffer = NULL;
    _private->buffer_size = 0;
    _private->start_index = 0;
    _private->end_index = 0;
    delete _private;
    _private = NULL;
}

void CircularBuffer::setMaxSize(uint32_t max_size)
{
    _private->mutex.lock();
    _private->_max_size = max_size;
    // If the new max is less than the current buffer, we need to shrink it now:
    if ( (_private->buffer != NULL) && (_private->buffer_size > _private->_max_size) ) // if buffer_size > 0 means buffer != NULL
    {
        char * new_buffer = MEMORY_CHECKER_ADD( new char[ _private->_max_size ], char );
        // First let's remove all consumed data and see if it's enough:
        uint32_t unconsumed_data_size = _private->end_index - _private->start_index;
        // We check also the end gap, if there is one, using end_index instead of buffer_size:
        if ( unconsumed_data_size <= _private->_max_size )
        {
            // Ok, we just remove the consumed data and it will fit:
            memcpy( new_buffer, &_private->buffer[ _private->start_index ], unconsumed_data_size );
            _private->end_index = unconsumed_data_size;
        }
        else // No, we need more space... we will have to drop some consumed data
        {
            memcpy( new_buffer, &_private->buffer[ _private->buffer_size - _private->_max_size ], _private->_max_size );
            _private->end_index = _private->_max_size;
        }
        _private->start_index = 0;
        delete [] MEMORY_CHECKER_DEL( _private->buffer, char );
        _private->buffer = new_buffer;
        _private->buffer_size = _private->_max_size;
    }
    _private->mutex.unlock();
}

uint32_t CircularBuffer::getSize()
{
    uint32_t ret = 0;
    bool unlock = false;
    if ( _private->mutex.tryLock() )
        unlock = true;
    ret = _private->_getSizeNoLock();
    if ( unlock )
        _private->mutex.unlock();
    return ret;
}

void CircularBuffer::compactMemory()
{
    _private->mutex.lock();
    // Don't do anything if there is no unconsumed data to free:
    if ( (_private->buffer != NULL) && (_private->buffer_size > _private->_getSizeNoLock()) )
    {
        // Create a new buffer of the reduced size:
        _private->_max_size = _private->buffer_size = _private->_getSizeNoLock();
        // if there is unconsumed data, keep it:
        if ( _private->buffer_size > 0 )
        {
            char* new_buffer = MEMORY_CHECKER_ADD( new char[ _private->buffer_size ], char );
            // Copy the data from the current buffer to the new smaller one:
            memcpy( new_buffer, &_private->buffer[ _private->start_index ], _private->_max_size );
            _private->end_index = _private->buffer_size;
            delete [] MEMORY_CHECKER_DEL( _private->buffer, char );
            _private->buffer = new_buffer;
        }
        // No unconsumed data, just delete the buffer:
        else
        {
            delete [] MEMORY_CHECKER_DEL( _private->buffer, char );
            _private->buffer = NULL;
            _private->end_index = 0;
        }
        _private->start_index = 0;
    }
    _private->mutex.unlock();
}

void *CircularBuffer::consumeData(uint32_t consume_data_size , uint32_t &actual_data_size)
{
    char* tmp = NULL;
    _private->mutex.lock();
    uint32_t available_size = _private->_getSizeNoLock();
    // Make sure we don't read more than what's available:
    if (  consume_data_size > available_size )
        consume_data_size = available_size;
    // Report to the called how much data ia really available to read:
    actual_data_size = consume_data_size;
    tmp = &_private->buffer[ _private->start_index ];
    _private->start_index += consume_data_size;
    return tmp;
}

void *CircularBuffer::consumeAllData(uint32_t &size )
{
    return consumeData( getSize(), size );
}

void CircularBuffer::unconsumeData(uint32_t unconsumed_data_size)
{
    if ( unconsumed_data_size < _private->start_index )
    {
        _private->start_index -= unconsumed_data_size;
    }
}

void *CircularBuffer::peekData(uint32_t &len)
{
    _private->mutex.lock();
    len = _private->_getSizeNoLock();
    if ( (_private->buffer != NULL) && (len > 0) )
        return &_private->buffer[ _private->start_index ];
    else
        return NULL;
}

void *CircularBuffer::appendData(uint32_t incoming_data_size )
{
    _private->mutex.lock();

    // Accept only appends which can fit into the maximum size (maybe by dropping existing unconsumed data):
    if ( incoming_data_size < _private->_max_size )
    {
        // Get the old and the new unconsumed data size:
        uint32_t old_size = _private->_getSizeNoLock();
        uint32_t new_size = old_size + incoming_data_size;
        // Does this size fit the existing buffer?
        if ( new_size <= _private->buffer_size )
        {
            // Yes it fits, but do we need to discard unconsumed data?
            if ( (_private->start_index + new_size) > _private->buffer_size )
            {
                // Yes, discard the unconsumed data, move back the buffer:
                memmove( _private->buffer, &_private->buffer[ _private->start_index ], old_size );
                _private->start_index = 0;
                _private->end_index = new_size;
            }
            else // No, it can fit, nothing to do here:
                _private->end_index += incoming_data_size;
        }
        else // It does not fit the buffer...
        {
            // Can we enlarge the buffer?
            if ( new_size <= _private->_max_size ) // buffer_size < new_size < max_size
            {
                // yes we can still enlarge the buffer...
                _private->buffer_size = _private->_sizeStepper( new_size );
                char* new_buffer = MEMORY_CHECKER_ADD( new char[ _private->buffer_size ], char );
                // Copy the unconsumed data to the new buffer, if any:
                if ( _private->buffer != NULL ) // buffer might be NULL here!
                {
                    memcpy( new_buffer, &_private->buffer[ _private->start_index ], old_size );
                    delete [] MEMORY_CHECKER_DEL( _private->buffer, char );
                }
                _private->start_index = 0;
                _private->end_index = new_size;
                _private->buffer = new_buffer;
            }
            else // buffer_size <= max_size < new_size
            {
                // How much shall we discard from the new buffer:
                uint32_t to_be_discarded = new_size - _private->_max_size;
                // Can we squeeze a few more bytes from the buffer size?
                if ( _private->buffer_size < _private->_max_size )
                {
                    _private->buffer_size = _private->_max_size;
                    char* new_buffer = MEMORY_CHECKER_ADD( new char[ _private->buffer_size ], char );
                    // Copy the unconsumed data to the new buffer, if any:
                    if ( _private->buffer != NULL ) // buffer might be NULL here!
                    {
                        memcpy( new_buffer, &_private->buffer[ _private->start_index + to_be_discarded ], old_size - to_be_discarded );
                        delete [] MEMORY_CHECKER_DEL( _private->buffer, char );
                    }
                    _private->start_index = 0;
                    _private->end_index = _private->_max_size;
                    _private->buffer = new_buffer;
                }
                else // Buffer is already at maximum size, we can just move the memory back
                {
                    memmove( _private->buffer, &_private->buffer[ _private->start_index + to_be_discarded ], old_size - to_be_discarded );
                    _private->start_index = 0;
                    _private->end_index = _private->_max_size;
                }
            }
        }
        return &_private->buffer[ _private->end_index - incoming_data_size ];
    }
    else
    {
        debugPrintError( "CircularBuffer" ) << "ERROR: requested " << incoming_data_size << " bytes which is LARGER than the maximum size of " << _private->_max_size << "\n";
        return NULL;
    }
}

void CircularBuffer::unlockBuffer()
{
    _private->mutex.unlock();
}

uint32_t CircularBuffer::getTotalBufferSize() const
{
    return _private->_max_size;
}

void CircularBuffer::clear()
{
    _private->mutex.lock();
    _private->start_index = 0;
    _private->end_index = 0;
    _private->mutex.unlock();
}

