#include "variantstatic.h"

#include <stdio.h>
#include <string.h>
#include <debugprint.h>
#include <memorychecker.h>

using namespace FrameworkLibrary;

bool VariantStatic::initFromSerialized(VariantStatic &variant, const std::string &blob)
{
    TypeInfo type;
    uint32_t array_size = 0;
    uint8_t* unserialized_data = (uint8_t*)Variant::unserialize( blob, type, array_size );
    if ( unserialized_data != NULL )
    {
        variant._array_size = array_size;
        variant._type = type;
        variant._type_size = type.getTypeSize();
        if ( variant._data_store != NULL )
            delete [] variant._data_store;
        variant._data_store = unserialized_data;
        return true;
    }
    return false;
}

void VariantStatic::initFromFloat(VariantStatic &variant, float v)
{
    variant._type = TypeInfo::float_field;
    variant._type_size = variant._type.getTypeSize();
    variant._array_size = 1;
    variant.prepareBuffer();
    if ( variant._data_store !=  NULL )
        memcpy( variant._data_store, &v, sizeof(v) );
}

void VariantStatic::initFromDouble(VariantStatic &variant, double v)
{
    variant._type = TypeInfo::double_field;
    variant._type_size = variant._type.getTypeSize();
    variant._array_size = 1;
    variant.prepareBuffer();
    if ( variant._data_store !=  NULL )
        memcpy( variant._data_store, &v, sizeof(v) );
}

void VariantStatic::initFromUint8(VariantStatic &variant, uint8_t v)
{
    variant._type = TypeInfo::uint8_field;
    variant._type_size = variant._type.getTypeSize();
    variant._array_size = 1;
    variant.prepareBuffer();
    if ( variant._data_store !=  NULL )
        memcpy( variant._data_store, &v, sizeof(v) );
}

void VariantStatic::initFromUint16(VariantStatic &variant, uint16_t v)
{
    variant._type = TypeInfo::uint16_field;
    variant._type_size = variant._type.getTypeSize();
    variant._array_size = 1;
    variant.prepareBuffer();
    if ( variant._data_store !=  NULL )
        memcpy( variant._data_store, &v, sizeof(v) );
}

void VariantStatic::initFromUint32(VariantStatic &variant, uint32_t v)
{
    variant._type = TypeInfo::uint32_field;
    variant._type_size = variant._type.getTypeSize();
    variant._array_size = 1;
    variant.prepareBuffer();
    if ( variant._data_store !=  NULL )
        memcpy( variant._data_store, &v, sizeof(v) );
}

void VariantStatic::initFromUint64(VariantStatic &variant, uint64_t v)
{
    variant._type = TypeInfo::uint64_field;
    variant._type_size = variant._type.getTypeSize();
    variant._array_size = 1;
    variant.prepareBuffer();
    if ( variant._data_store !=  NULL )
        memcpy( variant._data_store, &v, sizeof(v) );
}

void VariantStatic::initFromInt8(VariantStatic &variant, int8_t v)
{
    variant._type = TypeInfo::int8_field;
    variant._type_size = variant._type.getTypeSize();
    variant._array_size = 1;
    variant.prepareBuffer();
    if ( variant._data_store !=  NULL )
        memcpy( variant._data_store, &v, sizeof(v) );
}

void VariantStatic::initFromInt16(VariantStatic &variant, int16_t v)
{
    variant._type = TypeInfo::int16_field;
    variant._type_size = variant._type.getTypeSize();
    variant._array_size = 1;
    variant.prepareBuffer();
    if ( variant._data_store !=  NULL )
        memcpy( variant._data_store, &v, sizeof(v) );
}

void VariantStatic::initFromInt32(VariantStatic &variant, int32_t v)
{
    variant._type = TypeInfo::int32_field;
    variant._type_size = variant._type.getTypeSize();
    variant._array_size = 1;
    variant.prepareBuffer();
    if ( variant._data_store !=  NULL )
        memcpy( variant._data_store, &v, sizeof(v) );
}

void VariantStatic::initFromInt64(VariantStatic &variant, int64_t v)
{
    variant._type = TypeInfo::int64_field;
    variant._type_size = variant._type.getTypeSize();
    variant._array_size = 1;
    variant.prepareBuffer();
    if ( variant._data_store !=  NULL )
        memcpy( variant._data_store, &v, sizeof(v) );
}

void VariantStatic::initFromBool(VariantStatic &variant, bool v)
{
    variant._type = TypeInfo::bool_field;
    variant._type_size = variant._type.getTypeSize();
    variant._array_size = 1;
    variant.prepareBuffer();
    if ( variant._data_store !=  NULL )
    {
        uint32_t var = v ? 1 : 0;
        memcpy( variant._data_store, &var, sizeof(var) );
    }
}

void VariantStatic::initFromChar(VariantStatic &variant, char v)
{
    variant._type = TypeInfo::char_field;
    variant._type_size = variant._type.getTypeSize();
    variant._array_size = 1;
    variant.prepareBuffer();
    if ( variant._data_store !=  NULL )
        memcpy( variant._data_store, &v, sizeof(v) );
}

void VariantStatic::initFromString(VariantStatic &variant, const char* v)
{
    variant._type = TypeInfo::char_field;
    variant._type_size = variant._type.getTypeSize();
    variant._array_size = strlen( v );
    variant.prepareBuffer();
    if ( variant._data_store !=  NULL )
        memcpy( variant._data_store, v, sizeof(char) * variant._array_size );
}

void VariantStatic::initFromString(VariantStatic &variant, const std::string& v)
{
    variant._type = TypeInfo::char_field;
    variant._type_size = variant._type.getTypeSize();
    variant._array_size = v.length();
    variant.prepareBuffer();
    if ( variant._data_store !=  NULL )
        memcpy( variant._data_store, v.data(), sizeof(char) * variant._array_size );
}



VariantStatic::VariantStatic():
    Variant( TypeInfo(),  0)
{
    _data_store = NULL;
}


VariantStatic::VariantStatic(const VariantStatic &other):
    Variant( other )
{
    _data_store = NULL;
    prepareBuffer();
    copyBinaryData( other );
}

VariantStatic::VariantStatic(const TypeInfo &t, const std::string& value ):
    Variant( t, 1 )
{
    _data_store = NULL;
    prepareBuffer();
    fromHumanString( value );
}

VariantStatic::VariantStatic( const TypeInfo& t, uint32_t array_size ):
    Variant( t, array_size )
{
    _data_store = NULL;
    prepareBuffer();
    if ( _data_store != NULL )
        memset( _data_store, 0, _type_size * _array_size );
}

VariantStatic::VariantStatic( const TypeInfo& t, const void* ptr_data, uint32_t array_size ):
    Variant( t, array_size )
{
    _data_store = NULL;
    prepareBuffer();
    if ( _data_store != NULL )
    {
        if ( _type == TypeInfo::bool_field )
        {   // Since size of bool is system-dependent, it needs a special threatment:
            const bool* tmp = (const bool*)ptr_data;
            for ( uint32_t a = 0; a < _array_size; a++ )
                ((uint32_t*)_data_store)[a] = tmp[a] ? 1 : 0;
        }
        else
            memcpy( _data_store, ptr_data, _type_size * _array_size );
    }
}

VariantStatic::~VariantStatic()
{
    if ( _data_store != NULL )
    {
        delete [] _data_store;
        _data_store = NULL;
    }
}

const VariantStatic &VariantStatic::operator =(const VariantStatic & other)
{
    if ( this != &other )
    {
        _type = other._type;
        _array_size = other._array_size;
        _type_size = other._type_size;
        if ( other._data_store != NULL )
        {
            prepareBuffer();
            if ( _data_store != NULL )
                memcpy( _data_store, other._data_store, _array_size * _type_size );
        }
        else if ( _data_store != NULL )
        {
            delete [] _data_store;
            _data_store = NULL;
        }
    }
    return *this;
}

void VariantStatic::prepareBuffer()
{
    if ( _data_store != NULL )
    {
        delete [] _data_store;
        _data_store = NULL;
    }
    if ( (_type_size != 0) && (_array_size > 0) )
        _data_store = new uint8_t[ _array_size * _type_size ];
}

void VariantStatic::getDataAt(uint32_t n, void *dest) const
{
    memcpy( dest, &_data_store[ n * _type_size ], _type_size );
}

void VariantStatic::setDataAt(uint32_t n, const void *data)
{
    memcpy( &_data_store[ n * _type_size ], data, _type_size );
}
