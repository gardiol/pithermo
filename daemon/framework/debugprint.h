/**
  @file debugprint.h
 @brief Debug facility
*/
#ifndef DEBUGPRINT_H
#define DEBUGPRINT_H

#include <common_defs.h>

#include <string>
#include <map>

namespace FrameworkLibrary {

/**
  @addtogroup DEBUG_HELPERS
  @{
  */

//class __DebugPrintPrivate;

/** @brief This is a commodity tool to provide print outs, for debug or whatever.
 *
 * @note No C wrapper interface if available for this class, please use printf() instead.
 *
 * It works "chain style", so you can debug like this:
 * @code
 *    uint32_t var1 = 100;
 *    bool var2 = false;
 *    debugPrint( "MY TAG", DebugPrint::USER1_CLASS ) << "Var1: " << var1 << " Var2: " << var2 << ", did you understand?\n";
 * @endcode
 *
 * And the output will be something like:
 * <pre>
 * MY TAG -> Var1: 100 Var2: false, did you understand?
 * </pre>
 *
 * @note Following C++ standard library "cout" style, no end-of-line is printed automatically, you must include it when you need it.
 *
 * The DebugPrint has two concepts you need to understand:
 * - TAGS: each print can be labelled with a "tag", you can specify a "default" tag, with a call to DebugPrint::setDefaultTag(), for all your calls, and you can omit it with debugPrintUntagged().
 * - CLASSES: you can selectively enable classes of output by setting environment variables, and prints will be enabled or disabled automatically based on their classes.
 *
 * You must use one of the following functions:
 * * debutPrint()
 * - debugPrintNotice();
 * - debugPrintCritical();
 * - debugPrintWarning();
 *
 * And, as stated above, you can also use this method to avoid printing the tag:
 * - debugPrintUntagged();
 *
 * For example:
 * @code
 * debugPrintNotice( "testTag" ) << "this is a test\n";
 *      ---> output: "testTag -> this is a test"
 * debugPrintUntagged() << "another test\n";
 *      ---> output: "another test"
 * @endcode
 * This last function is useful when you need to print a formatted string without a new-line, to compose multiple strings on screen
 * without adding the tag each time.
 *
 * What will actually be printed on screen (or to file, see below), is determined by the combination of selected "classes" (not class as in C++ or OOP!).
 *
 * The default output classes are:
 * - ERROR_CLASS which are always printed and should be used for all critical errors (will be printed on stderr)
 * - WARNING_CLASS used for generic warnings, which needs to be printed out by default.
 * - NOTICE_CLASS used for normal messages, which needs to be printed out by default.
 *
 * The following output classes are predefined but are not enabled by default:
 * - THREAD_CLASS turn on to enable some extra thread-related output
 * - MEMORYCHECKER_CLASS used for memory checker notices
 * - LICENSE_CLASS used for license checks notices
 * - COMMS_CLASS used for BaseCommInterface / Generic devices (TCP/UDP etc) notices
 * - PLUGIN_CLASS used for dynamic loader (DLL/SO) and plugins notices
 * - COMPRESS_CLASS used for compression / decompression notices
 * - MEMORYAREA_CLASS used for memory areas notices
 * - CONFIG_CLASS used for config file notices
 * - SIGNAL_CLASS used for signals notices
 * - PROCESS_CLASS used for process control notices
 * - SCALER_CLASS used for scalers notices
 * - DATACHANNEL_CLASS used for the Ncfs2 Data Channel notices
 * - TIMER_CLASS used for additional timing notices
 * - IO_CLASS used for additional I/O interface devices notices
 * - LEGACY_CLASS used for specific Legacy Wrapper notices
 * - MUTEX_CLASS used fot mutex lock/unlock notices
 *
 * The following classes are available for you and are not used inside the Simulation Framework anywhere:
 * - USER1_CLASS
 * - USER2_CLASS
 * - USER3_CLASS
 * - USER4_CLASS
 *
 * You can add a description to each one of these levels by calling registerUserType() BEFORE any debugPrint() method is actually called.
 *
 * To enable one of these classes, you must define a specific environment variable called FRAMEWORK_DEBUG_[selected class]. For example, to enable threads
 * debug output you need to set: <pre>FRAMEWORK_DEBUG_THREAD=1</pre> environment variable. What value you set is not important, you just need to set to a non 0 value
 * to enable the class and either unset it or set to 0 to disable it.
 *
 * By default, WARNING_CLASS and NOTICE_CLASS are always enabled.  If you need to disable them, set:
 * - FRAMEWORK_DEBUG_WARNING=0
 * - FRAMEWORK_DEBUG_NOTICE=0
 *
 * Instead, ERROR_CLASS cannot be disabled.
 *
 * Output is normally cached and flushed according to stdout and stderr settings. So you usually want to print a newline "\n" character
 * at the end of each line.
 * If you need to force flushing, call:
 * - debugPrintFlush()
 *
 * which will flush the ativated classes.
 *
 * You can enable disk output by setting the FRAMEWORK_DEBUG_FILE environment variable, anythign print on screen will also be saved to disk.
 *
 * Adding your own types
 * =====================
 *
 * You can chain all the basic types. If you want to add support for your own
 * type, you need to implement your own operator, like:
 * @code
 * class MyClass:
 * public:
 *     DebugPrint& operator<<(DebugPrint&, MyClass );
 * @endcode
 *
 * Output to disk
 * ==============
 *
 * You can force write all the output to a file on disk, just set the FRAMEWORK_DEBUG_FILE environment variable
 * to a file name. Output will be appended to that file.
 *
 */
class DLLEXPORT DebugPrint
{
public:
    /** @brief Debug print classes
     */
    enum DebugClass { ERROR_CLASS =    0x00000000, /**< Always printed */
                      // Byte 0
                      WARNING_CLASS =           0x00000001, /**< Use for warnings */
                      NOTICE_CLASS =            0x00000002, /**< use for common notices */
                      THREAD_CLASS =            0x00000004, /**< enable thread specific notices */
                      MEMORYCHECKER_CLASS =     0x00000008, /**< enable memory checker notixes */
                      LICENSE_CLASS =           0x00000010, /**< enable license-check notices */
                      COMMS_CLASS =             0x00000020, /**< enable interface/comms notices */
                      PLUGIN_CLASS =            0x00000040, /**< enable dynamic loader (and plugins) notices */
                      COMPRESS_CLASS =          0x00000080, /**< enable compress/decompress notices */

                      // Byte 1
                      MEMORYAREA_CLASS =        0x00000100, /**< enable memory area notices */
                      CONFIG_CLASS =            0x00000200, /**< enable config file notices */
                      SIGNAL_CLASS =            0x00000400, /**< enable signal and sighandler notices */
                      PROCESS_CLASS =           0x00000800, /**< enable process control notices */
                      SCALER_CLASS =            0x00001000, /**< enable scalers notices */
                      DATACHANNEL_CLASS =       0x00002000, /**< enable Ncfs2 Data Channel notices */
                      TIMER_CLASS =             0x00004000, /**< enable additional timing notices */
                      IO_CLASS =                0x00008000, /**< enable additional I/O interface notices */

                      // Byte 2
                      LEGACY_CLASS =            0x00010000, /**< enable specific Legacy Wrapper notices */
                      MUTEX_CLASS =             0x00020000, /**< enable mutex specific notices */

                      // Byte 3
                      USER1_CLASS  =            0x10000000, /**< User-defined level 1 */
                      USER2_CLASS  =            0x20000000, /**< User-defined level 2 */
                      USER3_CLASS  =            0x40000000, /**< User-defined level 3 */
                      USER4_CLASS  =            0x80000000 /**< User-defined level 4 */
                    };

    /** @brief Get the environment variables corresponding to the debug classes
     * @param debug_classes classes to convert to env variables
     * @return map of strings, ready to be set as environment variables
     * @since 4.0.2
     */
    static std::map<std::string, std::string> getEnvironmentClass( const std::map<DebugClass, bool>& debug_classes );

    /** @brief Get the current active output classes
     * @return map of classes, ready to be set as environment variables
     * @since 4.0.2
     */
    static std::map<DebugClass, bool> getClasses();

    /** @brief Describe a class in human reabable string
     * @since 4.0.2
     * @param debug_class the class to describe
     * @return the description of the class
     */
    static std::string describeClass( DebugClass debug_class );

    /** @brief Describe a user-class
     *
     * Add a description to a user-defined class, it will be printed at the beginning so that your users will
     * know what to set and what they will get.
     * @param user_class the class to describe (one of USERX_CLASS)
     * @param description the description
     */
    static void registerUserType( DebugClass user_class, const std::string& description );

    /** @brief Set the default tag to be printed
     * @param str the tag to be used
     */
    static void setDefaultTag( const std::string& str );

    virtual ~DebugPrint();

    /** @brief print a C++ standard string
     * @param data the data to print
     * @return reference to itself to chaining operators
     */
    DebugPrint& operator<<(const std::string& data);

    /** @brief print a float
     * @param data the data to print
     * @return reference to itself to chaining operators
     */
    DebugPrint& operator<<(float data);

    /** @brief print a double
     * @param data the data to print
     * @return reference to itself to chaining operators
     */
    DebugPrint& operator<<(double data);

    /** @brief print a uint64
     * @param data the data to print
     * @return reference to itself to chaining operators
     */
    DebugPrint& operator<<(uint64_t data);

    /** @brief print a uint32
     * @param data the data to print
     * @return reference to itself to chaining operators
     */
    DebugPrint& operator<<(uint32_t data);

    /** @brief print a int16
     * @param data the data to print
     * @return reference to itself to chaining operators
     */
    DebugPrint& operator<<(uint16_t data);

    /** @brief print a uint8
     * @param data the data to print
     * @return reference to itself to chaining operators
     */
    DebugPrint& operator<<(uint8_t data);

    /** @brief print a int64
     * @param data the data to print
     * @return reference to itself to chaining operators
     */
    DebugPrint& operator<<(int64_t data);

    /** @brief print a int32
     * @param data the data to print
     * @return reference to itself to chaining operators
     */
    DebugPrint& operator<<(int32_t data);

    /** @brief print a int16
     * @param data the data to print
     * @return reference to itself to chaining operators
     */
    DebugPrint& operator<<(int16_t data);

    /** @brief print a int8
     * @param data the data to print
     * @return reference to itself to chaining operators
     */
    DebugPrint& operator<<(int8_t data);

    /** @brief print a generic memory address
     * @param ptr the memory address
     * @return reference to itself to chaining operators
     */
    DebugPrint& operator<<(void* ptr);

    /** @brief print a C string (null-terminated)
     * @param data the data to print
     * @return reference to itself to chaining operators
     */
    DebugPrint& operator<<(char* data);

    /** @brief print a C string (null terminated)
     * @param data the data to print
     * @return reference to itself to chaining operators
     */
    DebugPrint& operator<<(const char* data);

    /** @brief print a character
     * @param data the data to print
     * @return reference to itself to chaining operators
     */
    DebugPrint& operator<<(char data);

    /** @brief std::string operator to concatenate to a string in return
     * @since 4.0.2
     *
     * This allow to write:
     * std::string string = debugPrintNotice() << "Hello world\n";
     * and have "Hello world" string copied in string.
     *
     * @return the string which has been or would have been printed
     */
    operator std::string() const;

private:
    DebugPrint( const std::string& tag, DebugClass debug_class);
    DebugPrint( DebugClass debug_class);
    class __privateDebugPrint;
    __privateDebugPrint* _private;

    static std::string getDefaultTag();

private:
    friend DLLEXPORT DebugPrint debugPrint( const std::string& tag, DebugClass debug_class );
    friend DLLEXPORT DebugPrint debugPrint( DebugClass debug_class );
    friend DLLEXPORT DebugPrint debugPrintNotice( const std::string& tag );
    friend DLLEXPORT DebugPrint debugPrintNotice();
    friend DLLEXPORT DebugPrint debugPrintError( const std::string& tag );
    friend DLLEXPORT DebugPrint debugPrintError();
    friend DLLEXPORT DebugPrint debugPrintWarning( const std::string& tag );
    friend DLLEXPORT DebugPrint debugPrintWarning();
    friend DLLEXPORT DebugPrint debugPrintUntagged( DebugClass level );
};

/** @brief print a formatted string
 * @param tag a descriptive string which will be printed before the actual output data
 * @param debug_class class of printouts this message belongs to
 * @return a DebugPrint object which can be used to chain the debug output
 */
DLLEXPORT inline DebugPrint debugPrint( const std::string& tag, DebugPrint::DebugClass debug_class = DebugPrint::NOTICE_CLASS )
{
    return DebugPrint( tag, debug_class );
}

/** @brief print a formatted string with a default tag
 * @since 4.0.2
 * @param debug_class class of printouts this message belongs to
 * @return a DebugPrint object which can be used to chain the debug output
 */
DLLEXPORT inline DebugPrint debugPrint( DebugPrint::DebugClass debug_class = DebugPrint::NOTICE_CLASS )
{
    return DebugPrint( DebugPrint::getDefaultTag(), debug_class );
}

/** @brief Print a formatted string without the prepended "tag"
 * @since 4.0.2 renamed from debugPrintSimple()
 * @param debug_class class of printouts this message belongs to
 * @return a DebugPrint object which can be used to chain the debug output
 */
DLLEXPORT inline DebugPrint debugPrintUntagged( DebugPrint::DebugClass debug_class = DebugPrint::NOTICE_CLASS )
{
    return DebugPrint( debug_class );
}

/** @since 3.3
 * @brief print a string with DebugPrint::NOTICE_CLASS
 * @param tag a descriptive string which will be printed before the actual output data
 * @return a DebugPrint object which can be used to chain the debug output
 */
DLLEXPORT inline DebugPrint debugPrintNotice( const std::string& tag )
{
    return DebugPrint( tag, DebugPrint::NOTICE_CLASS );
}

/** @since 4.0.2
 * @brief print a string with DebugPrint::NOTICE_CLASS and the default tag
 * @return a DebugPrint object which can be used to chain the debug output
 */
DLLEXPORT inline DebugPrint debugPrintNotice()
{
    return DebugPrint( DebugPrint::getDefaultTag(), DebugPrint::NOTICE_CLASS );
}

/** @since 3.3
 * @brief print a string with DebugPrint::ERROR_CLASS
 * @param tag a descriptive string which will be printed before the actual output data
 * @return a DebugPrint object which can be used to chain the debug output
 */
DLLEXPORT inline DebugPrint debugPrintError( const std::string& tag )
{
    return DebugPrint( tag, DebugPrint::ERROR_CLASS );
}

/** @since 4.0.2
 * @brief print a string with DebugPrint::ERROR_CLASS and a default tag
 * @return a DebugPrint object which can be used to chain the debug output
 */
DLLEXPORT inline DebugPrint debugPrintError()
{
    return DebugPrint( DebugPrint::getDefaultTag(), DebugPrint::ERROR_CLASS );
}

/** @since 3.3
 * @brief print a string with DebugPrint::WARNING_CLASS
 * @param tag a descriptive string which will be printed before the actual output data
 * @return a DebugPrint object which can be used to chain the debug output
 */
DLLEXPORT inline DebugPrint debugPrintWarning( const std::string& tag)
{
    return DebugPrint( tag, DebugPrint::WARNING_CLASS );
}

/** @since 4.0.2
 * @brief print a string with DebugPrint::WARNING_CLASS with default tag
 * @return a DebugPrint object which can be used to chain the debug output
 */
DLLEXPORT inline DebugPrint debugPrintWarning( )
{
    return DebugPrint(DebugPrint::getDefaultTag(), DebugPrint::WARNING_CLASS );
}

/** @since 3.4
 * @brief force a flush on the output
 * @param debug_class class to flush (NOTICE class by default)
 */
DLLEXPORT void debugPrintFlush( DebugPrint::DebugClass debug_class = DebugPrint::NOTICE_CLASS );

/** @brief Put this define to automatically add a run-time print to deprecated code
 */
#define DEPRECATED FrameworkLibrary::debugPrintError( "DEPRECATED" ) << "Function " << ____pretty_func_def__ << " is DEPRECATED (at: " << __FILE__ << ":" << __LINE__ << ")\n"

/**
  @}
  */

}

#endif // DEBUGPRINT_H
