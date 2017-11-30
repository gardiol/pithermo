#ifndef VARIANT_H
#define VARIANT_H

#include <common_defs.h>
#include <typeinfo.h>
#include <string>
#include <vector>

namespace FrameworkLibrary{

/** @brief this class implements a generic data. It can contain any data type even as array.
 *
 * This class does not include a specific data storage, see VariantStatic for an internal storage approach or
 * VariantLive for an approach which reuses existing data pointers without overhead.
 *
 * There are different ways to access the stored data: one which keeps the binary representation of the data, one
 * which performs type-casting to keep the meaning of the data and one which let you read/write the data directly from
 * human-readable string.
 *
 * <b>WARNING ON BOOLEANS</b>: due to how bool is "inconsistently" defined by the C++ standard, casting and converting to/from bool
 * will give unexpected results. If you use a Variant with bool type inside, be aware that casting/converting to/from other types
 * might not yeld the expected result. This is a known issue and depend on the compiler underlying.
 *
 * How to use this class (short version)
 * =====================================
 *
 * Read this for a quick info guide. Read all to get the big picture and understand what you are doing.
 *
 * You can incapsulate ANY basic type in a Variant, and if you do like it's described here, it's also pretty fast.
 * There are two Variant: VariantStatic and VariantLive. VariantLive is FAST, since it uses your own data pointer
 * without doing any memory allocation internally. VariantStatic is what you need to use to actually store some data.
 * So, use VariantStatic as a containter and VariantLive as the means to pass data. It's pretty easy to get a copy VariantStatic
 * from a VariantLive, and a VariantLive from a VariantStatic.
 *
 * For example, here is a class which operate on a generic input:
@code
    class Input
    {
    public:
       Input( VariantLive initial_value )
       {
          setValue( initial_value );
       }
       void setValue( VariantLive value )
       {
          _value = value.copyToStatic();
       }
       VariantLive getValue()
       {
          return VariantLive( *this, false ); // false: not read only
       }
       VariantLive getValue() const;
       {
          return VariantLive( *this, true ); // true: read only
       }
    private:
       VariantStatic _value;
    }
@endcode
 *
 * The VariantLive is used whenever the actual Variant needs to be moved around, since it's so much light to create/destroy it
 * we do not need to mess with pointers at all. The VariantStatic, instead, is much better to actually store the data so we use it
 * as the actual storage.
 *
 * Aside from this, how do you actually USE a Variant? Well, just call any of the getAsXXX(), setFromXXX(), convertToXXX() access functions.
 * Depending on what you need to do, tipically you:
 * 1. Create the Variant with the type you need, for example:
@code
    VariantStatic MyVar( TypeInfo::float_field );
    VariantStatic MyArray( TypeInfo::float_field, 20 );
    uint32_t tmpUint = 7;
    VariantStatic MyUint;
    VariantStatic::initFromUin32( MyUint, tmpUint );
    VariantLive liveMyVar( _value.getLiveReference() );
    double tempDouble = 2.0;
    VariantLive liveDouble( TypeInfo::double_field, &tempDouble );
@endcode
 * 2. Read it's value by casting it to the type you want, or print it as a human-readable string:
@code
    printf("My var is: %f (as float)\n", MyVar.convertToFloat() );
    printf("My var is: %d (as integer)\n", MyVar.convertToInt32() );
    printf("My var is: %s (this is formatted automatically)\n", MyVar.toString() );
@endcode
 * 3. Set it's value from any type:
@code
    MyVar.setFromFloat( 1.0 );
    MyVar.setFromInt32( 7 );
@endcode
 * 4. You can also force a conversion (force the binary format regardless of the type, like ADA's unchecked conversion or a C raw memcpy):
@code
    printf("A float, when force (not casted!) to int32 will look like: %d\n", MyVar.getAsInt32() );
@endcode
 *
 * You are done, more or less. Check both VariantLive and VariantStatic for more information on how to convert beetwen them and how they work.
 *
 * Acces ignoring Binary Representation
 * ====================================
 *
 * The Variant class provides methods to get/set the underlying data at binary level. These methods are useful when
 * you are sure of the type of data or you want to impose a specific binary format to the data.
 *
 * The methods getAsFloat(), getAsUint8(), getAsInt32() and, in general, the getAsXXX() let you read the stored data
 * using the same binary representation of the indicated type. For example, if the underlying data format is float and the stored
 * data is 15.15 at binary level, the getAsUint32() will return the value "1098016358", this unsigned integer on 32bit has the same binary
 * data bits of the same float value.
 *
 * The method setRaw() let you set the binary data without checking the type: basically, it's a memcpy between the given data pointer and
 * the internal storage.
 *
 * The method getRaw() let you access the binary data without checking the type: it's a memcpy from the internal storage and your
 * data pointer.
 *
 * Access with type-Casting
 * ========================
 *
 * The methods indicated here let the user get/set the underlying data ensuring an appropriate type cast is performed on the data.
 * For example, using these methods, you can cast from a stored float to a returned integer, or you can set a stored unsigned integer from
 * any other type making sure the meaning of the stored number is properly converted. These are the methods you usually want to use!
 *
 * The methods setFromFloat(), setFromUint16(), setFromInt64() and, in general, the setFromXXX() let you set a value with the appropriate
 * cast. For example, if the underlying data format is int32 and you call setFromFloat( 2.2 ), the data will be cast to 2 (int) and the data
 * will contain the integer on 32 bit with value "2".
 *
 * The methods convertToFloat(), convertToInt32(), convertToUint64() and, in general, the convertToXXX(), let you get the stored data ensuring
 * it's casted to the desired type. For example, if the underlying data format is float and the stored
 * data is 15.15 at binary level, the convertToUint32() will return the value "15", which is the casted value of float to uint32.
 *
 * String data access
 * ==================
 *
 * The Variant class can convert to and from strings. After setting the data type, you can automatically read and convert
 * a data from a human-readable string (like "45.437" or "-32764"). The methods fromString() is capable of performing this kind
 * of data conversion. Think of it as an sscanf(). Of course, the data type must be known in advance.
 *
 * The method toString() instead can be used to write the underlying data to a human-readable string, like the above.
 *
 * Serializing data
 * ================
 *
 * It is possible to "serialize" the underlying data by calling the method serialize(). The returned string can be used with the
 * appropriate class constructor to create a new Variant, for example after sending the serialized string over the network.
 *
 * The format of the serialized string is:
 * 2 bytes: version watermark (fixed to 0xA5)
 * 1 byte: type (see TypeInfo class)
 * 1 byte: size in bytes of the type (S, as returned by TypeInfo)
 * 4 bytes: array count (N)
 * NxS bytes: the data
 *
 * @warning this format is NOT COMPATIBLE with the Serializable class!
 *
 * @warning long long doubles on 64bit are usually 80bits, like some specific hardware floating point types. These are currently not supported.
 *
 * Comparison and assignment
 * =========================
 *
 * No assignment operator is defined. It is left to child classes implementing the storage to define assignment operators of any kind.
 * Only a copy-constructor for Variant is defined, protected. It can be used only from derived classes.
 *
 * Comparison functions are provided, but to avoid errors, no comparison operator is specified (you would not know whether type is considered
 * or if type-cast is used or ignored).
 *
 * Use the isBinaryIdenticalTo() to check if two Variant share the exactly same binary data, without checking for the type.
 * Use the isIdenticalTo() to check is two Variant have both the same type AND binary data. The latter is usually faster when
 * the type differs, since the binary data is not checked in this case. The first might be slightly faster instead when you are
 * sure that the type is identical, because the type check is skipped.
 *
 * Use the isTypeIdentical() to only check is both Variant have the same type.
 *
 * If you need to compare two Variant classes by casting the contained data types, then you need to use isEqualTo() which will check
 * if the elements satisfy C/C++ "==" operator by casting the second Variant to the same type of the first one.
 *
 * The two methods isLessThan() and isMoreThan() return true if the other Variant is less/more than the current Variant. Be carefull if the
 * two types does not match because an internal type-cast is performed. It's like the standard C/C++ operators < and >: if the two types
 * does not match, the compiler will do a forced type cast for you and, usually, you are in trouble!
 *
 * Copy data
 * =========
 *
 * Two different methods are provided to copy data from one Variant to another Variant:
 * - copyBinaryData(): will perform a binary-level copy of the data without type-checking. This will fail if the two types have different sizes.
 * - copyData(): will perform a cast of each element of the data.
 *
 * Speed issues and considerations
 * ===============================
 *
 * The architecture of the Variant has been studied to guarantee the maximum possible speed. Using the Variant properly should not
 * cause significant performance loss over using native types directly, at least for most critical oeprations.
 *
 * How the data is actually stored internally depends on the child class, so your actual speed can vary according to this, please check
 * each child class you want to use for further details.
 *
 *
 *
 * @review Implement requirements
 * @review hide implementation
 *
 */
class DLLEXPORT Variant
{
public: // Lifecycle and metadata access
    virtual ~Variant();

    /** @brief return size of array (1 if not an array)
     * @return number of data items contained in the Variant
     */
    uint32_t getArraySize() const;

    /** @brief get type of the data contained in the Variant
     * @return type of the data
     */
    TypeInfo getType() const;

public: // String conversion and serialization
    /** @brief serialize the Variant to a string
      *
      * Please note this is NOT compatible with Serializable class!
      * @return a serialized string (to unserialize, see VariantStatic class)
      */
    std::string serialize() const;

    /** @brief Get a human-readable string representation of the data.
     *
     * This method will return a representation of the data which is understandable by a human being.
     * It depends on the internal type. For example:
     * - floats: 10.5
     * - integer: 45
     * - char: a
     * - boolean: true
     * and so on.
     *
     * Arrays are managed like this:
     * - non-char arrays (es. integer arrays): 5,9,12,563,33,1633
     * - char arrays: "example array"
     *
     * You can specify the array range you want to primt:
     * - from: starting position (starts from 0)
     * - to: first position NOT to be printed
     *
     * Given the string "abcdefghi", from = 3, to = 6 will print "def". If to is less than from, only one character will
     * be printed. If from is higher than the array size, only the last character will be printed.
     *
     * @param pos_from array position to print from, use -1 for printing entire array
     * @param pos_to array position to print up to. Use -1 to print only the "pos_from" element.
     * @return the item converted to human readable string
     */
    std::string toHumanString(int32_t pos_from = -1, int32_t pos_to = -1) const;

    /** @brief convert a human readable string into the store data
     *
     * This method will set the values of the Variant from a human-readable string which is equivalent to the one generated by
     * toHumanString(). This method can deal with arrays if they are in the same format of toString(): value1, value2, value3, ..., valueN
     * of course arrays will be copied only up to fill the Variant array size, extra values will be ignored.
     *
     * You can copy only one value by specifying the internal_pos parameter or deal with the entire array by passing -1 as parameter.
     * Internally, the string will be parsed to check if it's an array.
     *
     * When parsing chars, the following formats are accepted:
     * - 0x01: the hex value 01 will be stored into the char
     * - a: the char "a" is stored into the char
     * - "a": the char "a" is stored into the char, the " are discarded
     * - 'a': the char "a" is stored into the char, the ' are discarded
     * If the variant type is char, then as for the opposite function, the string must be passed as a normal string.
     * For example: "abcdefgh" will be loaded in the variant as an array of chars, up to the array size of the variant.
     *
     * In general, if the input array is shorter than the variant array size, then the remaining array positions are filled
     * with zeros.
     *
     * @note that type must be set for this method to work, type will NOT be auto-guessed.
     * @note that, for arrays, all the items of the string shall be of the same type.
     * @note that this methods require a writeable underlying data storage (es: VariantStatic and VariantLive limited to writeble pointers)
     * @param string the human readable string containing the value (which can be a comma separated string)
     */
    void fromHumanString(const std::string& string );

public: // Binary data access (no type casting)
    /** @brief get the data as if it's a float, disregaring it's internal format
     * @param array_pos which element of the array to read and convert
     * @return the internal data seen as float (not casted)
     */
    float getAsFloat( uint32_t array_pos = 0) const;

    /** @brief get the data as if it's a string, disregaring it's internal format
     * @since 4.0.0, evolved to return partial string
     *
     * @param start_pos which character of the source string to start from
     * @param len how many charecters to copy ( 0  for entire string )
     * @return the internal data seen as float (not casted)
     */
    std::string getAsString( uint32_t start_pos = 0, uint32_t len = 0) const;

    /** @brief get the data as if it's a double, disregaring it's internal format
     * @param array_pos which element of the array to read and convert
     * @return the internal data seen as double (not casted)
     */
    double getAsDouble( uint32_t array_pos = 0) const;

    /** @brief get the data as if it's a int8_t, disregaring it's internal format
     * @param array_pos which element of the array to read and convert
     * @return the internal data seen as int8_t (not casted)
     */
    int8_t getAsInt8( uint32_t array_pos = 0) const;

    /** @brief get the data as if it's a uint8_t, disregaring it's internal format
     * @param array_pos which element of the array to read and convert
     * @return the internal data seen as uint8_t (not casted)
     */
    uint8_t getAsUint8( uint32_t array_pos = 0) const;

    /** @brief get the data as if it's a int16_t, disregaring it's internal format
     * @param array_pos which element of the array to read and convert
     * @return the internal data seen as int16_t (not casted)
     */
    int16_t getAsInt16( uint32_t array_pos = 0) const;

    /** @brief get the data as if it's a uint16_t, disregaring it's internal format
     * @param array_pos which element of the array to read and convert
     * @return the internal data seen as uint16_t (not casted)
     */
    uint16_t getAsUint16( uint32_t array_pos = 0) const;

    /** @brief get the data as if it's a int32_t, disregaring it's internal format
     * @param array_pos which element of the array to read and convert
     * @return the internal data seen as int32_t (not casted)
     */
    int32_t getAsInt32( uint32_t array_pos = 0) const;

    /** @brief get the data as if it's a uint32_t, disregaring it's internal format
     * @param array_pos which element of the array to read and convert
     * @return the internal data seen as uint32_t (not casted)
     */
    uint32_t getAsUint32( uint32_t array_pos = 0) const;

    /** @brief get the data as if it's a int64_t, disregaring it's internal format
     * @param array_pos which element of the array to read and convert
     * @return the internal data seen as int64_t (not casted)
     */
    int64_t getAsInt64( uint32_t array_pos = 0) const;

    /** @brief get the data as if it's a uint64_t, disregaring it's internal format
     * @param array_pos which element of the array to read and convert
     * @return the internal data seen as uint64_t (not casted)
     */
    uint64_t getAsUint64( uint32_t array_pos = 0) const;

    /** @brief get the data as if it's a char, disregaring it's internal format
     * @param array_pos which element of the array to read and convert
     * @return the internal data seen as char (not casted)
     */
    char getAsChar( uint32_t array_pos = 0) const;

    /** @brief get the data as if it's a boolean, disregaring it's internal format
     * @param array_pos which element of the array to read and convert
     * @return the internal data seen as boolean (not casted)
     */
    bool getAsBool( uint32_t array_pos = 0) const;

    /** @brief set the raw bit data representation
     * @param array_pos which element of the array to set
     * @param v raw bits to set: make sure you pass enough bits to fill the internal type!
     */
    void setRaw( const void* v, uint32_t array_pos = 0 );

    /** @brief set the raw bit data representation into an array
     *
     * @note The source array is read always starting from position 0.
     * @since 3.3
     * @note For boolean type, the "raw" is an unsigned int on 32bits (1=true, 0=false)
     * @param start_pos set from this position (relative to the variant array, not the source array)
     * @param end_pos set up to this position, included. (relative to the variant array, not the source array)
     * @param v raw bits to fill: make sure you pass enough bits to fit the internal type and array positions!
     */
    void setRawArray(const void *v, uint32_t start_pos, uint32_t end_pos );

    /** @brief get the raw bit data representation
     * @param array_pos which element of the array to get
     * @param v raw bits to fill: make sure you pass enough bits to fit the internal type!
     */
    void getRaw(void *v, uint32_t array_pos = 0 ) const;

    /** @brief get the raw bit data representation into an array
     *
     * @note The destination array is filled always starting from position 0.
     * @note For boolean type, the "raw" is an unsigned int on 32bits (1=true, 0=false)
     * @since 3.3
     * @param start_pos get from this position (relative to the variant array, not the destination array)
     * @param end_pos get up to this position, included. (relative to the variant array, not the destination array)
     * @param v raw bits to fill: make sure you pass enough bits to fit the internal type and array positions!
     */
    void getRawArray(void *v, uint32_t start_pos, uint32_t end_pos ) const;

public: // Type-cast data access
    /** @brief Cast the data to float, the internal format will be converted to float.
     * @param array_pos which element of the array to read and convert
     * @return the internal data casted to float
     */
    float convertToFloat( uint32_t array_pos = 0) const;

    /** @brief Cast the data to double, the internal format will be converted to double.
     * @param array_pos which element of the array to read and convert
     * @return the internal data casted to double
     */
    double convertToDouble( uint32_t array_pos = 0) const;

    /** @brief Cast the data to int8_t, the internal format will be converted to int8_t.
     * @param array_pos which element of the array to read and convert
     * @return the internal data casted to int8_t
     */
    int8_t convertToInt8( uint32_t array_pos = 0) const;

    /** @brief Cast the data to int16_t, the internal format will be converted to int16_t.
     * @param array_pos which element of the array to read and convert
     * @return the internal data casted to int16_t
     */
    int16_t convertToInt16( uint32_t array_pos = 0) const;

    /** @brief Cast the data to int32_t, the internal format will be converted to int32_t.
     * @param array_pos which element of the array to read and convert
     * @return the internal data casted to int32_t
     */
    int32_t convertToInt32( uint32_t array_pos = 0) const;

    /** @brief Cast the data to int64_t, the internal format will be converted to int64_t.
     * @param array_pos which element of the array to read and convert
     * @return the internal data casted to int64_t
     */
    int64_t convertToInt64( uint32_t array_pos = 0) const;

    /** @brief Cast the data to uint8_t, the internal format will be converted to uint8_t.
     * @param array_pos which element of the array to read and convert
     * @return the internal data casted to uint8_t
     */
    uint8_t convertToUint8( uint32_t array_pos = 0) const;

    /** @brief Cast the data to uint16_t, the internal format will be converted to uint16_t.
     * @param array_pos which element of the array to read and convert
     * @return the internal data casted to uint16_t
     */
    uint16_t convertToUint16( uint32_t array_pos = 0) const;

    /** @brief Cast the data to uint32_t, the internal format will be converted to uint32_t.
     * @param array_pos which element of the array to read and convert
     * @return the internal data casted to uint32_t
     */
    uint32_t convertToUint32( uint32_t array_pos = 0) const;

    /** @brief Cast the data to uint64_t, the internal format will be converted to uint64_t.
     * @param array_pos which element of the array to read and convert
     * @return the internal data casted to uint64_t
     */
    uint64_t convertToUint64( uint32_t array_pos = 0) const;

    /** @brief Cast the data to char, the internal format will be converted to char.
     * @param array_pos which element of the array to read and convert
     * @return the internal data casted to char
     */
    char convertToChar( uint32_t array_pos = 0) const;

    /** @brief Cast the data to boolean, the internal format will be converted to bool.
     * @param array_pos which element of the array to read and convert
     * @return the internal data casted to bool
     */
    bool convertToBool( uint32_t array_pos = 0) const;

    /** @brief Cast the data as if it's a C++ standard string, the internal format will be converted to a string of char
     * @since 4.0.0 added partial string extraction
     *
     * @note This function might not do what you expect. It will consider EACH element of the variant array as a single char. Will not transform the variant to a human-readable string.
     * The string will have the same size of the array.
     *
     * @param start_pos initial character to start the string from
     * @param len number of characters to copy into the string (0 for the entire string)
     * @return the internal data casted to a string of char
     */
    std::string convertToString( uint32_t start_pos = 0, uint32_t len = 0) const;

    /** @brief update the internal data with a float (will be casted to the internal type)
     * @param array_pos which element of the array to update
     * @param v the value to write
     */
    void setFromFloat( float v, uint32_t array_pos = 0 );

    /** @brief update the internal data with a double (will be casted to the internal type)
     * @param array_pos which element of the array to update
     * @param v the value to write
     */
    void setFromDouble( double v, uint32_t array_pos = 0 );

    /** @brief update the internal data with a int8_t (will be casted to the internal type)
     * @param array_pos which element of the array to update
     * @param v the value to write
     */
    void setFromInt8( int8_t v, uint32_t array_pos = 0 );

    /** @brief update the internal data with a uint8_t (will be casted to the internal type)
     * @param array_pos which element of the array to update
     * @param v the value to write
     */
    void setFromUint8( uint8_t v, uint32_t array_pos = 0 );

    /** @brief update the internal data with a int16_t (will be casted to the internal type)
     * @param array_pos which element of the array to update
     * @param v the value to write
     */
    void setFromInt16( int16_t v, uint32_t array_pos = 0 );

    /** @brief update the internal data with a uint16_t (will be casted to the internal type)
     * @param array_pos which element of the array to update
     * @param v the value to write
     */
    void setFromUint16( uint16_t v, uint32_t array_pos = 0 );

    /** @brief update the internal data with a int32_t (will be casted to the internal type)
     * @param array_pos which element of the array to update
     * @param v the value to write
     */
    void setFromInt32( int32_t v, uint32_t array_pos = 0 );

    /** @brief update the internal data with a uint32_t (will be casted to the internal type)
     * @param array_pos which element of the array to update
     * @param v the value to write
     */
    void setFromUint32( uint32_t v, uint32_t array_pos = 0 );

    /** @brief update the internal data with a int64_t (will be casted to the internal type)
     * @param array_pos which element of the array to update
     * @param v the value to write
     */
    void setFromInt64( int64_t v, uint32_t array_pos = 0 );

    /** @brief update the internal data with a uint64_t (will be casted to the internal type)
     * @param array_pos which element of the array to update
     * @param v the value to write
     */
    void setFromUint64( uint64_t v, uint32_t array_pos = 0 );

    /** @brief update the internal data with a char (will be casted to the internal type)
     * @param array_pos which element of the array to update
     * @param v the value to write
     */
    void setFromChar( char v, uint32_t array_pos = 0 );

    /** @brief update the internal data with a boolean (will be casted to the internal type)
     * @param array_pos which element of the array to update
     * @param v the value to write
     */
    void setFromBool( bool v, uint32_t array_pos = 0 );

    /** @brief update the internal data with a string (will be casted to the internal type)
     *
     * This will copy as many chars from the string as will fit into the array size of the variant.
     * @note To actually resize the variant to store a larger string, you MUST call VariantStatic string constructor.
     * @since 3.4
     * @param str the string to write
     */
    void setFromString( const std::string& str );

public: // Comparison and math
    /** @brief Checks if the data are identical, using a type cast if necessary.
     *
     * This is not a binary check, it actually casts one type to the other if they don't match.
     * @param other the other Variant
     * @param array_pos which element of the array to check, -1 for all
     * @return true if they are identical after type cast, false otherwise.
     */
    bool isEqualTo( const Variant& other, int32_t array_pos = -1 ) const;


    /** @brief Checks if the two binary data are identical, ignore type.
     *
     * This can be slightly faster if type is identical, because only binary data is compared.
     * @param other the other Variant
     * @param array_pos which element of the array to check, -1 for all
     * @return true if they are identical at binary level (type is not checked), false otherwise.
     */
    bool isBinaryIdenticalTo( const Variant& other, int32_t array_pos = -1 ) const;

    /** @brief Checks if the two are identical both in type and binary data.
     *
     * This can be faster if type is not identical, because it's checked before binary data is compared.
     * @param other the other Variant
     * @param array_pos which element of the array to check, -1 for all
     * @return true if they are identical.
     */
    bool isIdenticalTo( const Variant& other, int32_t array_pos = -1 ) const;

    /** @brief Check if the two have the same type
     * @param other the other Variant
     * @return true if they have the same type
     */
    bool isTypeIdentical( const Variant& other ) const;

    /** @brief checks if this value is less than the other (automatic type cast will be done if needed)
     * @param other the other Variant
     * @param array_pos which element of this array to check
     * @param other_array_pos which element of the other array to check
     * @return true this is less than the other.
     */
    bool isLessThan(const Variant& other, int32_t array_pos = 0 , int32_t other_array_pos = 0) const;

    /** @brief checks if this value is more than the other (automatic type cast will be done if needed)
     * @param other the other Variant
     * @param array_pos which element of the array to check
     * @param other_array_pos which element of the other array to check
     * @return true this is more than the other.
     */
    bool isMoreThan( const Variant& other, int32_t array_pos = 0, int32_t other_array_pos = 0 ) const;

public: // Data copying
    /** @brief Copy data elements, binary-wise. No type check is done.
     * @param other the source Variant
     * @return true if copy was successful
     */
    bool copyBinaryData( const Variant& other );

    /** @brief Copy data elements, casting types for each element
     * @param other the source Variant
     * @return true if copy was successful
     */
    bool copyData( const Variant& other );

protected:
    /** @brief Copy constructor
     * @param other the other Variant
     */
    Variant( const Variant& other );

    /** @brief Create a new Variant given type and size.
     * @param type type of the data
     * @param array_size now many elements in the array (1 by default)
     */
    Variant( const TypeInfo& type, uint32_t array_size = 1 );

    /** @brief read data
     *
     * Access your own storage backend here
     * @param n Array element to access
     * @param dest pointer to read the data from (size must match type size)
     */
    virtual void getDataAt( uint32_t n, void* dest ) const = 0;
    /** @brief write data
     *
     * Access your own storage backend here
     * @param n Array element to access
     * @param src pointer to store the accessed data (size must match type size)
     */
    virtual void setDataAt( uint32_t n, const void* src ) = 0;

    /** @brief Unpack a serialized string. Will allocate a new buffer internally, rememebr to delete it with "delete []"!!!
     * @param blob the serialized data
     * @param type will be filled with the serialized type
     * @param array_size will be filled with the serialized array size
     * @return a new allocated buffer containing the data, please remember to free it with delete [] when done.
     */
    static uint8_t *unserialize( const std::string& blob, TypeInfo& type, uint32_t& array_size );

    TypeInfo _type; /**< Type of data */
    uint32_t _array_size; /**< Array size of data */
    uint32_t _type_size; /**< Size of type. For caching purposes */

};

}

#endif // VARIANT_H
