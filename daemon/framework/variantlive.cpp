#include "variantlive.h"
#include <string.h>

using namespace FrameworkLibrary;

void VariantLive::initFromStaticRW(VariantLive &vl, VariantStatic &other)
{
    vl._array_size = other._array_size;
    vl._type = other._type;
    vl._type_size = other._type_size;
    vl._read_ptr = other._data_store;
    vl._write_ptr = other._data_store;
}

VariantLive::VariantLive():
    Variant( TypeInfo::unknown_type, 0)
{
    _read_ptr = NULL;
    _write_ptr = NULL;
}

VariantLive::VariantLive( const VariantLive& other ):
    Variant( other )
{
    _read_ptr = other._read_ptr;
    _write_ptr = other._write_ptr;
}

VariantLive::VariantLive(const VariantStatic &other):
    Variant( other )
{
    _read_ptr = other._data_store;
    _write_ptr = NULL;
}

VariantLive::VariantLive(float *v, uint32_t array_size):
    Variant( TypeInfo::float_field, array_size )
{
    _read_ptr = (void*)v;
    _write_ptr = (void*)v;
}

VariantLive::VariantLive(double *v, uint32_t array_size):
    Variant( TypeInfo::double_field, array_size )
{
    _read_ptr = (void*)v;
    _write_ptr = (void*)v;
}

VariantLive::VariantLive(uint8_t *v, uint32_t array_size):
    Variant( TypeInfo::uint8_field, array_size )
{
    _read_ptr = (void*)v;
    _write_ptr = (void*)v;
}

VariantLive::VariantLive(uint16_t *v, uint32_t array_size):
    Variant( TypeInfo::uint16_field, array_size )
{
    _read_ptr = (void*)v;
    _write_ptr = (void*)v;
}

VariantLive::VariantLive(uint32_t *v, uint32_t array_size):
    Variant( TypeInfo::uint32_field, array_size )
{
    _read_ptr = (void*)v;
    _write_ptr = (void*)v;
}

VariantLive::VariantLive(uint64_t *v, uint32_t array_size):
    Variant( TypeInfo::uint64_field, array_size )
{
    _read_ptr = (void*)v;
    _write_ptr = (void*)v;
}

VariantLive::VariantLive(int8_t *v, uint32_t array_size):
    Variant( TypeInfo::int8_field, array_size )
{
    _read_ptr = (void*)v;
    _write_ptr = (void*)v;
}

VariantLive::VariantLive(int16_t *v, uint32_t array_size):
    Variant( TypeInfo::int16_field, array_size )
{
    _read_ptr = (void*)v;
    _write_ptr = (void*)v;
}

VariantLive::VariantLive(int32_t *v, uint32_t array_size):
    Variant( TypeInfo::int32_field, array_size )
{
    _read_ptr = (void*)v;
    _write_ptr = (void*)v;
}

VariantLive::VariantLive(int64_t *v, uint32_t array_size):
    Variant( TypeInfo::int64_field, array_size )
{
    _read_ptr = (void*)v;
    _write_ptr = (void*)v;
}

VariantLive::VariantLive(char *v, uint32_t array_size):
    Variant( TypeInfo::char_field, array_size )
{
    _read_ptr = (void*)v;
    _write_ptr = (void*)v;
}

VariantLive::VariantLive(const float *v, uint32_t array_size):
    Variant( TypeInfo::float_field, array_size )
{
    _read_ptr = (void*)v;
    _write_ptr = NULL;
}

VariantLive::VariantLive(const double *v, uint32_t array_size):
    Variant( TypeInfo::double_field, array_size )
{
    _read_ptr = (void*)v;
    _write_ptr = NULL;
}

VariantLive::VariantLive(const uint8_t *v, uint32_t array_size):
    Variant( TypeInfo::uint8_field, array_size )
{
    _read_ptr = (void*)v;
    _write_ptr = NULL;
}

VariantLive::VariantLive(const uint16_t *v, uint32_t array_size):
    Variant( TypeInfo::uint16_field, array_size )
{
    _read_ptr = (void*)v;
    _write_ptr = NULL;
}

VariantLive::VariantLive(const uint32_t *v, uint32_t array_size):
    Variant( TypeInfo::uint32_field, array_size )
{
    _read_ptr = (void*)v;
    _write_ptr = NULL;
}

VariantLive::VariantLive(const uint64_t *v, uint32_t array_size):
    Variant( TypeInfo::uint64_field, array_size )
{
    _read_ptr = (void*)v;
    _write_ptr = NULL;
}

VariantLive::VariantLive(const int8_t *v, uint32_t array_size):
    Variant( TypeInfo::int8_field, array_size )
{
    _read_ptr = (void*)v;
    _write_ptr = NULL;
}

VariantLive::VariantLive(const int16_t *v, uint32_t array_size):
    Variant( TypeInfo::int16_field, array_size )
{
    _read_ptr = (void*)v;
    _write_ptr = NULL;
}

VariantLive::VariantLive(const int32_t *v, uint32_t array_size):
    Variant( TypeInfo::int32_field, array_size )
{
    _read_ptr = (void*)v;
    _write_ptr = NULL;
}

VariantLive::VariantLive(const int64_t *v, uint32_t array_size):
    Variant( TypeInfo::int64_field, array_size )
{
    _read_ptr = (void*)v;
    _write_ptr = NULL;
}

VariantLive::VariantLive(const char *v, uint32_t array_size):
    Variant( TypeInfo::char_field, array_size )
{
    _read_ptr = (void*)v;
    _write_ptr = NULL;
}

VariantLive::VariantLive( const TypeInfo& type, const void* ptr_data, uint32_t array_size):
    Variant( type, array_size )
{
    _read_ptr = ptr_data;
    _write_ptr = NULL;
}

VariantLive::VariantLive( const TypeInfo& type, void* ptr_data, uint32_t array_size):
    Variant( type, array_size )
{
    _read_ptr = ptr_data;
    _write_ptr = ptr_data;
}

VariantLive::VariantLive(const TypeInfo &):
    Variant( TypeInfo(), 0 )
{
    _read_ptr = NULL;
    _write_ptr = NULL;
}

VariantLive::~VariantLive()
{
    _read_ptr = NULL;
    _write_ptr = NULL;
}

VariantStatic VariantLive::copyToStatic() const
{
    return VariantStatic( _type, _read_ptr, _array_size );
}

VariantStatic VariantLive::reinterpretType(const TypeInfo &type) const
{
    VariantStatic ret( type, _array_size );
    ret.copyBinaryData( *this );
    return ret;
}

VariantStatic VariantLive::recastType(const TypeInfo &type) const
{
    VariantStatic ret( type, _array_size );
    ret.copyData( *this );
    return ret;
}

const VariantLive& VariantLive::operator=(const VariantLive& other )
{
    _array_size = other._array_size;
    _type = other._type;
    _type_size = other._type_size;
    _read_ptr = other._read_ptr;
    _write_ptr = other._write_ptr;
    return *this;
}

void VariantLive::getDataAt(uint32_t n, void *dest) const
{
    memcpy( dest, &((uint8_t*)_read_ptr)[ _type_size*n ], _type_size );
}

void VariantLive::setDataAt(uint32_t n, const void *data)
{
    memcpy( &((uint8_t*)_write_ptr)[ _type_size*n ], data, _type_size );
}
