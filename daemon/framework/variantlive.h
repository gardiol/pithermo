#ifndef VARIANTLIVE_H
#define VARIANTLIVE_H

#include "common_defs.h"
#include "variantstatic.h"

namespace FrameworkLibrary {

/** @brief Implements a generic-data class which operates on shared data
 *
 * This class uses as storage backend a data pointer which is provided by the caller. This class will NEVER allocate or
 * release any memory. Speed-wise, this is the fastest implementation possible. Of course, you must make sure that the provided
 * data pointer is valid troughout the entire lifecycle of the object.
 *
 * Making a "copy" of a VariantLive is easy and fast. The copy-constructor will return a new VariantLive which operate on exactly the
 * same pointers as the original one: so modifying one will modify the other. If you need to create a "standalone" copy of the data,
 * you can call the copyToStatic() method which will create a VariantStatic from the VariantLive. This method will allocate
 * memory, so use wisely.
 *
 * It is possible to change the type of the Variant by calling reinterpretType() or recastType(). The first one will create a VariantStatic
 * copy of the VariantLive with a different type, without modifying the data. The latter will create a VariantStatic copy of the
 * VariantLive with a different type, and will cast the data to the new type.
 *
 * To create a VariantLive use one of the provided constructors, make sure the pointer remains valid troughout the life of the object.
 *
 * The VariantLive can be initialized with a read-only or a read/write data pointer. If you initialize with a const pointer, no write
 * operations will be possible. This allows you to create a constant (read-only) VariantLive.
 *
 *
 * @review Implement requirements
 * @review hide implementation
 *
 */
class DLLEXPORT VariantLive: public Variant
{
public:

    /** @brief Make a READ-WRITE live of a static Variant. It will use the other internal data buffer
     * @since 4.0.0
     * @param vl the variant which will be reinitialized to mirror the source one
     * @param other the static Variant to use as a source
     */
    static void initFromStaticRW( VariantLive& vl, VariantStatic& other );

    /** @brief Create an empty VariantLive
     */
    VariantLive();

    /** @brief Copy a VariantLive
      *
      * This constructor will create a copy of the VariantLive which will share the same live pointer.
      * The resulting Variant will see exactly the same data as the original.
      * @param other the Variant to use as source
      */
    VariantLive( const VariantLive& other );

    /** @brief Make a READ-ONLY live of a static Variant. It will use the other internal data buffer
     * @param other the static Variant to use as a source
     */
    VariantLive( const VariantStatic& other );

    /** @brief Create a VariantLive from this array of floats
     * @param v the data
     * @param array_size n. of elements in the array
     */
    VariantLive( float* v, uint32_t array_size = 1 );

    /** @brief Create a VariantLive from this array of doubles
     * @param v the data
     * @param array_size n. of elements in the array
     */
    VariantLive( double* v, uint32_t array_size = 1 );

    /** @brief Create a VariantLive from this array of uint8
     * @param v the data
     * @param array_size n. of elements in the array
     */
    VariantLive( uint8_t* v, uint32_t array_size = 1 );

    /** @brief Create a VariantLive from this array of uint16
     * @param v the data
     * @param array_size n. of elements in the array
     */
    VariantLive( uint16_t* v, uint32_t array_size = 1 );

    /** @brief Create a VariantLive from this array of uint32
     * @param v the data
     * @param array_size n. of elements in the array
     */
    VariantLive( uint32_t* v, uint32_t array_size = 1 );

    /** @brief Create a VariantLive from this array of uint64
     * @param v the data
     * @param array_size n. of elements in the array
     */
    VariantLive( uint64_t* v, uint32_t array_size = 1 );

    /** @brief Create a VariantLive from this array of int8
     * @param v the data
     * @param array_size n. of elements in the array
     */
    VariantLive( int8_t* v, uint32_t array_size = 1 );

    /** @brief Create a VariantLive from this array of int16
     * @param v the data
     * @param array_size n. of elements in the array
     */
    VariantLive( int16_t* v, uint32_t array_size = 1 );

    /** @brief Create a VariantLive from this array of int32
     * @param v the data
     * @param array_size n. of elements in the array
     */
    VariantLive( int32_t* v, uint32_t array_size = 1 );

    /** @brief Create a VariantLive from this array of int64
     * @param v the data
     * @param array_size n. of elements in the array
     */
    VariantLive( int64_t* v, uint32_t array_size = 1 );

    /** @brief Create a VariantLive from this array of char
     * @param v the data
     * @param array_size n. of elements in the array
     */
    VariantLive( char* v, uint32_t array_size = 1 );

    /** @brief Create a read-only VariantLive from this array of floats
     * @param v the data
     * @param array_size n. of elements in the array
     */
    VariantLive( const float* v, uint32_t array_size = 1 );

    /** @brief Create a read-only VariantLive from this array of doubles
     * @param v the data
     * @param array_size n. of elements in the array
     */
    VariantLive( const double* v, uint32_t array_size = 1 );

    /** @brief Create a read-only VariantLive from this array of uint8
     * @param v the data
     * @param array_size n. of elements in the array
     */
    VariantLive( const uint8_t* v, uint32_t array_size = 1 );

    /** @brief Create a read-only VariantLive from this array of uint16
     * @param v the data
     * @param array_size n. of elements in the array
     */
    VariantLive( const uint16_t* v, uint32_t array_size = 1 );

    /** @brief Create a read-only VariantLive from this array of uint32
     * @param v the data
     * @param array_size n. of elements in the array
     */
    VariantLive( const uint32_t* v, uint32_t array_size = 1 );

    /** @brief Create a read-only VariantLive from this array of uint64
     * @param v the data
     * @param array_size n. of elements in the array
     */
    VariantLive( const uint64_t* v, uint32_t array_size = 1 );

    /** @brief Create a read-only VariantLive from this array of int8
     * @param v the data
     * @param array_size n. of elements in the array
     */
    VariantLive( const int8_t* v, uint32_t array_size = 1 );

    /** @brief Create a read-only VariantLive from this array of int16
     * @param v the data
     * @param array_size n. of elements in the array
     */
    VariantLive( const int16_t* v, uint32_t array_size = 1 );

    /** @brief Create a read-only VariantLive from this array of int32
     * @param v the data
     * @param array_size n. of elements in the array
     */
    VariantLive( const int32_t* v, uint32_t array_size = 1 );

    /** @brief Create a read-only VariantLive from this array of int64
     * @param v the data
     * @param array_size n. of elements in the array
     */
    VariantLive( const int64_t* v, uint32_t array_size = 1 );

    /** @brief Create a read-only VariantLive from this array of char
     * @param v the data
     * @param array_size n. of elements in the array
     */
    VariantLive( const char* v, uint32_t array_size = 1 );

    /** @brief Create a variant from type and data pointer (read-only)
      *
      * This will use the data form the pointer as the internal Variant buffer.
      * Please make sure it remains valid until the VariantLive is destoyed.
      * @param type type of the data
      * @param ptr_data const pointer to the data to use as source for the variant
      * @param array_size number of items in the array (1 if not an array)
      */
    VariantLive( const TypeInfo& type, const void* ptr_data, uint32_t array_size = 1);

    /** @brief Create a variant from type and data pointer (read-write)
      *
      * This will use the data form the pointer as the internal Variant buffer.
      * Please make sure it remains valid until the VariantLive is destoyed.
      * @param type type of the data
      * @param ptr_data pointer to the data to use as source for the variant
      * @param array_size number of items in the array (1 if not an array)
      */
    VariantLive( const TypeInfo& type, void* ptr_data, uint32_t array_size = 1);

    virtual ~VariantLive();

public: // Copy...
    /** @brief Create a VariantStatic, which will be independent from this Variant
     *
     * Makes a copy of the data to a different buffer which will be allocated with new.
     * The created class will not share the data with this class.
     * @return independent copy of this variant
     */
    VariantStatic copyToStatic() const;

    /** @brief Make a binary copy of this VariantLive and change the type
     * @param type the new type
     * @return An independent copy of the data with the new type. The data is not casted, it has the same binary format as before.
     */
    VariantStatic reinterpretType( const TypeInfo& type ) const;

    /** @brief Make a copy of this VariantLive, change the type and cast the data to the new type
     * @param type the new type
     * @return An independent copy of the data with the new type. The data has been casted from the old type to the new type, element by element.
     */
    VariantStatic recastType( const TypeInfo& type ) const;

    /** @brief Assignment operator, so you can return a VariantLive easily!
     *
     * This will create a Variant which share the data from the original one.
     * @param other The source class
     * @return reference to other (for chaining)
     */
    const VariantLive& operator=(const VariantLive& other );

private:
    /** This is PRIVATE to prevent strange allocations if you erroneously try to
     * allocate the VariantLive from a type
     */
    VariantLive( const TypeInfo& );

    void getDataAt(uint32_t n, void *dest) const;
    void setDataAt(uint32_t n, const void *data);

    const void* _read_ptr;
    void* _write_ptr;    

};

}

#endif // VARIANTLIVE_H
