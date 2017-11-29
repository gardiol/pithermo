#include "typeinfo.h"

using namespace FrameworkLibrary;

std::set<TypeInfo::type> TypeInfo::listEnumType()
{
    std::set<type> ret;
    ret.insert( int8_field );
    ret.insert( uint8_field );
    ret.insert( int16_field );
    ret.insert( uint16_field );
    ret.insert( int32_field );
    ret.insert( uint32_field );
    ret.insert( int64_field );
    ret.insert( uint64_field );
    ret.insert( float_field );
    ret.insert( double_field );
    ret.insert( bool_field );
    ret.insert( char_field );
    ret.insert( string_field );
    ret.insert( raw_field );
    ret.insert( no_type_field );
    ret.insert( unknown_type );
    return ret;
}

std::list<TypeInfo> TypeInfo::listTypes()
{
    std::list<TypeInfo> ret;
    std::set<TypeInfo::type> types = listEnumType();
    for ( std::set<TypeInfo::type>::iterator i = types.begin(); i != types.end(); ++i )
        ret.push_back( TypeInfo(*i) );
    return ret;
}

std::list<std::string> TypeInfo::listTypesStr()
{
    std::list<std::string> ret;
    std::set<TypeInfo::type> types = listEnumType();
    for ( std::set<TypeInfo::type>::iterator i = types.begin(); i != types.end(); ++i )
        ret.push_back( TypeInfo(*i).getTypeStr() );
    return ret;
}

TypeInfo::TypeInfo(uint8_t t)
{
    switch ( t )
    {
    case 0:
        _type = int8_field;
        break;
    case 1:
        _type = int16_field;
        break;
    case 2:
        _type = int32_field;
        break;
    case 3:
        _type = int64_field;
        break;
    case 4:
        _type = uint8_field;
        break;
    case 5:
        _type = uint16_field;
        break;
    case 6:
        _type = uint32_field;
        break;
    case 7:
        _type = uint64_field;
        break;
    case 8:
        _type = float_field;
        break;
    case 9:
        _type = double_field;
        break;
    case 11:
        _type = bool_field;
        break;
    case 12:
        _type = char_field;
        break;
    case 0xFC:
        _type = no_type_field;
        break;
    case 0xFE:
        _type = string_field;
        break;
    case 0xFD:
        _type = raw_field;
        break;
    default:
        _type = unknown_type;
    }
}

TypeInfo::TypeInfo(type t)
{
    _type = t;
}

TypeInfo::TypeInfo(const std::string &tmp)
{
    std::string str = tmp.substr(0, tmp.find_first_of('_') );
    if ( str == "int8" )
        _type = int8_field;
    else if ( str == "uint8" )
        _type = uint8_field;
    else if ( str == "int16" )
        _type = int16_field;
    else if ( str == "uint16" )
        _type = uint16_field;
    else if ( str == "int32" )
        _type = int32_field;
    else if ( str == "uint32" )
        _type = uint32_field;
    else if ( str == "int64" )
        _type = int64_field;
    else if ( str == "uint64" )
        _type = uint64_field;
    else if ( str == "float" )
        _type = float_field;
    else if ( str == "double" )
        _type = double_field;
    else if ( str == "bool" )
        _type = bool_field;
    else if ( str == "char" )
        _type = char_field;
    else if ( str == "string" )
        _type = string_field;
    else if ( str == "" )
        _type = no_type_field;
    else if ( str == "raw" )
        _type = raw_field;
    else
        _type = unknown_type;
}

TypeInfo::TypeInfo(const TypeInfo &other)
{
    _type = other._type;
}

TypeInfo::type TypeInfo::getType() const
{
    return _type;
}

bool TypeInfo::operator==(const TypeInfo &other) const
{
    return _type == other._type;
}

bool TypeInfo::operator==(uint8_t t) const
{
    return (*this) == TypeInfo(t);
}

bool TypeInfo::operator==(TypeInfo::type t) const
{
    return (*this) == TypeInfo(t);
}

bool TypeInfo::operator==(const std::string &s) const
{
    return (*this) == TypeInfo(s);
}

bool TypeInfo::isUnknown() const
{
    return _type == unknown_type;
}

TypeInfo::operator uint8_t() const
{
    return (uint8_t)_type;
}

uint32_t TypeInfo::getTypeSize() const
{
    switch ( _type )
    {
    case int8_field:
    case char_field:
    case uint8_field:
        return 1;
    case int16_field:
    case uint16_field:
        return 2;
    case int32_field:
    case uint32_field:
    case float_field:
    case bool_field:
        return 4;
    case int64_field:
    case uint64_field:
    case double_field:
        return 8;
    case unknown_type:
    case string_field:
    case raw_field:
    case no_type_field:
    default:
        return 0;
    }
}

std::string TypeInfo::getTypeStr() const
{
    switch ( _type )
    {
    case int8_field:
        return "int8";
    case int16_field:
        return "int16";
    case int32_field:
        return "int32";
    case int64_field:
        return "int64";
    case uint8_field:
        return "uint8";
    case uint16_field:
        return "uint16";
    case uint32_field:
        return "uint32";
    case uint64_field:
        return "uint64";
    case float_field:
        return "float";
    case bool_field:
        return "bool";
    case double_field:
        return "double";
    case char_field:
        return "char";
    case string_field:
        return "string";
    case no_type_field:
        return "no_type";
    case raw_field:
        return "raw";
    case unknown_type:
    default:
        return "unknown";
    }
}
