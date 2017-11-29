#ifndef TYPEINFO_H
#define TYPEINFO_H

#include <common_defs.h>
#include <string>
#include <set>
#include <list>

namespace FrameworkLibrary {

/** @brief defines a type (used by Serializable and Variant, mostly)
  *
  * This class implements a generic basic type definition. It provides type and default size of the type.
  *
  * 
  *
  */
class DLLEXPORT TypeInfo
{
public:
    /** @brief type of data */
    enum type {   int8_field   = 0, /**< Integer on 8 bits, signed */
                  int16_field  = 1, /**< Integer on 16 bits, signed */
                  int32_field  = 2, /**< Integer on 32 bits, signed */
                  int64_field  = 3, /**< Integer on 64 bits, signed */
                  uint8_field   = 4, /**< Integer on 8 bits, unsigned */
                  uint16_field  = 5, /**< Integer on 16 bits, unsigned */
                  uint32_field  = 6, /**< Integer on 32 bits, unsigned */
                  uint64_field  = 7, /**< Integer on 64 bits, unsigned */
                  float_field  = 8, /**< Floating point on 32 bit */
                  double_field = 9, /**< Double precision floating point on 64bit */
                  bool_field  = 11, /**< boolean (usually on 32bit) */
                  char_field  = 12, /**< usually, an ASCII character (same as int8_field) */
                  no_type_field = 0xFC, /**< No type  */
                  raw_field = 0xFD, /**< Use this for generic byte streams */
                  string_field = 0xFE, /**< Don't use this, it's mostly for use on Serializable items */
                  unknown_type = 0xFF }; /**< Unknown type, size will be 0  */

    /** @brief Get list of valid types as enum
     * @return list of types as enum
      *
      * 
      *
     */
    static std::set<type> listEnumType();

    /** @since 3.4
    * @brief Get list of valid types as TypeInfo objects
     * @return list of types as TypeInfo's
      *
      * 
      *
     */
    static std::list<TypeInfo> listTypes();

    /** @since 3.4
    * @brief Get list of valid types as strings
     * @return list of types as strings
     */
    static std::list<std::string> listTypesStr();

    /** @brief create net TypeInfo from integer
     * @param t the type
      *
      * 
      *
     */
    TypeInfo( uint8_t t );

    /** @brief create net TypeInfo from enum
     * @param t the type
      *
      * 
      *
     */
    TypeInfo( type t = unknown_type );

    /** @brief create net TypeInfo from string
     * @param str the type
      *
      * 
      *
     */
    TypeInfo( const std::string& str );

    /** @brief create net TypeInfo from existing TypeInfo
     * @param other the type
      *
      * 
      *
     */
    TypeInfo( const TypeInfo& other );

    /** @brief get type
     * @return type
      *
      * 
      *
     */
    type getType() const;

    /** @brief conver to int
     * @return type
      *
      * 
      *
     */
    operator uint8_t() const;

    /** @brief compare type
     * @param other other type
     * @return true if equal
      *
      * 
      *
     */
    bool operator==(const TypeInfo& other ) const;

    /** @brief compare type
     * @param t other type
     * @return true if equal
      *
      * 
      *
     */
    bool operator==( uint8_t t ) const;

    /** @brief compare type
     * @param t other type
     * @return true if equal
      *
      * 
      *
     */
    bool operator==( type t ) const;

    /** @since 3.4
     * @brief compare type
     * @param s other type
     * @return true if equal
      *
      * 
      *
     */
    bool operator==( const std::string& s ) const;

    /** @brief check if type is unknown
     * @return true if unknown
      *
      * 
      *
     */
    bool isUnknown() const;

    /** @brief Translates a type name to it's string reresentation
      *
      * ex: int32_field, will return the string "int32_field"
      * @return The string representing the type
      *
      * 
      *
      */
    std::string getTypeStr() const;

    /** @brief Return the size (in bytes) of the type
      * @return size in bytes
      *
      * 
      *
      */
    uint32_t getTypeSize() const;

private:
    type _type;

};

}

#endif // TYPEINFO_H
