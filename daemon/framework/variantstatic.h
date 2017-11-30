#ifndef VARIANTSTATIC_H
#define VARIANTSTATIC_H

#include "variant.h"

namespace FrameworkLibrary {

class VariantLive;
/** @brief Implements a generic-data class which contains it's own data
 *
 * A VariantStatic is a Variant which stores the data internally in a locally allocated memory buffer.
 * When speed is a concern, please rememeber that allocating a new VariantStatic requires a new and deallocating it requires
 * a delete. VariantLive instead does not, so if speed is a serious issue please consider use VariantLive on a common data buffer.
 *
 * The VariantStatic guarantees that each instance has it's own internal independent copy of the data, thus modifiying ome VariantStatic
 * will never alter any other VariantStatic (this is what "Static" means here).
 *
 * When you need to pass a VariantStatic around, pass it by reference or by pointer, or better use a VariantLive by
 * creating it with the VariantLive constructor which requires a VariantStatic.
 *
 * The copy-constructor and and assignment operator have been defined in order to properly use this class.
 * Just please remember that the assignment operator is potentially SLOW, so use it only when really needed, convert to VariantLive
 * instead and move that one around.
 *
 * To create a new VariantStatic use one of the provided cosntructors.
 *
 * To make a copy of a VariantStatic use the assignment operator or go trough a VariantLive.
 *
 *
 * @todo Add resizing of the internal storage
 *
 * @review Implement requirements
 * @review hide implementation
 *
 */
class DLLEXPORT VariantStatic: public Variant
{
    friend class VariantLive;
public: // Static constructors

    /** @brief Initialize a VariantStatic from a serialized string
     * @since 4.0.0
     * @note The variant passed as parameter will be reinitialized with the unserialized data.
     *
     * If the blob cannot be unserialized properly, the variant will not be modified and false will be returned.
     * @param blob the serialized string
     * @param variant the variant which will be initialized with the serialized data
     * @return true if the blob has been unserialized properly, false otherwise.
     */
    static bool initFromSerialized( VariantStatic& variant, const std::string& blob );

    /** @brief Initialize a VariantStatic from one float
     * @since 4.0.0
     * @note The variant passed as parameter will be reinitialized with the data.
     *
     * This will initialize the variant as a single item of type float and value v
     * @param v The data
     * @param variant the variant which will be initialized with the data
     */
    static void initFromFloat( VariantStatic& variant, float v );

    /** @brief Initialize a VariantStatic from one double
     * @since 4.0.0
     * @note The variant passed as parameter will be reinitialized with the data.
     *
     * This will initialize the variant as a single item of type double and value v
     * @param v The data
     * @param variant the variant which will be initialized with the data
     */
    static void initFromDouble( VariantStatic& variant, double v );

    /** @brief Initialize a VariantStatic from one uint8_t
     * @since 4.0.0
     * @note The variant passed as parameter will be reinitialized with the data.
     *
     * This will initialize the variant as a single item of type uint8_t and value v
     * @param v The data
     * @param variant the variant which will be initialized with the data
     */
    static void initFromUint8( VariantStatic& variant, uint8_t v );

    /** @brief Initialize a VariantStatic from one uint16_t
     * @since 4.0.0
     * @note The variant passed as parameter will be reinitialized with the data.
     *
     * This will initialize the variant as a single item of type uint16_t and value v
     * @param v The data
     * @param variant the variant which will be initialized with the data
     */
    static void initFromUint16( VariantStatic& variant, uint16_t v );

    /** @brief Initialize a VariantStatic from one uint32_t
     * @since 4.0.0
     * @note The variant passed as parameter will be reinitialized with the data.
     *
     * This will initialize the variant as a single item of type uint32_t and value v
     * @param v The data
     * @param variant the variant which will be initialized with the data
     */
    static void initFromUint32( VariantStatic& variant, uint32_t v );

    /** @brief Initialize a VariantStatic from one uint64_t
     * @since 4.0.0
     * @note The variant passed as parameter will be reinitialized with the data.
     *
     * This will initialize the variant as a single item of type uint64_t and value v
     * @param v The data
     * @param variant the variant which will be initialized with the data
     */
    static void initFromUint64( VariantStatic& variant, uint64_t v );

    /** @brief Initialize a VariantStatic from one int8_t
     * @since 4.0.0
     * @note The variant passed as parameter will be reinitialized with the data.
     *
     * This will initialize the variant as a single item of type int8_t and value v
     * @param v The data
     * @param variant the variant which will be initialized with the data
     */
    static void initFromInt8( VariantStatic& variant, int8_t v );

    /** @brief Initialize a VariantStatic from one int16_t
     * @since 4.0.0
     * @note The variant passed as parameter will be reinitialized with the data.
     *
     * This will initialize the variant as a single item of type int16_t and value v
     * @param v The data
     * @param variant the variant which will be initialized with the data
     */
    static void initFromInt16( VariantStatic& variant, int16_t v );

    /** @brief Initialize a VariantStatic from one int32_t
     * @since 4.0.0
     * @note The variant passed as parameter will be reinitialized with the data.
     *
     * This will initialize the variant as a single item of type int32_t and value v
     * @param v The data
     * @param variant the variant which will be initialized with the data
     */
    static void initFromInt32( VariantStatic& variant, int32_t v );

    /** @brief Initialize a VariantStatic from one int64_t
     * @since 4.0.0
     * @note The variant passed as parameter will be reinitialized with the data.
     *
     * This will initialize the variant as a single item of type int64_t and value v
     * @param v The data
     * @param variant the variant which will be initialized with the data
     */
    static void initFromInt64( VariantStatic& variant, int64_t v );

    /** @brief Initialize a VariantStatic from one boolean
     * @since 4.0.0
     * @note The variant passed as parameter will be reinitialized with the data.
     *
     * This will initialize the variant as a single item of type bool and value v
     * @param v The data
     * @param variant the variant which will be initialized with the data
     */
    static void initFromBool( VariantStatic& variant, bool v );

    /** @brief Initialize a VariantStatic from one char
     * @since 4.0.0
     * @note The variant passed as parameter will be reinitialized with the data.
     *
     * This will initialize the variant as a single item of type char and value v
     * @param v The data
     * @param variant the variant which will be initialized with the data
     */
    static void initFromChar( VariantStatic& variant, char v );

    /** @brief Initialize a VariantStatic from a char string
     * @since 4.0.0
     * @note The variant passed as parameter will be reinitialized with the data.
     *
     * This will initialize the variant as an array of the same size of the string and value v
     * The string terminator will NOT be included in the variant.
     * @param v The data
     * @param variant the variant which will be initialized with the data
     */
    static void initFromString( VariantStatic& variant, const std::string& v );

    /** @brief Initialize a VariantStatic from a char string
     * @since 4.0.0
     * @note The variant passed as parameter will be reinitialized with the data.
     *
     * This will initialize the variant as an array of the same size of the string and value v
     * The string terminator will NOT be included in the variant.
     * @param v The data
     * @param variant the variant which will be initialized with the data
     */
    static void initFromString( VariantStatic& variant, const char* v );



public: // normal constructors
    /** @brief Initialize an empty Variant (type will be unknown)
      */
    VariantStatic();

    /** @brief Create a duplicate of other
     * @param other The VariantStatic which will be duplicated.
     */
    VariantStatic( const VariantStatic& other );

    /** @brief Create a variant from type, empty.
      * @param type type of the data
      * @param array_size number of items in the array (1 if not an array, 0 is invalid)
      */
    VariantStatic( const TypeInfo& type, uint32_t array_size );

    /** @brief Create a variant from type and value (non array)
      * @since 4.0.0, the array_size paramter has been removed and defaults to 1.
      * @param type type of the data, this can be an array with this constructor if the string is in the array form (a,b,c,d) in this case specify array_size.
      * @param value value of the type (in ASCII string format, human readable, to be generic)
      */
    VariantStatic( const TypeInfo& type, const std::string& value );

    /** @brief Create a variant from type and data pointer
      *
      * This will perform a copy of the data form the pointer to the internal Variant buffer.
      * @warning It's up to you to make sure that the data fits array_size * type_size!
      *
      * @param type type of the data
      * @param ptr_data pointer to the data to use as source for the variant
      * @param array_size number of items in the array
      */
    VariantStatic( const TypeInfo& type, const void* ptr_data, uint32_t array_size);

    virtual ~VariantStatic();

public: // Operators and duplicators
    /** @brief Assignment operator
     *
     * Make possible to write:
     *      VariantStatic a;
     *      VariantStatic b = a;
     * @param other the other VariantStatic
     * @return reference to other, for operator chaining (a = b = c)
     */
    const VariantStatic& operator=(const VariantStatic&other);

private:
    void prepareBuffer();

    void getDataAt(uint32_t n, void *dest) const;
    void setDataAt(uint32_t, const void *data);

    uint8_t* _data_store;

};

}

#endif // VARIANTSTATIC_H
