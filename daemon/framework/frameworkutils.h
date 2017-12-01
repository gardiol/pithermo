#ifndef FRAMEWORKUTILS_H
#define FRAMEWORKUTILS_H

#include <common_defs.h>

#include <string>
#include <vector>
#include <list>
#include <set>

namespace FrameworkLibrary {

/** @brief Some miscellaneous utilities which can be handy and speed up your coding
 *
 * For more details, please refer to @ref programming_support_1
 *
 * Feel free to use these multiplatform tools!
 *
 * @review Implement requirements
 *
 */
class DLLEXPORT FrameworkUtils
{
public:
    /** @brief Get the current OS architecture.
     * @note this is an hardcoded value at compile time.
     *
     * Valid values, at this time are:
     * - linux32
     * - linux64
     * - win32
     * - win64
     * @return architecture
     *
     */
    static std::string getOsArch();

    /** @brief get Process ID (PID)
     * @since 4.0.3
     * @return  PID as a signed integer on 32 bit
     */
    static int32_t getPID();

    /** @brief get Parent Process ID (PPID)
     * @note On windows there is no "safe" parent-child relationship, so this call will always return -1.
     * @since 4.0.3
     * @return  PPID as a signed integer on 32 bit
     */
    static int32_t getParentPID();

    /** @brief Get the current OS predefined path separator
      * @return path separator
    */
    static std::string getDirSep();

    /** @brief Return the current working directory
     * @return the CWD
     */
    static std::string getCwd();

    /** @brief Return the current process full path executable
     *
     * This will return the full path and executable name.
     * For example, if you are running notepad on windows, it should return "c:\windows\notepad.exe"
     * or if you are running bash on linux, it should return "/bin/bash".
     * @return full path and executable name of the current process
     */
    static std::string getExecutablePath();

    /** @brief Read an environment variable
     * @param str Environment variable to read
     * @return Value from environment (will return "" if not set)
     */
    static std::string read_env( const std::string& str );

    /** @brief Check if an environment variable exist
     * @since 4.0.0
     * @note on some platforms, there is no difference between an empty variable and an unset variable.
     * @param str Environment variable to test for existence
     * @return true if it is set, false if it is not set.
     */
    static bool check_env( const std::string& str );

    /** @brief Replace ALL environment varilables in a string (format: %env_var%)
     * @param str the source
     * @return the source with all environment variables replaced
     */
    static std::string env_replace( const std::string& str );


    /** @brief Type of human-readable printout, to be used with human_readable_number() method.
     */
    enum human_readable_type { DECIMAL_TYPE,  /**< Decimal type (1K = 1000) */
                               BINARY_TYPE }; /**< Binary type (1K = 1024) */

    /** @brief Convert a number to a human-readable string (b, kb, etc)
     *
     * For example, if you have 3267000, this will be printed if:
     * - unit = Byte, type = BINARY_TYPE, level = 1: 3.190KBytes
     * - unit = Hz, type = DECIMAL_TYPE, level = 0: 3.267.000Hz
     * - unit = M/s, type = DECIMAL_TYPE, level = 2: 3Mm/s
     * - unit = M/s, type = DECIMAL_TYPE, level = 1: 3.267Km/s
     * - unit = M/s, type = DECIMAL_TYPE, level = 0: 3.267.000m/s
     *
     * If you specicy "-1" as level, the function will determine the best level to show the most
     * significative part of the number: it will select a level which will result in a integer part
     * which is between 0 and 1000 (1024).
     *
     * @param number input number, in bytes
     * @param unit Specify "Hz", "B", or whatever you want to append to "K", "M", and so on
     * @param type divide by 1000 or by 1024
     * @param level Maximum level (0 = no split, 1 = k, 2 = m, 3 = g, 4 = t, 5 = p) to use for truncating. Use -1 for automatic detection.
     * @param print_decimals if true, print also fractions (ex: 1,5Kbytes if number is 1500 and level is 1)
     * @return string human-formatted
     */
    static std::string human_readable_number(int64_t number,
                                             const std::string& unit,
                                             bool print_decimals = true,
                                             human_readable_type type = DECIMAL_TYPE,
                                             int8_t level = -1);

    /** @brief Convert a number of seconds to a human-readable string (HH:MM:SS)
     *
     * For example:
     * - 3675 will be printed as: "01:01:15"
     * - 115830 will be printed as: "1d 12:10:30"
     * - 1389722 will be printed as: "2w 2d 02:02:02"
     *
     * If you also specify milliseconds, they will be appended on the right with the "ms" suffix.
     *
     * @param number_seconds number of seconds to convert
     * @param milliseconds optional number of milliseconds.
     * @return string human-formatted
     */
    static std::string human_readable_time(int64_t number_seconds, uint64_t milliseconds = 0 );

    /** @brief Write an environment variable
     * @param str Environment variable to write
     * @param val value to set
     */
    static void write_env( const std::string& str, const std::string& val );

    /** @brief Unset an environment variable
     * @note this is different from writing a value of "", this will remove the variable form the environment.
     * @param str Environment variable to unset
     */
    static void reset_env( const std::string& str );

    /** @brief Get the simulation framework user-id
     *
     * @warning If no USER-ID can be obtained or if the process is started as root on linux, IT WILL TERMINATE IMMEDIATELY with exit().
     * Will return first the FRAMEWORK_USER_ID environment variable. If not set only on Linux will return the User-ID of the system.
     * @return simulation framework unique id
     */
    static int simulationSessionId();

    /** @brief return the username for this session
     *
     * Will return the OS level username running the calling program
     * @return username
     */
    static std::string getCurrentUsername();

    /** @brief Get the simulation framework user-id as a string
     *
     * @warning If no USER-ID can be obtained or if the process is started as root on linux, IT WILL TERMINATE IMMEDIATELY with exit().
     * Will return first the FRAMEWORK_USER_ID environment variable. If not set only on Linux will return the User-ID of the system.
     * @return simulation framework unique id as a string
     */
    static std::string simulationSessionIdStr();

    /** @brief It's like doing: printf("%s: %s\n", msg, get_errno_string())
     * @param msg string to print
     * @param extra another string to print
     */
    static void print_errno_string( const std::string& msg, const std::string& extra = "" );

    /** @brief strerror thread safe multipiattaforma
     * @param extra stringa da aggiungere in coda tra ()
     * @return la stringa di errore
     */
    static std::string get_errno_string(const std::string& extra);

    /** @brief List files and folders inside a folder. Not recursive.
      * @param path directory to list
      * @param list_folders if false, will list only files. If true, also folders.
      * @return items contained in the folder. Will not return "." and "..".
      */
    static std::set<std::string> listFolder( const std::string& path, bool list_folders = false );

    /** @brief Get the real path for the given path
     *
     * On Linux, it will resolve symbolic links and return the real path. If path is not a symlink,
     * then the input parameter is returned.
     * On windows it will return the input parameter.
     * @param path Path to resolve
     * @return resolved path
     */
    static std::string resolvePath( const std::string& path );

    /** @brief Test if a file or folder exist
     *
     * @param filename file or folder to test for existence
     * @param check_folder will return true if exist AND it's a folder
     * @return true if it exist, false otherwise
     */
    static bool fileExist( const std::string& filename, bool check_folder = false );

    /** @brief Delete a file
     *
     * @param filename file to delete
     * @return Will return false if the file does not exist or cannot be deleted.
     */
    static bool deleteFile( const std::string& filename );

    /** @brief Create a folder
     *
     * @param filename name of folder to create
     * @return true if ok, false otherwise
     */
    static bool mkDir( const std::string& filename );

    /** @brief Remove a folder
     *
     * @param filename name of folder to remove
     * @return true if ok, false otherwise
     */
    static bool rmDir( const std::string& filename );

    /** @brief List all available MAC addresses on the system
      * @return list of all MACs
      */
    static std::set<std::string> list_MACs();

    /** @brief List all available IP addresses on the system
     *
     * @return list of all IPs
     */
    static std::set<std::string> list_IPs();

    /** @brief remove leading and trailing blanks from a string (tabs, spaces...)
      */
    static void string_trim( std::string& );

    /** @brief remove leading and trailing blanks from a string (tabs, spaces...)
     * @since 4.0.0
     * @param str the string to trim
     * @return the trimmed string
     *
     * This method is slightly slower, since it works on a copy of the string.
      */
    static std::string string_trim(const std::string &str );

    /** @brief Convert a string to lowercase
      */
    static std::string string_tolower(const std::string & );

    /** @brief Convert a string to uppercase
      */
    static std::string string_toupper(const std::string & );

    /** @brief Convert a string to an float
     * @since 4.0
     * @param str the string
     * @return the float
     */
    static double string_tof( const std::string& str );

    /** @brief Convert a string to an itneger
     * @param str the string
     * @return the integer
     */
    static int string_toi( const std::string& str );

    /** @brief Split a string according to a marker (tokenize a string)
      * @param str string to be cut
      * @param sep the marker
      * @return vector of tokenized strings
      */
    static std::vector<std::string> string_split( const std::string& str, const std::string& sep );

    /** @brief Shift a vector.
     *
     * Return a new vector with the first pos elements dropped. The new vector index starts from 0.
     * @param vector the vector to shift
     * @param pos now many elements to drop from the start
     * @return the shifted vector
     */
    static std::vector<std::string> vector_shift( const std::vector<std::string>& vector, uint32_t pos = 1 );

    /** @brief Split a string according to a marker (tokenize a string)
     * @since 4.0.0 added list return version
      * @param str string to be cut
      * @param sep the marker
      * @return list of tokenized strings
      */
    static std::list<std::string> string_split_list( const std::string& str, const std::string& sep );

    /** @brief Split a string according to a marker (tokenize a string)
     * @since 4.0.3 added set return version
      * @param str string to be cut
      * @param sep the marker
      * @return set of tokenized strings
      */
    static std::set<std::string> string_split_set( const std::string& str, const std::string& sep );

    /** @brief Parse a string (row) as a CSV
     * @since 4.0.0 the "sep" parameter is now a std::string. Only the first character will be used anyway.
     *
     * This works more or less like string_split but obeys escaping the separator like: aaa,bbb,"this is, one, field!",ccc
     * You can enclose a field between "" or prepend any separator with \, to ignore it.
     * The \ character will always be printed unless it is followed by the separator or by the " character.
     * If you need to print \" or \, exactly like this, you need to write it like: \\" or \\,
     * If you want to print an " character in your string, ALWAYS escape it like: \" or you will be in trouble!
     *
      * @param row the row to parse
      * @param sep the separator (note that only the FIRST character will be used)
      * @return tokenized CSV fields
      */
    static std::vector<std::string> parse_csv(const std::string& row, const std::string &sep );

    /** @brief Tokenize a string by spaces, keeping quotes.
     * @param str string to split
     * @return splitted items
     */
    static std::vector<std::string> string_split_quotes(const std::string& str);

    /** @brief Replace part of a string with nother one
      * @param src string on which to perform the substitution
      * @param token the substring to replace
      * @param value the replacement string
      * @param only_one if true quit after first find
      * @return source string with replacements in place
      */
    static std::string string_replace(const std::string& src, const std::string& token, const std::string& value , bool only_one = false);

    /** @brief Given an hostname, resolve it's IP address
     * @param hostname name of the host
     * @return IP, in string format, or "" if unable to resolv
     */
    static std::string resolve_hostname( const std::string& hostname );

    /** @brief Read a string from a binary file
     *
     * This will read a string saved with write_string() from a binary file
     * @param file the file to read from
     * @return the string read
     */
    static std::string read_string( FILE* file );

    /** @brief Write a string to a binary file
     *
     * This will write a string to a binary file. This string can be read with read_string().
     * It's saved as:
     * @code
     * uint32_t str_len;
     * char str[ str_len ];
     * @endcode
     * String terminator is not stored.
     * @param file the file to write to
     * @param str the string to save
     */
    static void write_string( FILE* file, const std::string& str );

    /** @brief check if a character is blank (space, tab, etc)
     *
     * This is here because some compilers (Visual Studio, i am looking at you!) does not implement the C++11 isblank()
     * @param c the character to test
     * @return true if it's a blank
     */
    static bool isBlank( char c );

    /** @brief rounds a to the nearest integer value in floating-point
     *
     * This is here because some compilers (Visual Studio, i am looking at you!) does not implement the C++11 rint()
     * @param a the number to be rounded
     * @return the nearest integer
     */
    static double rint( double a );

    /** @brief Load an entire text file into a string
     * @param name name of file
     * @param str destination string
     * @return true if read ok, false otherwise.
     */
    static bool file_to_str( std::string name, std::string& str );

    /** @brief Write an entire string into a text file
     * @warning if it exist, file will be replaced by the new string
     * @param name name of file
     * @param str source string
     * @return true if write ok, false otherwise.
     */
    static bool str_to_file( std::string name, std::string str );

    /** @brief get local hostname
     * @return hostname
     */
    static std::string getHostname();

    /** @brief Convert an integer to string
     * @param t integer to convert
     * @return converted string
     */
    static std::string tostring( const int64_t t);

    /** @brief Convert a double to string
     * @param t float to convert
     * @return converted string
     */
    static std::string ftostring( const double t);

    /** @brief Change working directory
     *
     * @param path directory name to change
     * @return Will return false if the directory does not exist or cannot be accessed.
     */
    static bool chDir( const std::string& path );


private:
    FrameworkUtils();
};

/** @brief Calculate minimum
 * @param a first operand
 * @param b second operand
 * @return minimum between first and second
 */
template<typename T>
DLLEXPORT T FrameworkUtils_min( T a, T b )
{
    return a < b ? a : b;
}

/** @brief Calculate maximum
 * @param a first operand
 * @param b second operand
 * @return maximum between first and second
 */
template<typename T>
DLLEXPORT T FrameworkUtils_max( T a, T b )
{
    return a > b ? a : b;
}

}

#endif // FRAMEWORKUTILS_H
