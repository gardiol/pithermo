#include "debugprint.h"

#include "frameworkutils.h"
#include "profiler.h"


#if defined( FRAMEWORK_PLATFORM_XENOMAI )
#include <rtdk.h>
#define LOCAL_PRINTF rt_printf
#else
#include <stdio.h>
#include <string.h>
#define LOCAL_PRINTF printf
#endif

// I know... this SHOULD NOT BE USED. But:
// 1. It's used only inside this very source file and nowhere else
// 2. It's fast(er)
#define DEBUG_PRINT_BUFFSIZE 80
#define DEBUG_PRINT_CONVBUFF_SIZE 25

using namespace FrameworkLibrary;

namespace FrameworkLibrary {

class __DebugPrintPrivate
{
    friend class DebugPrint;
    friend DLLEXPORT DebugPrint debugPrint( const std::string& tag, DebugPrint::DebugClass debug_class );
    friend DLLEXPORT DebugPrint debugPrintNotice( const std::string& tag );
    friend DLLEXPORT DebugPrint debugPrintError( const std::string& tag );
    friend DLLEXPORT DebugPrint debugPrintWarning( const std::string& tag );
    friend DLLEXPORT DebugPrint debugPrintUntagged( DebugPrint::DebugClass level );

    static bool _initialized;
    static std::string _default_tag;

    // This is kept as a bit mask to maximize speed:
    static uint32_t _active_classes;
    static std::map<DebugPrint::DebugClass, std::string> _classes;
    static std::map<DebugPrint::DebugClass, std::string> _descriptions;

    static bool _file_output_enable;
    static std::string _file_output;

    static void _initialize()
    {
        if ( _initialized )
            return;
        _initialized = true;

#if defined( FRAMEWORK_PLATFORM_XENOMAI )
    rt_print_auto_init(1);
#endif
        _classes[ DebugPrint::WARNING_CLASS ] = "WARNING";
        _descriptions[ DebugPrint::WARNING_CLASS ] = "Generic warnings";

        _classes[ DebugPrint::NOTICE_CLASS ] = "NOTICE";
        _descriptions[ DebugPrint::NOTICE_CLASS ] = "Normal prints";

        _classes[ DebugPrint::THREAD_CLASS ] = "THREAD";
        _descriptions[ DebugPrint::THREAD_CLASS ] = "thread and mutex specific notices";

        _classes[ DebugPrint::MEMORYCHECKER_CLASS ] = "MEMORYAREA";
        _descriptions[ DebugPrint::MEMORYCHECKER_CLASS ] = "Memory checker notices";

        _classes[ DebugPrint::LICENSE_CLASS ] = "LICENSE";
        _descriptions[ DebugPrint::LICENSE_CLASS ] = "License issues";

        _classes[ DebugPrint::COMMS_CLASS ] = "COMMS";
        _descriptions[ DebugPrint::COMMS_CLASS ] = "Comms, sockets and interfaces notices";

        _classes[ DebugPrint::PLUGIN_CLASS ] = "PLUGIN";
        _descriptions[ DebugPrint::PLUGIN_CLASS ] = "Plugins and dynamic libraries";

        _classes[ DebugPrint::COMPRESS_CLASS ] = "COMPRESS";
        _descriptions[ DebugPrint::COMPRESS_CLASS ] = "Compression and decompression messages";

        _classes[ DebugPrint::MEMORYAREA_CLASS ] = "MEMORYAREA";
        _descriptions[ DebugPrint::MEMORYAREA_CLASS ] = "Memory areas and shared memory notices";

        _classes[ DebugPrint::CONFIG_CLASS ] = "CONFIG";
        _descriptions[ DebugPrint::CONFIG_CLASS ] = "Config files messages";

        _classes[ DebugPrint::SIGNAL_CLASS ] = "SIGNAL";
        _descriptions[ DebugPrint::SIGNAL_CLASS ] = "Signals and sighandlers notices";

        _classes[ DebugPrint::PROCESS_CLASS ] = "PROCESS";
        _descriptions[ DebugPrint::PROCESS_CLASS ] = "Processes launch and control issues";

        _classes[ DebugPrint::SCALER_CLASS ] = "SCALER";
        _descriptions[ DebugPrint::SCALER_CLASS ] = "Scalers messages";

        _classes[ DebugPrint::DATACHANNEL_CLASS ] = "DATACHANNEL";
        _descriptions[ DebugPrint::DATACHANNEL_CLASS ] = "Ncfs2 data exchange issues";

        _classes[ DebugPrint::TIMER_CLASS ] = "TIMER";
        _descriptions[ DebugPrint::TIMER_CLASS ] = "Timers and timing issues";

        _classes[ DebugPrint::IO_CLASS ] = "IO";
        _descriptions[ DebugPrint::IO_CLASS ] = "I/O interface and devices notices";

        _classes[ DebugPrint::LEGACY_CLASS ] = "LEGACY";
        _descriptions[ DebugPrint::LEGACY_CLASS ] = "Legacy Wrapper messages";

        _classes[ DebugPrint::MUTEX_CLASS ] = "MUTEX";
        _descriptions[ DebugPrint::MUTEX_CLASS ] = "Mutex lock/unlock messages";

        _classes[ DebugPrint::USER1_CLASS ] = "USER1";
        if ( _descriptions.find( DebugPrint::USER1_CLASS ) == _descriptions.end() )
            _descriptions[ DebugPrint::USER1_CLASS ] = "User-specific messages, class 1";

        _classes[ DebugPrint::USER2_CLASS ] = "USER2";
        if ( _descriptions.find( DebugPrint::USER2_CLASS ) == _descriptions.end() )
            _descriptions[ DebugPrint::USER2_CLASS ] = "User-specific messages, class 2";

        _classes[ DebugPrint::USER3_CLASS ] = "USER3";
        if ( _descriptions.find( DebugPrint::USER3_CLASS ) == _descriptions.end() )
            _descriptions[ DebugPrint::USER3_CLASS ] = "User-specific messages, class 3";

        _classes[ DebugPrint::USER4_CLASS ] = "USER4";
        if ( _descriptions.find( DebugPrint::USER4_CLASS ) == _descriptions.end() )
            _descriptions[ DebugPrint::USER4_CLASS ] = "User-specific messages, class 4";

        if ( FrameworkUtils::check_env( "FRAMEWORK_DEBUG" ) )
            FrameworkUtils::reset_env("FRAMEWORK_DEBUG");

        if ( FrameworkUtils::check_env( "FRAMEWORK_DEBUG_FILE" ) )
        {
            _file_output_enable = true;
            _file_output = FrameworkUtils::read_env( "FRAMEWORK_DEBUG_FILE" );
            LOCAL_PRINTF(" Output to file enabled: FRAMEWORK_DEBUG_FILE = %s\n", _file_output.c_str() );
            FrameworkUtils::reset_env("FRAMEWORK_DEBUG_FILE");
        }
        else
            LOCAL_PRINTF(" Output to file disabled: FRAMEWORK_DEBUG_FILE not set.\n");

        // Fix WARNING and NOTICE levels first, they are active by default:
        if ( FrameworkUtils::read_env( "FRAMEWORK_DEBUG_WARNING" ) == "0" )
            FrameworkUtils::write_env( "FRAMEWORK_DEBUG_WARNING", "0" );
        else
            FrameworkUtils::write_env( "FRAMEWORK_DEBUG_WARNING", "1" );
        if ( FrameworkUtils::read_env( "FRAMEWORK_DEBUG_NOTICE" ) == "0" )
            FrameworkUtils::write_env( "FRAMEWORK_DEBUG_NOTICE", "0" );
        else
            FrameworkUtils::write_env( "FRAMEWORK_DEBUG_NOTICE", "1" );

        for ( std::map<DebugPrint::DebugClass, std::string>::const_iterator i = __DebugPrintPrivate::_classes.begin();
              i != __DebugPrintPrivate::_classes.end();
              ++i )
        {
            std::string class_value = FrameworkUtils::read_env( "FRAMEWORK_DEBUG_" + i->second );
            if ( (class_value != "0") && (class_value != "") )
            {
                _active_classes = _active_classes | i->first;
                FrameworkUtils::reset_env( "FRAMEWORK_DEBUG_" + i->second );
            }
        }
    }
};

bool __DebugPrintPrivate::_initialized = false;
uint32_t __DebugPrintPrivate::_active_classes =  DebugPrint::ERROR_CLASS;

bool __DebugPrintPrivate::_file_output_enable = false;
std::string __DebugPrintPrivate::_file_output = "";
std::map<DebugPrint::DebugClass, std::string> __DebugPrintPrivate::_descriptions;
std::map<DebugPrint::DebugClass, std::string> __DebugPrintPrivate::_classes;
std::string __DebugPrintPrivate::_default_tag = "DebugPrint";


class DebugPrint::__privateDebugPrint
{
public:
    inline __privateDebugPrint( const char* tag, uint32_t tag_size, bool err ):
        _isError(err),
        _buffer_pos(tag_size)
    {
        memcpy( _buffer, tag, tag_size );
        _buffer[ _buffer_pos++ ] = ' ';
        _buffer[ _buffer_pos++ ] = '-';
        _buffer[ _buffer_pos++ ] = '>';
        _buffer[ _buffer_pos++ ] = ' ';
        _conv_buf[ DEBUG_PRINT_CONVBUFF_SIZE-1 ] = '\0';

    }

    explicit inline __privateDebugPrint( bool err ):
        _isError(err),
        _buffer_pos(0)
    {
        _buffer[0] = '\0';
        _conv_buf[ DEBUG_PRINT_CONVBUFF_SIZE-1 ] = '\0';
    }

    inline ~__privateDebugPrint()
    {
        flush();
    }

    inline void append( const char* data, uint32_t data_size )
    {
        uint32_t step = DEBUG_PRINT_BUFFSIZE - _buffer_pos;
        while ( data_size > 0 )
        {
            if ( step > data_size )
                step = data_size;
            memcpy( &_buffer[ _buffer_pos ], data, step );
            data += step;
            _buffer_pos += step;
            data_size -= step;
            if ( _buffer_pos >= DEBUG_PRINT_BUFFSIZE )
            {
                flush();
                step = DEBUG_PRINT_BUFFSIZE;
            }
        }
    }

    inline std::string getBuffer() const
    {
        return std::string( _buffer, _buffer_pos );
    }

    inline void flush()
    {
        if ( _buffer_pos > 0 )
        {
            if ( __DebugPrintPrivate::_file_output_enable )
            {
                FILE* file = fopen( __DebugPrintPrivate::_file_output.c_str(), "a" );
                if ( file != NULL )
                {
                    fwrite( _buffer, _buffer_pos, 1, file );
                    fclose(file);
                }
            }
#if defined( FRAMEWORK_PLATFORM_XENOMAI )
            _buffer[ _buffer_pos ] = '\0';
            rt_printf( "%s", _buffer );
#else
            fwrite( _buffer, _buffer_pos, 1, _isError ? stderr : stdout );
#endif
            _buffer_pos = 0;
            // Windows does nt have pseudo-TTY, this change is needed to ensure at least framework output is correctly
            //  flushed to the output pipes
#if defined(FRAMEWORK_PLATFORM_WINDOWS)
            fflush( _isError ? stderr : stdout );
#endif
        }
    }

public:
    inline void appendUnsigned( uint64_t data )
    {
        uint32_t pos = DEBUG_PRINT_CONVBUFF_SIZE-2; // last = \0, start from last-1
        while ( data >= 10 )
        {
            _conv_buf[ pos-- ] = '0' + ( data % 10 );
            data = data / 10;
        }\
        _conv_buf[ pos ] = '0' + ( data % 10 );
        append( &_conv_buf[ pos ], (DEBUG_PRINT_CONVBUFF_SIZE-1) - pos );
    }

    inline void appendSigned( int64_t data )
    {
        if ( (uint64_t)data == 0x8000000000000000ULL )
//        if ( (uint8_t)data == 0x8000000000000000ULL ) error Expression '(uint8_t) data == 0x8000000000000000ULL' is always false. The value range of unsigned char type: [0, 255]
        {
            char buf[21] = "-9223372036854775808";
            append( buf, 20 );
        }
        else
        {
            if ( data < 0 )
            {
                append( "-", 1 );
                appendUnsigned( -data );
            }
            else
                appendUnsigned( data );
        }
    }

    inline void appendDouble( double data )
    {
        int64_t int_part = data;
        appendSigned( int_part );
        append( ".", 1 );
        double mantissa_part = data - (double)int_part;
        if ( mantissa_part <  0 )
            mantissa_part = -mantissa_part;
        uint32_t mantissa_length = 5;
        while ( (mantissa_length-- > 0) && (mantissa_part != (uint32_t)mantissa_part) )
        {
            mantissa_part *= 10;
            char c = '0' + ((uint32_t)mantissa_part) % 10;
            append( &c, 1 );
        }
    }

    inline void appendHex( uint64_t data )
    {
        static const char bytes[16] = { '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F' };
        append( "0x", 2 );
        int coef = 60;
        while ( ((data >> coef) & 0x000000000000000F) == 0 )
            coef -= 4;
        while (coef > 0)
        {
            append( bytes + ((data >> coef) & 0x000000000000000F), 1 );
            coef -= 4;
        }
    }

private:
    bool _isError;
    char _buffer[ DEBUG_PRINT_BUFFSIZE+1 ]; // +1 is for XENOMAI where an additional \0 is needed
    char _conv_buf[ DEBUG_PRINT_CONVBUFF_SIZE ];
    uint32_t _buffer_pos;
};

}

DebugPrint::DebugPrint(const std::string& tag,
                       DebugClass debug_class):
    _private(NULL)
{
    __DebugPrintPrivate::_initialize();

    if ( ( __DebugPrintPrivate::_active_classes  & debug_class ) || ( debug_class == DebugPrint::ERROR_CLASS ) )
        _private = new __privateDebugPrint( tag.c_str(), tag.length(), debug_class == DebugPrint::ERROR_CLASS );
}

DebugPrint::DebugPrint(DebugClass debug_class):
    _private(NULL)
{
    __DebugPrintPrivate::_initialize();

    if ( ( __DebugPrintPrivate::_active_classes  & debug_class ) || ( debug_class == DebugPrint::ERROR_CLASS ) )
        _private = new __privateDebugPrint( debug_class == DebugPrint::ERROR_CLASS );
}

std::string DebugPrint::getDefaultTag()
{
    return __DebugPrintPrivate::_default_tag;
}

std::map<DebugPrint::DebugClass, bool> DebugPrint::getClasses()
{
    __DebugPrintPrivate::_initialize();
    std::map<DebugPrint::DebugClass, bool> ret;
    for ( std::map<DebugClass, std::string>::const_iterator i = __DebugPrintPrivate::_classes.begin();
          i != __DebugPrintPrivate::_classes.end();
          ++i )
    {
        DebugClass debug_class = i->first;
        ret[ debug_class ] = __DebugPrintPrivate::_active_classes & debug_class;
    }
    return ret;

}

std::string DebugPrint::describeClass(DebugPrint::DebugClass debug_class)
{
    if ( __DebugPrintPrivate::_classes.find( debug_class ) != __DebugPrintPrivate::_classes.end() )
        return __DebugPrintPrivate::_descriptions[ debug_class ];
    else
        return "Unknown class";
}

std::map<std::string, std::string> DebugPrint::getEnvironmentClass(const std::map<DebugClass, bool> &debug_classes)
{
    std::string base("FRAMEWORK_DEBUG_");
    __DebugPrintPrivate::_initialize();
    std::map<std::string, std::string> ret;
    for ( std::map<DebugClass, bool>::const_iterator i = debug_classes.begin();
          i != debug_classes.end();
          ++i )
    {
        DebugClass debug_class = i->first;
        bool enabled = i->second;
        std::map<DebugPrint::DebugClass, std::string>::const_iterator j =__DebugPrintPrivate::_classes.find( debug_class );
        if ( j != __DebugPrintPrivate::_classes.end() )
        {
            std::string class_name = j->second;
            ret[ base + class_name ] = enabled ? "1" : "0";
        }
    }
    return ret;
}

void DebugPrint::registerUserType(DebugClass user_class, const std::string &description)
{
    if ( __DebugPrintPrivate::_descriptions.find( user_class ) != __DebugPrintPrivate::_descriptions.end() )
        __DebugPrintPrivate::_descriptions[ user_class ] += "/";
    __DebugPrintPrivate::_descriptions[ user_class ] += description ;
}

DebugPrint::~DebugPrint()
{
    if ( _private != NULL )
        delete _private;
}

DebugPrint& DebugPrint::operator<<(const std::string& str )
{
    if ( _private != NULL )
        _private->append( str.c_str(), str.length() );
    return *this;
}

DebugPrint &DebugPrint::operator<<(char * data)
{
    if ( _private != NULL )
        _private->append( data, strlen( data ) );
    return *this;
}

DebugPrint &DebugPrint::operator<<(const char * data)
{
    if ( _private != NULL )
        _private->append( data, strlen( data ) );
    return *this;
}

DebugPrint &DebugPrint::operator<<(char data)
{
    if ( _private != NULL )
        _private->append( &data, 1 );
    return *this;
}

DebugPrint& DebugPrint::operator<<(uint64_t data)
{
    if ( _private != NULL )
        _private->appendUnsigned( data );
    return *this;
}

DebugPrint& DebugPrint::operator<<(int64_t data)
{
    if ( _private != NULL )
        _private->appendSigned( data );
    return *this;
}

DebugPrint& DebugPrint::operator<<(uint32_t data)
{
    if ( _private != NULL )
        _private->appendUnsigned( data );
    return *this;
}

DebugPrint& DebugPrint::operator<<(int32_t data)
{
    if ( _private != NULL )
        _private->appendSigned( data );
    return *this;
}

DebugPrint& DebugPrint::operator<<(uint16_t data)
{
    if ( _private != NULL )
        _private->appendUnsigned( data );
    return *this;
}

DebugPrint& DebugPrint::operator<<(int16_t data)
{
    if ( _private != NULL )
        _private->appendSigned( data );
    return *this;
}

DebugPrint& DebugPrint::operator<<(uint8_t data)
{
    if ( _private != NULL )
        _private->appendUnsigned( data );
    return *this;
}

DebugPrint& DebugPrint::operator<<(int8_t data)
{
    if ( _private != NULL )
        _private->appendSigned( data );
    return *this;
}

DebugPrint& DebugPrint::operator<<(float data)
{
    if ( _private != NULL )
        _private->appendDouble( data );
    return *this;
}

DebugPrint& DebugPrint::operator<<(double data)
{
    if ( _private != NULL )
        _private->appendDouble( data );
    return *this;
}

DebugPrint &DebugPrint::operator <<(void * data)
{
    if ( _private != NULL )
        _private->appendHex( (uint64_t)data );
    return *this;
}

FrameworkLibrary::DebugPrint::operator std::string() const
{
    return _private->getBuffer();
}

void DebugPrint::setDefaultTag(const std::string &str)
{
    __DebugPrintPrivate::_default_tag = str;
}

void FrameworkLibrary::debugPrintFlush( DebugPrint::DebugClass debug_class )
{
#if defined( FRAMEWORK_PLATFORM_XENOMAI )
        rt_print_flush_buffers();
#else
    fflush( debug_class == DebugPrint::ERROR_CLASS ? stderr : stdout );
#endif
}
