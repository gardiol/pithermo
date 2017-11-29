#ifndef CIRCULARBUFFER_H
#define CIRCULARBUFFER_H

#include <common_defs.h>
#include <basethread.h>
#include <string.h>
#include <basemutex.h>

namespace FrameworkLibrary {

class __private_CircularBuffer;

/** @brief Implement a buffer which can be enlarged and shrinked as needed
  *
  * Implement a queue where you can append and retrieve (consume) data. It's a FIFO lile queue but
  * you use it like a pointer.
  *
  * To append data you call CircularBuffer::appendData(), you must pass the size of data you will be appending.
  * This will return a pointer which is big enough to hold the data: you must then copy the data to it and then
  * release it with CircularBuffer::unlockBuffer(). Never store this pointer which might be invalidated
  * immediately after the second call.
  *
  * To access (consume) the appended data you must call the CircularBuffer::consumeData() method, you must pass
  * the size of the data you will consume. The method will return a pointer wihch you must use to access the data
  * up to the required size. Do not read more than that, it's not guaranteed to be valid. As soon as you are
  * done, call CircularBuffer::unlockBuffer() to release the pointer. Never store this pointer which might be invalidated
  * immediately after the second call. This method will mark all the consumed data as "consumed", so it will be
  * purged and it's memory reused as soon as needed.
  *
  * If you need to "peek" (read and not consume) data in the buffer, use the CircularBuffer::peekAllData() or
  * CircularBuffer::peekData() methods.
  * As usual, remember to call CircularBuffer::unlockBuffer() to release the pointer and don't store it.
  *
  * This class is thread safe.
  *
  * As for memory consumption, you can specify a starting size on the constructor (useful if you know in
  * advance how much data you need, to reduce memory allocation). The CircularBuffer will allocate more memory when
  * needed. you can also specify a maximum size with CircularBuffer::setMaxSize(), in this case appending on a
  * full buffer will overwrite the older non-yet-consumed data. By default, the maximum size is 64Kbyte.
  *
  * Please note that memory, by default, will never be released unless you set a smaller minimum maxSize. If
  * you want to clear up the consumed memory you need to explicitly call the CircularBuffer::freeConsumed()
  * method.
  *
  * Appending data over the maximum size will first use already
  * consumed memory, but if not enough consumed memory is available, it will start eating your
  * unconsumed memory. So keep consuming your memory if you cannot afford to lose it.
  *
  * @include example_cpp_circbuf.cpp
  *
  * 
  *
  */
class DLLEXPORT CircularBuffer
{
public:
    const static uint32_t default_maximum_size = 64 * 1024; /**< Maximum size of the internal buffer, default value  */

    /** @brief Initialize the buffer, with an initial size.
     *
     * If the initial_size is bigger than the default maximum size, the maximum size will increased to fit.
      * @param initial_size initial buffer sizes
      * @param max_size maximum size of buffer. If omitted it's equal to CircularBuffer::default_maximum_size
      *
      * 
      *
      */
    CircularBuffer( uint32_t initial_size = 0,
                    uint32_t max_size = CircularBuffer::default_maximum_size );
    ~CircularBuffer();

    /** @brief set the maximum size of the buffer
     * @param max_size in bytes, maximum memory usage
      *
      * 
      *
     */
    void setMaxSize( uint32_t max_size );

    /** @brief queued data size: how many unconsumed bytes are available
      * @return size in bytes of the data which needs to be consumed
      *
      * 
      *
      */
    uint32_t getSize();

    /** @brief free allocated memory which has been consumed
      *
      * 
      *
     */
    void compactMemory();

    /** @brief Consume data
      *
      * Read the data from the returned pointer. DONT STORE THE POINTER!
      * Consumed data will be removed from the buffer and the memory reused.
      * Remember to ALWAYS release the pointer as soon as you are done calling unlockBuffer()!
      * And remember to check actual_data_size, in case somebody else has consumed some data and
      * consume_data_size was too big.
      *
      * @param consume_data_size how much data to consume (values greater than getSize() will limited)
      * @param actual_data_size how much data has actually been consumed. CHECK THIS VALUE!!!
      * @return data pointer to consume
      *
      * 
      *
      */
    void *consumeData( uint32_t consume_data_size, uint32_t& actual_data_size );

    /** @brief Consume ALL data
      *
      * Read the data from the returned pointer. DONT STORE THE POINTER!
      * Consumed data will be removed from the buffer and the memory reused.
      * Remember to ALWAYS release the pointer as soon as you are done calling unlockBuffer()!
      * This works like consumeData() but the returned pointer is always as big as all the data in the
      * buffer. Use getSize() to know how big it is.
      *
      * @since 4.0.3
      * @param size size of data
      * @return data pointer to consume
      */
    void *consumeAllData(uint32_t &size);

    /** @brief reset some amount of consumed data
     *
     * @since 4.0
     *
     * This will reduce the amount of actually consumed data after a call to consumeData().
     * Call this BEFORE calling unlockBuffer() and AFTER consumeData()!
     * This method is usefull when you asked for more data than you intended to consume, and now
     * you want to make sure that those extra bytes are still available to be consumed later.
     *
     * @param unconsumed_data_size size in bytes of data to "unconsume" at the end of the requested data to be consumed
     *
     */
    void unconsumeData( uint32_t unconsumed_data_size );

    /** @brief Read unconsumed data, without consuming
      *
      * Read the data from the returned pointer. DONT STORE THE POINTER!
      * Data will not be consumed. This is the raw data pointer from the beginning of the unconsumed data.
      * Remember to ALWAYS release the pointer as soon as you are done calling unlockBuffer()!
      *
      * @param len how much data is available in the returned buffer.
      * @return data pointer to consume, NULL if there is no unconsumed data
      *
      * 
      *
      */
    void* peekData( uint32_t & len );

    /** @brief Append some data
      *
      * Copy your data to the returned pointer. Do not copy more than you have indicated! and do not store the
      * pointer for later reuse.
      * Please also note that appending will remove some unconsumed data if it is necessary.
      * Remember to ALWAYS release the pointer as soon as you are done calling unlockBuffer()!
      * @param incoming_data_size how many bytes will be appended
      * @return pointer to use to store the data, NULL if the requested size does not fit into the buffer.
      *
      * 
      *
      */
    void* appendData(uint32_t incoming_data_size );

    /** @brief Release the pointer
      *
      * 
      * 
      *
      */
    void unlockBuffer();

    /** @brief for debug purposes, get internal bufer total size
     * @return total allocated memory
     * @since 3.3
      *
      * 
      *
     */
    uint32_t getTotalBufferSize() const;

    /** @brief Reset the buffer to empty
     * @since 4.0.3
     */
    void clear();

private:
    __private_CircularBuffer* _private;
};

}

#endif // CIRCULARBUFFER_H
