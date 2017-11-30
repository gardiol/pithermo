#ifndef CMDLINEPARSER_H
#define CMDLINEPARSER_H

#include <common_defs.h>

#include <string>
#include <map>
#include <list>

namespace FrameworkLibrary {

class CmdlineParser;
class __private_CmdlineParser;
class __private_CmdlineParameter;

/** @brief Define command-line parameters to be used with CmdlineParser class
 * @since 3.4
 *
 * This class is used to define a parameter for parsing the command-line.
 *
 * A "parameter" is a command-line argument which can have one "short" version and any number of "options".
 *
 * For example, these are "parameters":
 * @code
 *      --param (optionless parameter)
 *      -p (short version)
 *      --param=option (one option parameter)
 *      -p option (one option, short version)
 *      --param=op1,op2,op3 (multi-option parameter)
 *       -p op1,op2,op3 (multi-option, short version)
 * @endcode
 *
 * A parameter can be mandatory, in this case it MUST be specified by the user or CmdlineParser will give an error.
 * A parameter can have a minimum and a maximum number of options. the minimum number can be 0 (no options), while the
 * maximum number can match the minimum number (fixed number of options) or be bigger (additional "optional" options).
 *
 * You can specify "default values" for the options. If the parameter is "mandatory" it's default values will be used if the
 * user does not specify the parameter on the command-line.
 *
 * This class will also contain the parsed command-line parameter after it has been processed by the CmdlineParser::parse()
 * method. Please keep in mind the isntances of CmdLineParameter which contains the results of the command-line parsing are
 * not the same you passed to CmdlineParser::defineParameter(), but new copies.
 *
 * This class is built to be used on the stack, you should NOT create it on the heap.
 *
 * Defining a parameter
 * ====================
 *
 * It's intended usage is by chained-calls, like this:
 * @code
 *  CmdlineParser parser(...);
 *
 * parser.defineParameter( CmdlineParameter( "param", "my param description" )
 *                         .setShortName( "p" )
 *                         .setOptions(2)
 *                         .setOptionDefault("opt1", 0)
 *                         .setOptionDefault("opt2", 1) );
 * @endcode
 *
 * This allow for a more compact coding approach.
 *
 * Reading parsed parameter
 * ========================
 *
 * Use getNumOptions() to know how many options have been specified, then use getOption() to read each option.
 * This will work only after CmdlineParser::parse() has been called, and only for items returned by CmdlineParser::getParameter().
 *
 * Help support
 * ============
 *
 * It is important to specify meaningful description for the parameter and comments for the options, so that a proper help screen
 * can be automatically created.
 *
 * - "description" will be used in the summary of all the parameters
 * - "option help name" will be used in the examples and in the summary.
 *
 * For example:
 * @code
 *Example usages:
 *  ./bin/myBin -p <option_help> | --param=<option_help>
 *
 *  Summary of all available commands:
 *	-p <option_help> | --param=<option-help>: description of parameter
 * @endcode
 *
 * If you specify a "default value" for an option, this will be used instead of the "option help" string in the help screen.
 *
 * 
 *
 */
class DLLEXPORT CmdLineParameter
{
    friend class CmdlineParser;

public:
    /** @brief Accept the parameter once or more than once
     */
    enum MultiMode { single,  /**< Accept this parameter only once on the command line */
                     multiple /**< Accept this parameter more than once on the command line */
                   };

    /** @brief Create a new valid parameter
     * @since added "multiple" parameter in 4.0.0
     * @param name the parameter name (es: "param") without the leading delimiter
     * @param description description of the parameter for the help screen
     * @param multiple if true, the parameter can be present more than once on the command line.
     * @param mandatory if true, this parameter will be required on the command line, or the parsing will fail
     *
     * 
     * 
     *
     */
    CmdLineParameter(const std::string& name,
                     const std::string& description,
                     MultiMode multiple,
                     bool mandatory = false);

    /** @brief create an invalid parameter (internal usage only)
     *
     *
     *
     */
    CmdLineParameter();

    /** @brief Copy-contructor for usage on the stack
     * @param other the other object
     *
     * 
     *
     */
    CmdLineParameter( const CmdLineParameter& other );

    virtual ~CmdLineParameter();

    /** @brief set a short-name equivalent for this parameter
     * @param short_name the short name
     * @return chain-object to chain-call this method
     *
     * 
     *
     */
    CmdLineParameter& setShortName( const std::string& short_name );

    /** @brief set the number of options
     * @param num_min_options minimum number of options (0 for no options)
     * @param num_max_options maximum number of options (-1 is equal to num_min_options)
     * @return chain-object to chain-call this method
     *
     * 
     *
     */
    CmdLineParameter& setOptions( uint32_t num_min_options,
                                  int32_t num_max_options = -1 );

    /** @brief set a default value to an option
     *
     * If you set this, the option help you set calling setOptionHelp() for this option will be ignored.
     * @param default_value default value for this option
     * @param which_option which option to associate the value to
     * @return chain-object to chain-call this method
     *
     * 
     *
     */
    CmdLineParameter& setOptionDefault(const std::string& default_value,
                                       uint32_t which_option = 0 );

    /** @brief set an help string for an option
     *
     * This will be overriden by the default value set with setOptionDefault(), if you set it.
     * @param help_name string to use on the help screen
     * @param which_option which option to associate the value to
     * @return chain-object to chain-call this method
     *
     * 
     *
     */
    CmdLineParameter& setOptionHelp( const std::string& help_name,
                                     uint32_t which_option = 0 );

    /** @brief Check if this parameter is valid
     *
     * You should not care about this method, it's mostly for internal use.
     * @return true if the parameter is a valid parameter.
     */
    bool isValid() const;

    /** @brief Number of parsed options
     * @note This method returns valid values only after CmdlineParser::parse() has returned.
     * @return number of parsed parameters (this includes any pre-existing default values)
     *
     * 
     *
     */
    uint32_t getNumOptions() const;

    /** @brief Get value of option
     * @note This method returns valid values only after CmdlineParser::parse() has returned.
     * @param which_option which option you want to get (0, the first one, by default)
     * @return the option value. This will return the default value if no value has been defined on the command line.
     *
     * 
     *
     */
    std::string getOption( uint32_t which_option = 0 ) const;

    /** @brief Assignment operator, for stack usage.
     * @param other other parameter
     * @return reference for chaining
     *
     * 
     *
     */
    CmdLineParameter& operator=( const CmdLineParameter& other );

    /** @brief Get parameter name
     * @since 4.0.0
     * @return name of the parameter
     */
    std::string getName() const;

private:
    __private_CmdlineParameter* _private;
};

/** @brief Parse command-line style arguments
 * @since 3.4
 *
 * This class can parse command-line arguments in a safe and multiplatform way.
 * This class can also print, automatically, the help screen for your program based
 * on all the parameters and options you defines.
 *
 * You can define a list of parameters by calling defineParameter(), then you assign the
 * command-line with setParameters() and do the actual parsing with parse().
 *
 * Please always check for parse() return value! If parsed has failed, you most probably want to
 * abort your program before doing anything else. In this case, the help screen will be printed automatically.
 *
 * @note If the user specify the --help or -h parameter, the help screen will be printed and parse() will return false as well.
 * @note the --help / -h parameter is automatically defined.
 * @warning You CANNOT reuse the --help or -h names!
 *
 * You can then get the results by calling getParameter() and getExtras().
 * The simpler hasParameter() can be useful to check if a parameter without options has been specified.
 *
 * If you need to read the executable name (argv[0]) you can also use getArgv0().
 *
 * If you want to be notified of parameters while they are decoded, you must derive your child class
 * from CmdlineParser and implement the two virtual methods CmdlineParser::customParsedParameter() and CmdlineParser::customParsedExtra().
 *
 * If you want to have at least ONE parameter specified, but you don't want to make one specific parameter
 * mandatory, you can just call:
 * - enableOneParameterMandatory()
 * in this way, parse() will fail if the user does not specify at least one parameter, and help will be printed.
 *
 * Parameters format
 * =================
 *
 * By default, parameters can have the following format:
 * @code
 * --param=option       : a long-named param with a mandatory option
 * --param              : a long-named param without option
 * -p option            : a short-named param with a mandatory option
 * -p                   : a short-named param without option
 * --par=value1,value2  : a multi-option param (long)
 * --pr value1,value2   : a multi-option param (short)
 * @endcode
 *
 * For each parameter you can specify both a "long" and a "short" version. The user can use both versions
 * transparently, the parameter will be recognized. Note that the parsed parameter will always return the long
 * version, it is not possible to know if the user has selected the short version.
 *
 * The default delimiters (--, -, = and {space}) can be changed by calling setParameterDelimiter(),
 * setShortDelimiter(), setOptionDelimiter() and setShortOptionDelimiter(). If you plan to change the
 * default delimiters, please do so BEFORE calling parse().
 *
 * Use getParameter() to get a valid parsed parameter.
 * Use hasParamater() to check if a parameter has been specified.
 * Use getExtras() to get the list of extra-parameters.
 *
 * Defining valid parameters
 * =========================
 *
 * Call setParameters() before you call parse(), one time for each parameter you want to define.
 * Each parameter can be mandatory or optional and can have one option.
 *
 * Usually the parsing will fail if "unknown" parameters are found. Sometimes, you might want to allow
 * those parameters, for example when passing the command line to Qt or some other toolkit. In this case,
 * you can disable this error checking by calling disableCheckUnknowns().
 *
 * Extra parameters
 * ================
 *
 * You can enable the identification and collection of extra parameters (anything on the command-line which
 * is not a parameter as defined in the previous section). If you do so, you can read them with getExtras()
 * which will return an ordered list of anything the user typed on the command line which does not match
 * a parameter.
 *
 * Note that "invalid" parameters are discarded and not returned as extras.
 *
 * To enable extras, call enableExtras().
 *
 * Help screen
 * ===========
 *
 * A default help screen will be composed and printed on screen automatically if:
 * - the user write the -h or --help parameter
 * - the command-line contains invalid parameters, missing options or extra-options (if not allowed)
 * - you call the printHelp() function
 *
 * (in the first two cases parse() will return false)
 *
 * The help screen is similar to:
 * @code
 *     <help title>
 *
 *     <argv[0]> [-p1 option|--param1=option] -p2|--param2 [more...]
 *
 *     --param1 : param1 description
 *     --param2 : param2 description
 *     more     : extra-help
 *
 * <Help note goes here>
 *
 * @endcode
 *
 * Help title, note and extra-help can be specified in the CmdlineParser::CmdlineParser() constructor.
 * The description of each parameter can be specified when calling defineParameter().
 *
 * Usage example
 * =============
 *
 * @include example_cpp_cmdparse.cpp
 *
 * 
 *
 */
class DLLEXPORT CmdlineParser
{
public:
    /** @brief initialize a command-line parser
     * @param help_title title of the help screen
     * @param help_note bottom text for the help screen
     *
     * 
     * 
     *
     */
    CmdlineParser( const std::string& help_title = "",
                   const std::string& help_note = "" );

    virtual ~CmdlineParser();

    /** @brief print the help screen
     *
     * 
     *
     */
    void printHelp();

    /** @brief Disable error on unknown parameter
     *
     * By default ( disable_check == false ) the parse() function will fail if an "unknown" parameter is
     * encountered. By calling this, you can disable this check.
     *
     * If disable_check == false, any parameter which has not been defined by defineParameter() will cause parse() to return false (default)
     * If disable_check == true, any parameter which has not been defined by defineParameter() will be ignored and discarded.
     * @param disable_check Whether you want to disable the unknown parameter check
     *
     * 
     *
     */
    void disableCheckUnknowns( bool disable_check = true );

    /** @brief Enable check for at least ONE parameter to be specified
     *
     * @todo add mandatory also for "extra"
     *
     * If no parameters are given, the help will be printed and parse() will return false.
     * @param mandatory if true, at least one parameter must be specified.
     *
     * 
     *
     */
    void enableOneParameterMandatory( bool mandatory );

public: // setup
    /** @brief Enable support for "extra" parameters
     *
     * Example:
     * @code
     * ./myExe --param1 --param2 extra1 extra2
     * @endcode
     * This will enable support for parsing extra2 and extra1 parameters.
     *
    * @param help_extra help text for the "extra" parameter on the help screen
    * @param help_extra_name word to use in the help screen instead of "more" when defining extras
     *
     * 
     * 
     *
    */
    void enableExtras( const std::string& help_extra,
                       const std::string& help_extra_name);

    /** @brief define a new valid parameter
     *
     * Call this method for each parameter you need to define as valid.
     * @param param Parameter data
     *
     * 
     *
     */
    void defineParameter(const CmdLineParameter &param );

    /** @brief load the command line
     * @since 4.0.0 renamed from setParameters.
     * @param argc the number of parameters on the command line
     * @param argv the command line as given by the main() function
     *
     * 
     *
     */
    void setCommandLine( uint32_t argc,
                         char** argv );

    /** @brief parse the command line and decode the valid parameters
     * @warning you must have called setParameters() first!
     * @note the help screen will be printed automatically if this method return false.
     * @return true if all parameters where parsed correctly, false in case of any error.
     *
     * 
     *
     */
    bool parse();

public: // access parsed data
    /** @brief return all the "extra" parameters decoded
     * @note call parse() first
     * @return list of extra parametes
     *
     * 
     *
     */
    std::list<std::string> getExtras() const;

    /** @brief get and remove from the parsed one parameter
     * @since 4.0.0
     * @note call parse() first
     *
     * This method will return (and remove from the object) one parsed parameter.
     * You can iterate on this untill getNumParameters() return 0.
     * @param param_name The name of the parameter, leave empty to get the next one.
     * @return the parameter as parsed from the command line, an invalid one if not existent.
     *
     * 
     *
     */
    CmdLineParameter consumeParameter( const std::string& param_name = "") const;

    /** @brief Check if a valid parameter has been specified
     * @note call parse() first
     * @warning The returned object is NOT the same object you passed on the call to defineParameter()!
     * @param param_name name of parameter to check for existence
     * @return true if this parameter has been parsed false otherwise (at least once)
     *
     * 
     *
     */
    bool hasParameter(const std::string& param_name ) const;

    /** @brief Get number of valid parameters
     * @return the number of parameters which have been decoded properly.
     *
     * 
     *
     */
    uint32_t getNumParameters() const;

    /** @brief get the argv 0
     * @return value of argv 0
     *
     * 
     *
     */
    std::string getArgv0() const;

public: // customize delimiters
    /** @brief Change the default delimiter for a parameter
     *
     * Default is "--" (like in --help).
     * @param del new characters to be used as delimiters
     *
     * 
     *
     */
    void setParameterDelimiter( const std::string& del );

    /** @brief Change the default delimiter for a short-name parameter
     *
     * Default is "-" (like in -h).
     * @param del new characters to be used as delimiters
     *
     * 
     *
     */
    void setShortDelimiter( const std::string& del );

    /** @brief Change the default delimiter for an option to a parameter
     *
     * Default is "=" (like in --param=option).
     * @param del new characters to be used as delimiters
     *
     * 
     *
     */
    void setOptionDelimiter( const std::string& del );

    /** @brief Change the default delimiter multi-options to a parameter
     *
     * Default is "," (like in --param=option,option2,option4 and -p option,option2,option3).
     * @param del new characters to be used as delimiters
     *
     * 
     *
     */
    void setMultiOptionDelimiter( const std::string& del );

    /** @brief Change the default delimiter for an option to a short-name parameter
     *
     * Default is " " (like in -p option).
     * @param del new characters to be used as delimiters
     *
     * 
     *
     */
    void setShortOptionDelimiter( const std::string& del );


protected:
    /** @brief implement this to be notified of every parameters as soon as it is decoded
     * @param param the decoded parameter
     *
     * 
     *
     */
    virtual void customParsedParameter( const CmdLineParameter& param );

    /** @brief implement this to be notified of every extra as soon as it is decoded
     * @param extra the decoded extra
     *
     * 
     *
     */
    virtual void customParsedExtra( const std::string& extra );

private:
    __private_CmdlineParser* _private;

};

}

#endif // CMDLINEPARSER_H
