#include "variant.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <frameworkutils.h>
#include <memorychecker.h>
#include <sstream>

namespace FrameworkLibrary {

static const uint16_t VariantSeralizedWatermark = 0xA5A5;

union __privateAliaser
{
    uint64_t src;
    int64_t dst_64i;
    int32_t dst_32i;
    int16_t dst_16i;
    int8_t dst_8i;
    uint64_t dst_u64i;
    uint32_t dst_u32i;
    uint16_t dst_u16i;
    uint8_t dst_u8i;
    float dst_float;
    double dst_double;
};

}

#if defined (_MSC_VER)
#ifndef atoll
#define atoll _atoi64
#endif
#ifndef strtoull
#define strtoull _strtoui64
#endif
#ifndef strtoll
#define strtoll _strtoi64
#endif
#endif

using namespace FrameworkLibrary;

Variant::Variant( const Variant& other )
{
    _type = other._type;
    _type_size = other._type_size;
    _array_size = other._array_size;
}

Variant::Variant( const TypeInfo& t, uint32_t array_size )
{
    // Ensure type is valid:
    switch ( t )
    {
    case TypeInfo::string_field:
        _type = TypeInfo( TypeInfo::char_field );
        break;
    case TypeInfo::raw_field:
        _type = TypeInfo( TypeInfo::uint8_field );
        break;
    default:
        _type = t;
    }
    _type_size = _type.getTypeSize();
    _array_size = array_size;
}

Variant::~Variant()
{
}

std::string Variant::serialize() const
{
    // The format of the serialized string is:
    // 2 bytes: version watermark (fixed to 0xA5)
    // 1 byte: type (see TypeInfo class)
    // 1 byte: size in bytes of the type (S, as returned by TypeInfo)
    // 4 bytes: array count (N)
    // NxS bytes: the data    uint8_t* buffer = new uint8_t[ 1 + 4 + 8*_array_size ];
    uint8_t* buffer = new uint8_t[ sizeof(VariantSeralizedWatermark) + 1 + 1 + sizeof(_array_size) + _type_size * _array_size ];
    memcpy( &buffer[ 0 ], &VariantSeralizedWatermark, sizeof(VariantSeralizedWatermark) );

    buffer[ sizeof(VariantSeralizedWatermark) ] = (uint8_t)_type;
    buffer[ sizeof(VariantSeralizedWatermark) + 1 ] = (uint8_t)_type_size;

    memcpy( &buffer[ sizeof(VariantSeralizedWatermark) + 1 + 1 ], &_array_size, sizeof(_array_size) );
    uint64_t tmp;
    for ( uint32_t n = 0; n < _array_size; n++ )
    {
        tmp = 0;
        getDataAt( n, &tmp );
        memcpy( &buffer[ sizeof(VariantSeralizedWatermark) + 1 + 1 + sizeof(_array_size) + n * _type_size ], &tmp, _type_size );
    }
    std::string blob( (char*)buffer, sizeof(VariantSeralizedWatermark) + 1 + 1 + sizeof(_array_size) + _array_size * _type_size );
    delete [] buffer;
    return blob;
}

uint8_t *Variant::unserialize(const std::string &blob, TypeInfo &type, uint32_t &array_size)
{
    uint8_t* ret = NULL;
    const uint8_t* ptr = (const uint8_t*)blob.c_str();
    uint16_t watermark = 0;
    memcpy( &watermark, &ptr[ 0 ], sizeof( VariantSeralizedWatermark ) );
    if ( watermark == VariantSeralizedWatermark )
    {
        type = TypeInfo( ptr[ sizeof(VariantSeralizedWatermark) ] );
        int8_t type_size = ptr[ sizeof(VariantSeralizedWatermark) + 1 ];
        if ( (uint32_t)type_size == type.getTypeSize() )
        {
            memcpy( &array_size, &ptr[ sizeof( VariantSeralizedWatermark) + 1 + 1 ], sizeof(array_size) );
            // Since it's caller resposability to deallocate this buffer, here we do not use the MemoryChecker:
            ret = new uint8_t[ type_size * array_size ];
            for ( uint32_t n = 0; n < array_size; n++ )
                memcpy( &ret[ n * type_size ], &ptr[ sizeof(VariantSeralizedWatermark) + 1 + 1 + sizeof(array_size) + n * type_size], type_size );
        }
    }
    return ret;
}

uint32_t Variant::getArraySize() const
{
    return _array_size;
}

TypeInfo Variant::getType() const
{
    return _type;
}

bool Variant::isTypeIdentical( const Variant& other ) const
{
    return _type == other._type;
}

bool Variant::isEqualTo(const Variant &other, int32_t array_pos) const
{
    if ( array_pos == -1 ) // test all
    {
        bool equal = false;
        if ( _array_size != other._array_size )
            return false;
        for ( uint32_t pos = 0; pos < _array_size; pos++ )
        {
            switch ( _type )
            {
            case TypeInfo::char_field:
            case TypeInfo::int8_field:
                equal = getAsInt8( pos ) == other.convertToInt8( pos );
                break;
            case TypeInfo::int16_field:
                equal = getAsInt16( pos ) == other.convertToInt16( pos );
                break;
            case TypeInfo::int32_field:
                equal = getAsInt32( pos ) == other.convertToInt32( pos );
                break;
            case TypeInfo::int64_field:
                equal = getAsInt64( pos ) == other.convertToInt64( pos );
                break;
            case TypeInfo::uint8_field:
                equal = getAsUint8( pos ) == other.convertToUint8( pos );
                break;
            case TypeInfo::uint16_field:
                equal = getAsUint16( pos ) == other.convertToUint16( pos );
                break;
            case TypeInfo::bool_field:
            case TypeInfo::uint32_field:
                equal = getAsUint32( pos ) == other.convertToUint32( pos );
                break;
            case TypeInfo::uint64_field:
                equal = getAsUint64( pos ) == other.convertToUint64( pos );
                break;
            case TypeInfo::float_field:
                equal = getAsFloat( pos ) == other.convertToFloat( pos );
                break;
            case TypeInfo::double_field:
                equal = getAsDouble( pos ) == other.convertToDouble( pos );
                break;
            default:
                break;
            }
            if ( !equal )
                return false;
        }
        return true;
    }
    else if ( (array_pos > -1 ) &&
              ((uint32_t)array_pos < _array_size) &&
              ((uint32_t)array_pos < other._array_size ) )// test one
    {
        switch ( _type )
        {
        case TypeInfo::char_field:
        case TypeInfo::int8_field:
            return getAsInt8( array_pos ) == other.convertToInt8( array_pos );
        case TypeInfo::int16_field:
            return getAsInt16( array_pos ) == other.convertToInt16( array_pos );
        case TypeInfo::int32_field:
            return getAsInt32( array_pos ) == other.convertToInt32( array_pos );
        case TypeInfo::int64_field:
            return getAsInt64( array_pos ) == other.convertToInt64( array_pos );
        case TypeInfo::uint8_field:
            return getAsUint8( array_pos ) == other.convertToUint8( array_pos );
        case TypeInfo::uint16_field:
            return getAsUint16( array_pos ) == other.convertToUint16( array_pos );
        case TypeInfo::bool_field:
        case TypeInfo::uint32_field:
            return getAsUint32( array_pos ) == other.convertToUint32( array_pos );
        case TypeInfo::uint64_field:
            return getAsUint64( array_pos ) == other.convertToUint64( array_pos );
        case TypeInfo::float_field:
            return getAsFloat( array_pos ) == other.convertToFloat( array_pos );
        case TypeInfo::double_field:
            return getAsDouble( array_pos ) == other.convertToDouble( array_pos );
        default:
            return false;
        }
    }
    return false;
}

bool Variant::isBinaryIdenticalTo( const Variant& other, int32_t array_pos ) const
{
    if ( array_pos == -1 ) // test all
    {
        uint64_t src = 0;
        uint64_t dest = 0;
        if ( _array_size != other._array_size )
            return false;
        for ( uint32_t pos = 0; pos < _array_size; pos++ )
        {
            getDataAt( pos, &src );
            other.getDataAt( pos, &dest );
            if ( src != dest )
                return false;
        }
        return true;
    }
    else if ( (array_pos > -1 ) &&
              ((uint32_t)array_pos < _array_size) &&
              ((uint32_t)array_pos < other._array_size ) )// test one
    {
        uint64_t src = 0;
        uint64_t dest = 0;
        getDataAt( array_pos, &src );
        other.getDataAt( array_pos, &dest );
        return src == dest;
    }
    return false;
}

bool Variant::isIdenticalTo(const Variant &other, int32_t array_pos) const
{
    if ( isTypeIdentical( other ) )
        return isBinaryIdenticalTo( other, array_pos );
    return false;
}

void Variant::fromHumanString(const std::string &string)
{
    std::vector<std::string> values = FrameworkUtils::string_split( string, "," );
    uint32_t string_max_pos = values.size();

    for ( uint32_t pos = 0; pos < _array_size; pos++ )
    {
        uint64_t tmp_data = 0;
        char * tmp = (char*)&tmp_data;
        if ( pos < string_max_pos )
        {
            std::string value = values[ pos ];
            switch ( _type )
            {
            case TypeInfo::int8_field:
            {
                int8_t data = atoi( value.c_str() );
                memcpy( tmp, &data, sizeof(data) );
                break;
            }

            case TypeInfo::int16_field:
            {
                int16_t data = atoi( value.c_str() );
                memcpy( tmp, &data, sizeof(data) );
                break;
            }

            case TypeInfo::int32_field:
            {
                int32_t data = atoi( value.c_str() );
                memcpy( tmp, &data, sizeof(data) );
                break;
            }

            case TypeInfo::int64_field:
            {
                int64_t data = atoll( value.c_str() );
                memcpy( tmp, &data, sizeof(data) );
                break;
            }

            case TypeInfo::uint8_field:
            {
                uint8_t data = atoi( value.c_str() );
                memcpy( tmp, &data, sizeof(data) );
                break;
            }

            case TypeInfo::uint16_field:
            {
                uint16_t data = atoi( value.c_str() );
                memcpy( tmp, &data, sizeof(data) );
                break;
            }

            case TypeInfo::uint32_field:
            {
                uint32_t data = atoi( value.c_str() );
                memcpy( tmp, &data, sizeof(data) );
                break;
            }

            case TypeInfo::uint64_field:
            {

                uint64_t data = strtoull( value.c_str(), NULL, 0 );
                memcpy( tmp, &data, sizeof(data) );
                break;
            }

            case TypeInfo::float_field:
            {
                float data = (float)atof( value.c_str() );
                memcpy( tmp, &data, sizeof(data) );
                break;
            }

            case TypeInfo::double_field:
            {
                double data = atof( value.c_str() );
                memcpy( tmp, &data, sizeof(data) );
                break;
            }

            case TypeInfo::char_field:
            {
                char data = 0;
                // First format: a, just put the letter "a" in the char.
                if ( value.length() == 1 )
                    data = value.at(0);
                else if ( value.length() > 1 )
                {
                    std::string stripped = value;
                    FrameworkUtils::string_trim( stripped );
                    // Remove ' or " around the string:
                    char first = stripped.at(0);
                    char last = stripped.at( stripped.length()-1 );
                    if ( ( (first == '"') && (last == '"') ) ||
                         ( (first == '\'') && (last == '\'') ) )
                        stripped = stripped.substr(1, stripped.length()-2);
                    // Second format: 0x01, put the exadecimal 01 into the char
                    if ( stripped.substr( 0, 2 ) == "0x" )
                    {
                        unsigned int raw = 0;
                        sscanf( stripped.c_str(), "0x%x", &raw );
                        data = (uint8_t)raw;
                    }
                    else
                        // last format: abcd, copy all the letters up to the array_size:
                    {
                        std::string extra = "";
                        std::vector<std::string>::iterator itx = values.begin();
                        ++itx;
                        for (; itx != values.end(); ++itx )
                            extra += "," + (*itx);
                        values.clear();
                        stripped += extra;
                        for ( uint32_t c = 0; c < stripped.length(); ++c )
                            values.push_back( std::string(&stripped[c],1) );
                        string_max_pos = values.size();
                        data = values[0].at(0);
                    }
                }
                memcpy( tmp, &data, sizeof(data) );
                break;
            }

            case TypeInfo::bool_field:
            {
                uint32_t data = 1;
                if ( (value == "0") || (FrameworkUtils::string_tolower(value) == "false") )
                    data = 0;
                memcpy( tmp, &data, sizeof(data) );
                break;
            }

            default:
                tmp = 0;
            }
        }
        setDataAt( pos, tmp );
    }
}

std::string Variant::toHumanString(int32_t pos_from, int32_t pos_to) const
{
    std::string sep;
    std::string ret = "";
    uint32_t min = 0;
    uint32_t max = 0;
    if ( pos_from < 0 )
    {   // entire array
        min = 0;
        max = _array_size;
    }
    else // from a point
    {
        min = FrameworkUtils_min<uint32_t>( pos_from, _array_size-1 );
        if ( ( pos_to < 0 ) ||
             ( pos_to <= (int32_t)min ) )
            max = FrameworkUtils_min<uint32_t>( min+1, _array_size );
        else
            max = FrameworkUtils_min<uint32_t>( pos_to, _array_size );
    }

    for ( uint32_t n = min; n < max; n++ )
    {
        sep = ((max - n) > 1) ? "," : sep = "";
        __privateAliaser tmp;
        tmp.src = 0;
        getDataAt( n, &tmp.src );
        char stmp[1024];
        memset( stmp, 0, 1024 );
        switch ( _type )
        {
        case TypeInfo::int8_field:
            sprintf(stmp, "%d", tmp.dst_8i );
            break;
        case TypeInfo::int16_field:
            sprintf(stmp, "%d", tmp.dst_16i );
            break;
        case TypeInfo::int32_field:
            sprintf(stmp, "%d", tmp.dst_32i );
            break;
        case TypeInfo::int64_field:
            sprintf(stmp, "%lld", (long long int) tmp.dst_64i );
            break;
        case TypeInfo::uint8_field:
            sprintf(stmp, "%u", tmp.dst_u8i );
            break;
        case TypeInfo::uint16_field:
            sprintf(stmp, "%u", tmp.dst_u16i );
            break;
        case TypeInfo::uint32_field:
            sprintf(stmp, "%u", tmp.dst_u32i );
            break;
        case TypeInfo::uint64_field:
            sprintf(stmp, "%llu", (unsigned long long int) tmp.dst_u64i );
            break;
        case TypeInfo::float_field:
            sprintf(stmp, "%f", tmp.dst_float );
            break;
        case TypeInfo::double_field:
            sprintf(stmp, "%lf", tmp.dst_double );
            break;
        case TypeInfo::char_field:
            sep = "";
            stmp[0] = tmp.dst_8i;
            break;
        case TypeInfo::bool_field:
            sprintf(stmp, "%s", tmp.src == 0 ? "false" : "true");
            break;

        default:
            sprintf(stmp, "0x%llX", (long long int)tmp.dst_64i );
        }
        ret += std::string(stmp) + sep;
    }
    return ret;
}

float Variant::getAsFloat(uint32_t array_pos) const
{
    float ret = 0.0;
    uint64_t tmp = 0;
    getDataAt( array_pos, &tmp );
    memcpy( &ret, &tmp, sizeof(ret) );
    return ret;
}

std::string Variant::getAsString(uint32_t start_pos, uint32_t len) const
{
    std::string ret;
    if ( (_array_size > 0) )
    {
        if ( len == 0 )
            len = _array_size - start_pos;
        if ( start_pos >= _array_size )
        {
            start_pos = _array_size-1;
            len = 1;
        }
        else if ( ( start_pos + len ) > _array_size )
            len = _array_size - start_pos - 1;

        char c[2];
        c[0] = '\0';
        c[1] = '\0';
        for ( uint32_t i = 0; i < len; i++ )
        {
            c[0] = getAsChar( start_pos + i );
            ret += c;
            if ( c[0] == '\0' )
                i = len;
        }
    }
    return ret;
}

double Variant::getAsDouble( uint32_t array_pos ) const
{
    double ret = 0.0;
    uint64_t tmp = 0;
    getDataAt( array_pos, &tmp );
    memcpy( &ret, &tmp, sizeof(ret) );
    return ret;
}

int8_t Variant::getAsInt8(uint32_t array_pos) const
{
    int8_t ret = 0;
    uint64_t tmp = 0;
    getDataAt( array_pos, &tmp );
    memcpy( &ret, &tmp, sizeof(ret) );
    return ret;
}

uint8_t Variant::getAsUint8(uint32_t array_pos) const
{
    uint8_t ret = 0;
    uint64_t tmp = 0;
    getDataAt( array_pos, &tmp );
    memcpy( &ret, &tmp, sizeof(ret) );
    return ret;
}

int16_t Variant::getAsInt16(uint32_t array_pos) const
{
    int16_t ret = 0;
    uint64_t tmp = 0;
    getDataAt( array_pos, &tmp );
    memcpy( &ret, &tmp, sizeof(ret) );
    return ret;
}

uint16_t Variant::getAsUint16(uint32_t array_pos) const
{
    uint16_t ret = 0;
    uint64_t tmp = 0;
    getDataAt( array_pos, &tmp );
    memcpy( &ret, &tmp, sizeof(ret) );
    return ret;
}

int32_t Variant::getAsInt32(uint32_t array_pos) const
{
    int32_t ret = 0;
    uint64_t tmp = 0;
    getDataAt( array_pos, &tmp );
    memcpy( &ret, &tmp, sizeof(ret) );
    return ret;
}

float Variant::convertToFloat(uint32_t array_pos) const
{
    float ret = 0.0;
    switch ( _type )
    {
    case TypeInfo::char_field:
        ret = (float)getAsInt8( array_pos );
        break;
    case TypeInfo::int8_field:
        ret = (float)getAsInt8( array_pos );
        break;
    case TypeInfo::int16_field:
        ret = (float)getAsInt16( array_pos );
        break;
    case TypeInfo::int32_field:
        ret = (float)getAsInt32( array_pos );
        break;
    case TypeInfo::int64_field:
        ret = (float)getAsInt64( array_pos );
        break;

    case TypeInfo::bool_field:
        ret = (float)getAsUint32( array_pos );
        break;

    case TypeInfo::uint8_field:
        ret = (float)getAsUint8( array_pos );
        break;

    case TypeInfo::uint16_field:
        ret = (float)getAsUint16( array_pos );
        break;

    case TypeInfo::uint32_field:
        ret = (float)getAsUint32( array_pos );
        break;

    case TypeInfo::uint64_field:
        ret = (float)getAsUint64( array_pos );
        break;

    case TypeInfo::double_field:
        ret = (float)getAsDouble( array_pos );
        break;

    default:
        ret = getAsFloat( array_pos );
    }
    return ret;
}

double Variant::convertToDouble(uint32_t array_pos) const
{
    double ret = 0.0;
    switch ( _type )
    {
    case TypeInfo::char_field:
        ret = (double)getAsInt8( array_pos );
        break;
    case TypeInfo::int8_field:
        ret = (double)getAsInt8( array_pos );
        break;
    case TypeInfo::int16_field:
        ret = (double)getAsInt16( array_pos );
        break;
    case TypeInfo::int32_field:
        ret = (double)getAsInt32( array_pos );
        break;
    case TypeInfo::int64_field:
        ret = (double)getAsInt64( array_pos );
        break;

    case TypeInfo::bool_field:
        ret = (double)getAsUint32( array_pos );
        break;

    case TypeInfo::uint8_field:
        ret = (double)getAsUint8( array_pos );
        break;

    case TypeInfo::uint16_field:
        ret = (double)getAsUint16( array_pos );
        break;

    case TypeInfo::uint32_field:
        ret = (double)getAsUint32( array_pos );
        break;

    case TypeInfo::uint64_field:
        ret = (double)getAsUint64( array_pos );
        break;

    case TypeInfo::float_field:
        ret = (double)getAsFloat( array_pos );
        break;

    default:
        ret = getAsDouble( array_pos );
    }
    return ret;
}

int8_t Variant::convertToInt8(uint32_t array_pos) const
{
    int8_t ret = 0;
    switch ( _type )
    {
    case TypeInfo::char_field:
        ret = (int8_t)getAsInt8( array_pos );
        break;
    case TypeInfo::int16_field:
        ret = (int8_t)getAsInt16( array_pos );
        break;
    case TypeInfo::int32_field:
        ret = (int8_t)getAsInt32( array_pos );
        break;
    case TypeInfo::int64_field:
        ret = (int8_t)getAsInt64( array_pos );
        break;

    case TypeInfo::bool_field:
        ret = (int8_t)getAsUint32( array_pos );
        break;

    case TypeInfo::uint8_field:
        ret = (int8_t)getAsUint8( array_pos );
        break;

    case TypeInfo::uint16_field:
        ret = (int8_t)getAsUint16( array_pos );
        break;

    case TypeInfo::uint32_field:
        ret = (int8_t)getAsUint32( array_pos );
        break;

    case TypeInfo::uint64_field:
        ret = (int8_t)getAsUint64( array_pos );
        break;

    case TypeInfo::double_field:
        ret = (int8_t)getAsDouble( array_pos );
        break;

    case TypeInfo::float_field:
        ret = (int8_t)getAsFloat( array_pos );
        break;

    default:
        ret = getAsInt8( array_pos );
    }
    return ret;
}

int16_t Variant::convertToInt16(uint32_t array_pos) const
{
    int16_t ret = 0;
    switch ( _type )
    {
    case TypeInfo::char_field:
        ret = (int16_t)getAsInt8( array_pos );
        break;
    case TypeInfo::int8_field:
        ret = (int16_t)getAsInt8( array_pos );
        break;
    case TypeInfo::int32_field:
        ret = (int16_t)getAsInt32( array_pos );
        break;
    case TypeInfo::int64_field:
        ret = (int16_t)getAsInt64( array_pos );
        break;

    case TypeInfo::bool_field:
        ret = (int16_t)getAsUint32( array_pos );
        break;

    case TypeInfo::uint8_field:
        ret = (int16_t)getAsUint8( array_pos );
        break;

    case TypeInfo::uint16_field:
        ret = (int16_t)getAsUint16( array_pos );
        break;

    case TypeInfo::uint32_field:
        ret = (int16_t)getAsUint32( array_pos );
        break;

    case TypeInfo::uint64_field:
        ret = (int16_t)getAsUint64( array_pos );
        break;

    case TypeInfo::double_field:
        ret = (int16_t)getAsDouble( array_pos );
        break;

    case TypeInfo::float_field:
        ret = (int16_t)getAsFloat( array_pos );
        break;

    default:
        ret = getAsInt16( array_pos );
    }
    return ret;
}

int32_t Variant::convertToInt32(uint32_t array_pos) const
{
    int32_t ret = 0;
    switch ( _type )
    {
    case TypeInfo::char_field:
        ret = (int32_t)getAsInt8( array_pos );
        break;
    case TypeInfo::int8_field:
        ret = (int32_t)getAsInt8( array_pos );
        break;
    case TypeInfo::int16_field:
        ret = (int32_t)getAsInt16( array_pos );
        break;
    case TypeInfo::int64_field:
        ret = (int32_t)getAsInt64( array_pos );
        break;

    case TypeInfo::bool_field:
        ret = (int32_t)getAsUint32( array_pos );
        break;

    case TypeInfo::uint8_field:
        ret = (int32_t)getAsUint8( array_pos );
        break;

    case TypeInfo::uint16_field:
        ret = (int32_t)getAsUint16( array_pos );
        break;

    case TypeInfo::uint32_field:
        ret = (int32_t)getAsUint32( array_pos );
        break;

    case TypeInfo::uint64_field:
        ret = (int32_t)getAsUint64( array_pos );
        break;

    case TypeInfo::double_field:
        ret = (int32_t)getAsDouble( array_pos );
        break;

    case TypeInfo::float_field:
        ret = (int32_t)getAsFloat( array_pos );
        break;

    default:
        ret = getAsInt32( array_pos );
    }
    return ret;
}

int64_t Variant::convertToInt64(uint32_t array_pos) const
{
    int64_t ret = 0;
    switch ( _type )
    {
    case TypeInfo::char_field:
        ret = (int64_t)getAsInt8( array_pos );
        break;
    case TypeInfo::int8_field:
        ret = (int64_t)getAsInt8( array_pos );
        break;
    case TypeInfo::int16_field:
        ret = (int64_t)getAsInt16( array_pos );
        break;
    case TypeInfo::int32_field:
        ret = (int64_t)getAsInt32( array_pos );
        break;

    case TypeInfo::bool_field:
        ret = (int64_t)getAsUint32( array_pos );
        break;

    case TypeInfo::uint8_field:
        ret = (int64_t)getAsUint8( array_pos );
        break;

    case TypeInfo::uint16_field:
        ret = (int64_t)getAsUint16( array_pos );
        break;

    case TypeInfo::uint32_field:
        ret = (int64_t)getAsUint32( array_pos );
        break;

    case TypeInfo::uint64_field:
        ret = (int64_t)getAsUint64( array_pos );
        break;

    case TypeInfo::double_field:
        ret = (int64_t)getAsDouble( array_pos );
        break;

    case TypeInfo::float_field:
        ret = (int64_t)getAsFloat( array_pos );
        break;

    default:
        ret = getAsInt64( array_pos );
    }
    return ret;
}

uint8_t Variant::convertToUint8(uint32_t array_pos) const
{
    uint8_t ret = 0;
    switch ( _type )
    {
    case TypeInfo::char_field:
    case TypeInfo::int8_field:
    case TypeInfo::int16_field:
    case TypeInfo::int32_field:
    case TypeInfo::int64_field:
        ret = (uint8_t)getAsInt64( array_pos );
        break;

    case TypeInfo::bool_field:
        ret = (uint8_t)getAsUint32( array_pos );
        break;

    case TypeInfo::uint16_field:
        ret = (uint8_t)getAsUint16( array_pos );
        break;

    case TypeInfo::uint32_field:
        ret = (uint8_t)getAsUint32( array_pos );
        break;

    case TypeInfo::uint64_field:
        ret = (uint8_t)getAsUint64( array_pos );
        break;

    case TypeInfo::double_field:
        ret = (uint8_t)getAsDouble( array_pos );
        break;

    case TypeInfo::float_field:
        ret = (uint8_t)getAsFloat( array_pos );
        break;

    default:
        ret = getAsUint8( array_pos );
    }
    return ret;
}

uint16_t Variant::convertToUint16(uint32_t array_pos) const
{
    uint16_t ret = 0;
    switch ( _type )
    {
    case TypeInfo::char_field:
    case TypeInfo::int8_field:
    case TypeInfo::int16_field:
    case TypeInfo::int32_field:
    case TypeInfo::int64_field:
        ret = (uint16_t)getAsInt64( array_pos );
        break;

    case TypeInfo::bool_field:
        ret = (uint16_t)getAsUint32( array_pos );
        break;

    case TypeInfo::uint8_field:
        ret = (uint16_t)getAsUint8( array_pos );
        break;

    case TypeInfo::uint32_field:
        ret = (uint16_t)getAsUint32( array_pos );
        break;

    case TypeInfo::uint64_field:
        ret = (uint16_t)getAsUint64( array_pos );
        break;

    case TypeInfo::double_field:
        ret = (uint16_t)getAsDouble( array_pos );
        break;

    case TypeInfo::float_field:
        ret = (uint16_t)getAsFloat( array_pos );
        break;

    default:
        ret = getAsUint16( array_pos );
    }
    return ret;
}


uint32_t Variant::convertToUint32(uint32_t array_pos) const
{
    uint32_t ret = 0;
    switch ( _type )
    {
    case TypeInfo::char_field:
    case TypeInfo::int8_field:
    case TypeInfo::int16_field:
    case TypeInfo::int32_field:
    case TypeInfo::int64_field:
        ret = (uint32_t)getAsInt64( array_pos );
        break;

    case TypeInfo::bool_field:
        ret = (uint32_t)getAsUint32( array_pos );
        break;

    case TypeInfo::uint8_field:
        ret = (uint32_t)getAsUint8( array_pos );
        break;

    case TypeInfo::uint16_field:
        ret = (uint32_t)getAsUint16( array_pos );
        break;

    case TypeInfo::uint64_field:
        ret = (uint32_t)getAsUint64( array_pos );
        break;

    case TypeInfo::double_field:
        ret = (uint32_t)getAsDouble( array_pos );
        break;

    case TypeInfo::float_field:
        ret = (uint32_t)getAsFloat( array_pos );
        break;

    default:
        ret = getAsUint32( array_pos );
    }
    return ret;
}

uint64_t Variant::convertToUint64(uint32_t array_pos) const
{
    uint64_t ret = 0;
    switch ( _type )
    {
    case TypeInfo::char_field:
    case TypeInfo::int8_field:
    case TypeInfo::int16_field:
    case TypeInfo::int32_field:
    case TypeInfo::int64_field:
        ret = (uint64_t)getAsInt64( array_pos );
        break;

    case TypeInfo::bool_field:
        ret = (uint64_t)getAsUint32( array_pos );
        break;

    case TypeInfo::uint8_field:
        ret = (uint64_t)getAsUint8( array_pos );
        break;

    case TypeInfo::uint16_field:
        ret = (uint64_t)getAsUint16( array_pos );
        break;

    case TypeInfo::uint32_field:
        ret = (uint64_t)getAsUint32( array_pos );
        break;

    case TypeInfo::double_field:
        ret = (uint64_t)getAsDouble( array_pos );
        break;

    case TypeInfo::float_field:
        ret = (uint64_t)getAsFloat( array_pos );
        break;

    default:
        ret = getAsUint64( array_pos );
    }
    return ret;
}

char Variant::convertToChar(uint32_t array_pos) const
{
    char ret = 0;
    switch ( _type )
    {
    case TypeInfo::int8_field:
    case TypeInfo::int16_field:
    case TypeInfo::int32_field:
    case TypeInfo::int64_field:
        ret = (char)getAsInt64( array_pos );
        break;

    case TypeInfo::bool_field:
        ret = getAsUint32( array_pos ) ? 't' : 'f';
        break;

    case TypeInfo::uint8_field:
        ret = (char)getAsUint8( array_pos );
        break;

    case TypeInfo::uint16_field:
        ret = (char)getAsUint16( array_pos );
        break;

    case TypeInfo::uint32_field:
        ret = (char)getAsUint32( array_pos );
        break;

    case TypeInfo::double_field:
        ret = (uint64_t)getAsDouble( array_pos );
        break;

    case TypeInfo::float_field:
        ret = (char)getAsFloat( array_pos );
        break;

    default:
        ret = getAsChar( array_pos );
    }
    return ret;
}

bool Variant::convertToBool(uint32_t array_pos) const
{
    bool ret = 0;
    switch ( _type )
    {
    case TypeInfo::char_field:
    {
        char c = getAsChar( array_pos );
        ret = ( c != '0' ) && ( c != 'f' );
        break;
    }

    case TypeInfo::int8_field:
        ret = getAsInt8( array_pos ) != 0;
        break;

    case TypeInfo::int16_field:
        ret = getAsInt16( array_pos ) != 0;
        break;

    case TypeInfo::int32_field:
        ret = getAsInt32( array_pos ) != 0;
        break;

    case TypeInfo::int64_field:
        ret = getAsInt64( array_pos ) != 0;
        break;

    case TypeInfo::uint8_field:
        ret = getAsUint8( array_pos ) != 0;
        break;

    case TypeInfo::uint16_field:
        ret = getAsUint16( array_pos ) != 0;
        break;

    case TypeInfo::uint32_field:
        ret = getAsUint32( array_pos ) != 0;
        break;

    case TypeInfo::double_field:
        ret = (uint32_t)getAsDouble( array_pos ) != 0;
        break;

    case TypeInfo::float_field:
        ret = (uint32_t)getAsFloat( array_pos ) != 0;
        break;

    default:
        ret = getAsBool( array_pos );
    }
    return ret;
}

uint32_t Variant::getAsUint32(uint32_t array_pos) const
{
    uint32_t ret = 0;
    uint64_t tmp = 0;
    getDataAt( array_pos, &tmp );
    memcpy( &ret, &tmp, sizeof(ret) );
    return ret;
}

int64_t Variant::getAsInt64(uint32_t array_pos) const
{
    int64_t ret = 0;
    uint64_t tmp = 0;
    getDataAt( array_pos, &tmp );
    memcpy( &ret, &tmp, sizeof(ret) );
    return ret;
}

uint64_t Variant::getAsUint64(uint32_t array_pos) const
{
    uint64_t ret = 0;
    uint64_t tmp = 0;
    getDataAt( array_pos, &tmp );
    memcpy( &ret, &tmp, sizeof(ret) );
    return ret;
}

char Variant::getAsChar(uint32_t array_pos) const
{
    char ret = 0;
    uint64_t tmp = 0;
    getDataAt( array_pos, &tmp );
    memcpy( &ret, &tmp, sizeof(ret) );
    return ret;
}

bool Variant::getAsBool(uint32_t array_pos) const
{
    return getAsUint32( array_pos ) != 0;
}

std::string Variant::convertToString(uint32_t start_pos, uint32_t len) const
{
    std::string ret;
    if ( _array_size > 0 )
    {
        if ( len == 0 )
            len = _array_size;

        if ( start_pos >= _array_size )
        {
            start_pos = _array_size-1;
            len = 1;
        }
        else if ( ( start_pos + len ) > _array_size )
            len = _array_size - start_pos - 1;

        char c[2];
        c[0] = '\0';
        c[1] = '\0';
        for ( uint32_t i = 0; i < len; i++ )
        {
            c[0] = convertToChar( start_pos + i );
            if (  c[0] == '\0' )
                i = len;
            else
                ret += c;
        }
    }
    return ret;
}

void Variant::setRaw(const void *v, uint32_t array_pos)
{
    setDataAt( array_pos, v );
}

void Variant::setRawArray(const void *v, uint32_t start_pos, uint32_t end_pos)
{
    if ( end_pos >= start_pos )
    {
        if ( (start_pos < _array_size) && (end_pos < _array_size) )
        {
            const char* ptr = (const char*)v;
            for ( uint32_t pos = start_pos; pos <= end_pos; pos++ )
                setRaw( &ptr[ _type_size * (pos-start_pos) ], pos );
        }
    }
}

void Variant::getRaw(void *v, uint32_t array_pos) const
{
    getDataAt( array_pos, v );
}

void Variant::getRawArray(void *v, uint32_t start_pos, uint32_t end_pos) const
{
    if ( end_pos >= start_pos )
    {
        if ( (start_pos < _array_size) && (end_pos < _array_size) )
        {
            char* ptr = (char*)v;
            for ( uint32_t pos = start_pos; pos <= end_pos; pos++ )
                getRaw( &ptr[ _type_size * (pos - start_pos) ], pos );
        }
    }
}

void Variant::setFromFloat(float v, uint32_t array_pos)
{
    switch ( _type )
    {
    case TypeInfo::char_field:
    {
        char tmp = (char)v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::bool_field:
    {
        uint32_t tmp = (v != 0 ) ? 1 : 0;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::int8_field:
    {
        int8_t tmp = (int8_t)v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::uint8_field:
    {
        uint8_t tmp = (uint8_t)v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::int16_field:
    {
        int16_t tmp = (int16_t)v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::uint16_field:
    {
        uint16_t tmp = (uint16_t)v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::int32_field:
    {
        int32_t tmp = (int32_t)v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::uint32_field:
    {
        uint32_t tmp = (uint32_t)v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::int64_field:
    {
        int64_t tmp = (int64_t)v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::uint64_field:
    {
        uint64_t tmp = (uint64_t)v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::double_field:
    {
        double tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    default:
        setDataAt( array_pos, &v );
    }
}

void Variant::setFromDouble(double v, uint32_t array_pos)
{
    switch ( _type )
    {
    case TypeInfo::char_field:
    {
        char tmp = (char)v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::bool_field:
    {
        uint32_t tmp = (v != 0.0 ) ? 1 : 0;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::int8_field:
    {
        int8_t tmp = (int8_t)v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::uint8_field:
    {
        uint8_t tmp = (uint8_t)v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::int16_field:
    {
        int16_t tmp = (int16_t)v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::uint16_field:
    {
        uint16_t tmp = (uint16_t)v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::int32_field:
    {
        int32_t tmp = (int32_t)v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::uint32_field:
    {
        uint32_t tmp = (uint32_t)v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::int64_field:
    {
        int64_t tmp = (int64_t)v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::uint64_field:
    {
        uint64_t tmp = (uint64_t)v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::float_field:
    {
        float tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    default:
        setDataAt( array_pos, &v );
    }}

void Variant::setFromInt8(int8_t v, uint32_t array_pos)
{
    switch ( _type )
    {
    case TypeInfo::char_field:
    {
        char tmp = (char)v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::bool_field:
    {
        uint32_t tmp = (v != 0) ? 1 : 0;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::uint8_field:
    {
        uint8_t tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::int16_field:
    {
        int16_t tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::uint16_field:
    {
        uint16_t tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::int32_field:
    {
        int32_t tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::uint32_field:
    {
        uint32_t tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::int64_field:
    {
        int64_t tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::uint64_field:
    {
        uint64_t tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::float_field:
    {
        float tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::double_field:
    {
        double tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    default:
        setDataAt( array_pos, &v );
    }}

void Variant::setFromUint8(uint8_t v, uint32_t array_pos)
{
    switch ( _type )
    {
    case TypeInfo::char_field:
    {
        char tmp = (char)v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::bool_field:
    {
        uint32_t tmp = (v != 0) ? 1 : 0;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::int8_field:
    {
        int8_t tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::int16_field:
    {
        int16_t tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::uint16_field:
    {
        uint16_t tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::int32_field:
    {
        int32_t tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::uint32_field:
    {
        uint32_t tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::int64_field:
    {
        int64_t tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::uint64_field:
    {
        uint64_t tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::float_field:
    {
        float tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::double_field:
    {
        double tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    default:
        setDataAt( array_pos, &v );
    }
}

void Variant::setFromInt16(int16_t v, uint32_t array_pos)
{
    switch ( _type )
    {
    case TypeInfo::char_field:
    {
        char tmp = (char)v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::bool_field:
    {
        uint32_t tmp = (v != 0) ? 1 : 0;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::int8_field:
    {
        int8_t tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::uint8_field:
    {
        uint8_t tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::uint16_field:
    {
        uint16_t tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::int32_field:
    {
        int32_t tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::uint32_field:
    {
        uint32_t tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::int64_field:
    {
        int64_t tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::uint64_field:
    {
        uint64_t tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::float_field:
    {
        float tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::double_field:
    {
        double tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    default:
        setDataAt( array_pos, &v );
    }}

void Variant::setFromUint16(uint16_t v, uint32_t array_pos)
{
    switch ( _type )
    {
    case TypeInfo::char_field:
    {
        char tmp = (char)v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::bool_field:
    {
        uint32_t tmp = (v != 0) ? 1 : 0;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::int8_field:
    {
        int8_t tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::uint8_field:
    {
        uint8_t tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::int16_field:
    {
        int16_t tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::int32_field:
    {
        int32_t tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::uint32_field:
    {
        uint32_t tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::int64_field:
    {
        int64_t tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::uint64_field:
    {
        uint64_t tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::float_field:
    {
        float tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::double_field:
    {
        double tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    default:
        setDataAt( array_pos, &v );
    }}

void Variant::setFromInt32(int32_t v, uint32_t array_pos)
{
    switch ( _type )
    {
    case TypeInfo::char_field:
    {
        char tmp = (char)v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::bool_field:
    {
        uint32_t tmp = (v != 0) ? 1 : 0;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::int8_field:
    {
        int8_t tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::uint8_field:
    {
        uint8_t tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::int16_field:
    {
        int16_t tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::uint16_field:
    {
        uint16_t tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::uint32_field:
    {
        uint32_t tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::int64_field:
    {
        int64_t tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::uint64_field:
    {
        uint64_t tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::float_field:
    {
        float tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::double_field:
    {
        double tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    default:
        setDataAt( array_pos, &v );
    }}

void Variant::setFromUint32(uint32_t v, uint32_t array_pos)
{
    switch ( _type )
    {
    case TypeInfo::char_field:
    {
        char tmp = (char)v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::bool_field:
    {
        uint32_t tmp = (v != 0) ? 1 : 0;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::int8_field:
    {
        int8_t tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::uint8_field:
    {
        uint8_t tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::int16_field:
    {
        int16_t tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::uint16_field:
    {
        uint16_t tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::int32_field:
    {
        int32_t tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::int64_field:
    {
        int64_t tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::uint64_field:
    {
        uint64_t tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::float_field:
    {
        float tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::double_field:
    {
        double tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    default:
        setDataAt( array_pos, &v );
    }
}

void Variant::setFromInt64(int64_t v, uint32_t array_pos)
{
    switch ( _type )
    {
    case TypeInfo::char_field:
    {
        char tmp = (char)v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::bool_field:
    {
        uint32_t tmp = (v != 0) ? 1 : 0;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::int8_field:
    {
        int8_t tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::uint8_field:
    {
        uint8_t tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::int16_field:
    {
        int16_t tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::uint16_field:
    {
        uint16_t tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::int32_field:
    {
        int32_t tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::uint32_field:
    {
        uint32_t tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::uint64_field:
    {
        uint64_t tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::float_field:
    {
        float tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::double_field:
    {
        double tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    default:
        setDataAt( array_pos, &v );
    }
}

void Variant::setFromUint64(uint64_t v, uint32_t array_pos)
{
    switch ( _type )
    {
    case TypeInfo::char_field:
    {
        char tmp = (char)v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::bool_field:
    {
        uint32_t tmp = (v != 0) ? 1 : 0;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::int8_field:
    {
        int8_t tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::uint8_field:
    {
        uint8_t tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::int16_field:
    {
        int16_t tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::uint16_field:
    {
        uint16_t tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::int32_field:
    {
        int32_t tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::uint32_field:
    {
        uint32_t tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::int64_field:
    {
        int64_t tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::float_field:
    {
        float tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::double_field:
    {
        double tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    default:
        setDataAt( array_pos, &v );
    }
}

void Variant::setFromChar(char v, uint32_t array_pos)
{
    switch ( _type )
    {
    case TypeInfo::bool_field:
    {
        uint32_t tmp = ( (v != 'f') && (v != '0') ) ? 1 : 0;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::int8_field:
    {
        int8_t tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::uint8_field:
    {
        uint8_t tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::int16_field:
    {
        int16_t tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::uint16_field:
    {
        uint16_t tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::int32_field:
    {
        int32_t tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::uint32_field:
    {
        uint32_t tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::int64_field:
    {
        int64_t tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::uint64_field:
    {
        uint64_t tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::float_field:
    {
        float tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::double_field:
    {
        double tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    default:
        setDataAt( array_pos, &v );
    }
}

void Variant::setFromBool(bool v, uint32_t array_pos)
{
    switch ( _type )
    {
    case TypeInfo::char_field:
    {
        char tmp = (char)v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::int8_field:
    {
        int8_t tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::uint8_field:
    {
        uint8_t tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::int16_field:
    {
        int16_t tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::uint16_field:
    {
        uint16_t tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::int32_field:
    {
        int32_t tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::uint32_field:
    {
        uint32_t tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::int64_field:
    {
        int64_t tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::uint64_field:
    {
        uint64_t tmp = v;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::float_field:
    {
        float tmp = v ? 1.0 : 0.0;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::double_field:
    {
        double tmp = v ? 1.0 : 0.0;
        setDataAt( array_pos, &tmp );
        break;
    }
    case TypeInfo::bool_field:
    {
        uint32_t tmp = v ? 1 : 0;
        setDataAt( array_pos, &tmp );
        break;
    }
    default:
        setDataAt( array_pos, &v );
    }
}

void Variant::setFromString(const std::string &str)
{
    uint32_t size = FrameworkUtils_min<uint32_t>( _array_size, (uint32_t)str.length() );
    uint32_t i = 0;
    uint64_t tmp = 0;
    for ( i = 0; i < size; i++ )
    {
        tmp = str.at(i);
        setDataAt( i, &tmp );
    }
    if ( i < _array_size )
    {
        tmp = '\0';
        setDataAt( i, &tmp );
    }
}

bool Variant::copyBinaryData( const Variant& other )
{
    if ( _array_size == other._array_size )
    {
        uint64_t tmp;
        for ( uint32_t i = 0; i < _array_size; i++ )
        {
            other.getDataAt( i, &tmp );
            setDataAt( i, &tmp );
        }
        return true;
    }
    return false;
}

bool Variant::copyData( const Variant& other )
{
    if ( _array_size == other._array_size )
    {
        for ( uint32_t i = 0; i < _array_size; i++ )
        {
            switch ( _type )
            {
            case TypeInfo::char_field:
            {
                char tmp = other.convertToChar( i );
                setDataAt( i, &tmp );
                break;
            }
            case TypeInfo::bool_field:
            {
                bool tmp = other.convertToBool( i );
                uint32_t x = tmp ? 1 : 0;
                setDataAt( i, &x );
                break;
            }
            case TypeInfo::int8_field:
            {
                int8_t tmp = other.convertToInt8( i );
                setDataAt( i, &tmp );
                break;
            }
            case TypeInfo::int16_field:
            {
                int16_t tmp = other.convertToInt16( i );
                setDataAt( i, &tmp );
                break;
            }
            case TypeInfo::int32_field:
            {
                int32_t tmp = other.convertToInt32( i );
                setDataAt( i, &tmp );
                break;
            }
            case TypeInfo::int64_field:
            {
                int64_t tmp = other.convertToInt64( i );
                setDataAt( i, &tmp );
                break;
            }
            case TypeInfo::uint8_field:
            {
                uint8_t tmp = other.convertToUint8( i );
                setDataAt( i, &tmp );
                break;
            }
            case TypeInfo::uint16_field:
            {
                uint16_t tmp = other.convertToUint16( i );
                setDataAt( i, &tmp );
                break;
            }
            case TypeInfo::uint32_field:
            {
                uint32_t tmp = other.convertToUint32( i );
                setDataAt( i, &tmp );
                break;
            }
            case TypeInfo::uint64_field:
            {
                uint64_t tmp = other.convertToUint64( i );
                setDataAt( i, &tmp );
                break;
            }
            case TypeInfo::float_field:
            {
                float tmp = other.convertToFloat( i );
                setDataAt( i, &tmp );
                break;
            }
            case TypeInfo::double_field:
            {
                double tmp = other.convertToDouble( i );
                setDataAt( i, &tmp );
                break;
            }
            default:
                return false;
            }
        }
        return true;
    }
    return false;
}

bool Variant::isLessThan(const Variant &other, int32_t array_pos, int32_t other_array_pos) const
{
    if ( _type != other._type )
        return false;
    switch ( _type )
    {
    case TypeInfo::char_field:
    case TypeInfo::int8_field:
    case TypeInfo::int16_field:
    case TypeInfo::int32_field:
    case TypeInfo::int64_field:
        return getAsUint64(array_pos) < other.getAsUint64(other_array_pos);
        break;

    case TypeInfo::bool_field:
        return getAsUint32(array_pos) < other.getAsUint32(other_array_pos);
        break;

    case TypeInfo::uint8_field:
    case TypeInfo::uint16_field:
    case TypeInfo::uint32_field:
    case TypeInfo::uint64_field:
        return getAsUint64(array_pos) < other.getAsUint64(other_array_pos);
        break;

    case TypeInfo::float_field:
        return getAsFloat(array_pos) < other.getAsFloat(other_array_pos);
        break;

    case TypeInfo::double_field:
        return getAsDouble(array_pos) < other.getAsDouble(other_array_pos);

    default:
        return false;
    }
}

bool Variant::isMoreThan(const Variant &other, int32_t array_pos, int32_t other_array_pos) const
{
    if ( _type != other._type )
        return false;
    switch ( _type )
    {
    case TypeInfo::char_field:
    case TypeInfo::int8_field:
    case TypeInfo::int16_field:
    case TypeInfo::int32_field:
    case TypeInfo::int64_field:
        return getAsUint64(array_pos) > other.getAsUint64(other_array_pos);
        break;

    case TypeInfo::bool_field:
        return getAsUint32(array_pos) > other.getAsUint32(other_array_pos);
        break;

    case TypeInfo::uint8_field:
    case TypeInfo::uint16_field:
    case TypeInfo::uint32_field:
    case TypeInfo::uint64_field:
        return getAsUint64(array_pos) > other.getAsUint64(other_array_pos);
        break;

    case TypeInfo::float_field:
        return getAsFloat(array_pos) > other.getAsFloat(other_array_pos);
        break;

    case TypeInfo::double_field:
        return getAsDouble(array_pos) > other.getAsDouble(other_array_pos);

    default:
        return false;
    }
}
