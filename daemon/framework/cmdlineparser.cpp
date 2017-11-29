#include "cmdlineparser.h"
#include "memorychecker.h"
#include "debugprint.h"
#include "frameworkutils.h"
#include <stdio.h>

#include <set>
#include <vector>

using namespace FrameworkLibrary;

namespace FrameworkLibrary {

class __private_CmdlineParameter
{
    friend class CmdLineParameter;
    friend class CmdlineParser;

    void fixNames( const std::string& d1, const std::string& d2 )
    {
        if ( _name.substr( 0, d1.size() ) == d1 )
            _name = _name.substr( d1.size()+1 );
        if ( _short_name.substr( 0, d2.size() ) == d2 )
            _short_name = _short_name.substr( d2.size()+1 );
    }

    std::string buildHelp( const std::string& delimiter,
                           const std::string& option_delimiter,
                           const std::string& multi_delimiter )
    {
        std::string str = delimiter + _name;
        for ( uint32_t o = 0; o < _max_options; o++ )
        {
            if ( o == 0 )
                str += option_delimiter;
            else
                str += multi_delimiter;
            str += "<" + (_options_values.find(o) != _options_values.end() ? _options_values.at(o) : _options_help[ o ]) + ">";
        }
        return str;
    }

    std::string buildShortHelp( const std::string& delimiter_short,
                                const std::string& option_short_delimiter,
                                const std::string& multi_delimiter )
    {
        std::string str = "";
        if ( _short_name != "" )
        {
            str += delimiter_short + _short_name;
            for ( uint32_t o = 0; o < _max_options; o++ )
            {
                if ( o == 0 )
                    str += option_short_delimiter;
                else
                    str += multi_delimiter;
                str += "<" + (_options_values.find(o) != _options_values.end() ? _options_values.at(o) : _options_help[ o ]) + ">";
            }
        }
        return str;
    }

    std::string _name;
    std::string _short_name;
    std::string _description;
    uint32_t _min_options;
    uint32_t _max_options;
    std::map<uint32_t, std::string> _options_help;
    std::map<uint32_t, std::string> _options_values;

    CmdLineParameter::MultiMode _multiple;
    bool _mandatory;
    bool _has_default;
};

class __private_CmdlineParser
{
    friend class CmdlineParser;
    std::string _help_title;
    std::string _help_note;
    std::string _help_extra;
    std::string _help_extra_name;

    bool _allow_extra;
    bool _check_unknowns;
    bool _at_least_one_mandatory;

    std::string _delimiter;
    std::string _delimiter_param;
    std::string _delimiter_short;
    std::string _delimiter_option;
    std::string _delimiter_short_option;
    std::string _delimiter_multioption;

    std::map<std::string, CmdLineParameter> _expected_parameters;

    std::string _argv0;
    std::string _input_parameters;

    std::list<CmdLineParameter> _decoded_parameters_list;
    std::list<std::string> _decoded_extras;

    std::set<std::string> _params_list;
    std::set<std::string> _shorts_list;
};

}

CmdLineParameter::CmdLineParameter()
{
    _private = NULL;
}

CmdLineParameter::CmdLineParameter(const std::string &name, const std::string &description, MultiMode multiple, bool mandatory)
{
    _private = new __private_CmdlineParameter;
    _private->_name = name;
    _private->_description = description;
    _private->_mandatory = mandatory;
    _private->_short_name = "";
    _private->_min_options = 0;
    _private->_max_options = 0;
    _private->_has_default = false;
    _private->_multiple = multiple;
}

CmdLineParameter::CmdLineParameter(const CmdLineParameter &other)
{
    _private = NULL;
    if ( other._private != NULL )
    {
        _private = new __private_CmdlineParameter;
        _private->_name = other._private->_name;
        _private->_mandatory = other._private->_mandatory;
        _private->_short_name = other._private->_short_name;
        _private->_description = other._private->_description;
        _private->_min_options = other._private->_min_options;
        _private->_max_options = other._private->_max_options;
        _private->_options_help = other._private->_options_help;
        _private->_options_values = other._private->_options_values;
        _private->_multiple = other._private->_multiple;
        _private->_has_default = other._private->_has_default;
    }
}

CmdLineParameter::~CmdLineParameter()
{
    if ( _private != NULL )
        delete _private;
}

CmdLineParameter &CmdLineParameter::setShortName(const std::string &short_name)
{
    if ( isValid() )
    {
        _private->_short_name = short_name;
    }
    return *this;
}

CmdLineParameter &CmdLineParameter::setOptions(uint32_t num_min_options, int32_t num_max_options)
{
    if ( isValid() )
    {
        _private->_min_options = num_min_options;
        if ( num_max_options == -1 )
            _private->_max_options = num_min_options;
        else
            _private->_max_options = num_max_options;
        if ( _private->_max_options  < _private->_min_options )
            _private->_max_options = _private->_min_options;
    }
    return *this;
}

CmdLineParameter &CmdLineParameter::setOptionDefault(const std::string &default_value, uint32_t which_option)
{
    if ( isValid() )
    {
        if ( which_option < _private->_max_options )
        {
            _private->_options_values[ which_option ] = default_value;
            _private->_has_default = true;
        }
    }
    return *this;
}

CmdLineParameter &CmdLineParameter::setOptionHelp(const std::string &help_name, uint32_t which_option)
{
    if ( isValid() )
    {
        if ( which_option < _private->_max_options )
            _private->_options_help[ which_option ] = help_name;
    }
    return *this;
}

bool CmdLineParameter::isValid() const
{
    return _private != NULL;
}

uint32_t CmdLineParameter::getNumOptions() const
{
    if ( isValid() )
        return _private->_options_values.size();
    return 0;
}

std::string CmdLineParameter::getOption(uint32_t which_option) const
{
    if ( isValid() )
        if ( _private->_options_values.find( which_option ) != _private->_options_values.end() )
            return _private->_options_values.at( which_option );
    return "";
}

CmdLineParameter &CmdLineParameter::operator=(const CmdLineParameter &other)
{
    if ( this != &other )
    {
        if ( other._private == NULL )
        {
            if ( _private != NULL )
            {
                delete _private;
                _private = NULL;
            }
        }
        else
        {
            if ( _private == NULL )
                _private = new __private_CmdlineParameter;
            _private->_name = other._private->_name;
            _private->_short_name = other._private->_short_name;
            _private->_description = other._private->_description;
            _private->_min_options = other._private->_min_options;
            _private->_max_options = other._private->_max_options;
            _private->_mandatory = other._private->_mandatory;
            _private->_options_help = other._private->_options_help;
            _private->_options_values = other._private->_options_values;
            _private->_multiple = other._private->_multiple;
            _private->_has_default = other._private->_has_default;
        }
    }
    return *this;
}

std::string CmdLineParameter::getName() const
{
    return _private->_name;
}

CmdlineParser::CmdlineParser(const std::string &help_title,
                             const std::string &help_note )
{
    _private = new __private_CmdlineParser;
    _private->_allow_extra = false;
    _private->_help_extra_name = "";
    _private->_help_extra = "";
    _private->_help_title = help_title == "" ? "Usage help:" : help_title;
    _private->_help_note = help_note == "" ? "Please refer to the provided documentation for more support" : help_note;
    _private->_delimiter_param = "--";
    _private->_delimiter_short = "-";
    _private->_delimiter_option = "=";
    _private->_delimiter_short_option = "=";
    _private->_delimiter_multioption = ",";
    _private->_delimiter = "\"";
    _private->_check_unknowns = true;
    _private->_at_least_one_mandatory = false;

    defineParameter( CmdLineParameter("help", "Show this help", CmdLineParameter::multiple).setShortName("h") );
}

CmdlineParser::~CmdlineParser()
{
    _private->_expected_parameters.clear();
    _private->_input_parameters.clear();
    _private->_decoded_parameters_list.clear();
    _private->_decoded_extras.clear();
    delete _private;
    _private = NULL;
}

void CmdlineParser::enableExtras(const std::string &help_extra, const std::string &help_extra_name)
{
    _private->_allow_extra = true;
    _private->_help_extra_name = help_extra_name == "" ? "more" : help_extra_name;
    _private->_help_extra = help_extra == "" ? "extra parameters" : help_extra;
}

void CmdlineParser::setParameterDelimiter(const std::string &del)
{
    _private->_delimiter_param = del;
}

void CmdlineParser::setShortDelimiter(const std::string &del)
{
    _private->_delimiter_short = del;
}

void CmdlineParser::setOptionDelimiter(const std::string &del)
{
    _private->_delimiter_option = del;
}

void CmdlineParser::setShortOptionDelimiter(const std::string &del)
{
    _private->_delimiter_short_option = del;
}

void CmdlineParser::setMultiOptionDelimiter(const std::string &del)
{
    _private->_delimiter_multioption = del;
}

void CmdlineParser::defineParameter(const CmdLineParameter &param)
{
    if ( param.isValid() )
    {
        // Check for duplicates:
        if ( _private->_params_list.find( param._private->_name ) == _private->_params_list.end() )
        {
            if ( _private->_shorts_list.find( param._private->_short_name ) == _private->_shorts_list.end() )
            {
                param._private->fixNames( _private->_delimiter_param, _private->_delimiter_short );
                _private->_expected_parameters[ param._private->_name ] = param;
            }
            else
                debugPrintError( "CmdLineParser" ) << "Duplicated short-name: " << param._private->_short_name << " for parameter " << param._private->_name << " specified!\n";
        }
        else
            debugPrintError( "CmdLineParser" ) << "Duplicated parameter " << param._private->_name << " specified!\n";
    }
    else
        debugPrintError( "CmdLineParser" ) << "Invalid parameter " << param._private->_name << " specified!\n";
}

void CmdlineParser::setCommandLine(uint32_t argc, char **argv)
{
    if ( (argc > 0) && (argv != NULL) )
    {
        _private->_argv0 = argv[0];
        _private->_input_parameters = "";
        if ( argc > 1 )
        {
            for ( uint32_t x = 1; x < (argc-1); x++ )
            {
                std::string str = argv[x] != NULL ? argv[x] : "";
                FrameworkUtils::string_trim( str );
                _private->_input_parameters += str + _private->_delimiter;
            }
            _private->_input_parameters += argv[ argc-1 ];
        }
    }
}

bool CmdlineParser::parse()
{
    bool error = false;

    _private->_decoded_extras.clear();
    _private->_decoded_parameters_list.clear();

    // Walk the entire input string, split it up, and decode all parameters and options:
    uint32_t input_pos = 0;
    uint32_t input_size = _private->_input_parameters.length();
    uint32_t last_parameter_pos = 0;
    std::string parameter = "";
    std::string option = "";
    bool in_parameter = false;
    std::string option_delimiter = "";
    uint32_t last_delimiter_size = 0;
    std::string prev_c = _private->_delimiter;
    while ( input_pos < input_size )
    {
        // Not found next parameter? check...
        if ( !in_parameter )
        {
            if ( ( prev_c == _private->_delimiter ) &&
                 (_private->_input_parameters.substr( input_pos, _private->_delimiter_param.size() ) == _private->_delimiter_param ) )
            {
                in_parameter = true;
                last_delimiter_size = _private->_delimiter_param.size();
                input_pos += last_delimiter_size;
                option_delimiter = _private->_delimiter_option;
            }
            else if ( ( prev_c == _private->_delimiter ) &&
                      (_private->_input_parameters.substr( input_pos, _private->_delimiter_short.size() ) == _private->_delimiter_short ) )
            {
                in_parameter = true;
                last_delimiter_size = _private->_delimiter_short.size();
                input_pos += last_delimiter_size;
                option_delimiter = _private->_delimiter_short_option;
            }
            else // this could be extra stuff
            {
                prev_c = _private->_input_parameters.substr( input_pos,1 );
                input_pos++;
            }
        }

        if ( in_parameter )
        { // in parameter, split up param and option

            // Split&save all extras:
            std::string extra =  _private->_input_parameters.substr( last_parameter_pos, input_pos - last_parameter_pos - last_delimiter_size );
            FrameworkUtils::string_trim( extra );
            std::vector<std::string> tokens = FrameworkUtils::string_split( extra, _private->_delimiter );
            for ( std::vector<std::string>::iterator e = tokens.begin(); e != tokens.end(); ++e )
            {
                std::string token = *e;
                FrameworkUtils::string_trim( token );
                if ( token != "" )
                {
                    if ( _private->_allow_extra )
                    {
                        _private->_decoded_extras.push_back( token );
                        customParsedExtra( token );
                    }
                    else
                    {
                        error = true;
                        debugPrintError( "CmdlineParser" ) << "ERROR detected unsupported extra option\n";
                    }
                }
            }
            size_t end_pos = _private->_input_parameters.find_first_of( _private->_delimiter, input_pos );
            size_t option_pos = _private->_input_parameters.find_first_of( option_delimiter, input_pos );
            if ( option_pos < end_pos )
            {
                parameter = _private->_input_parameters.substr( input_pos, option_pos-input_pos );
                option = _private->_input_parameters.substr( option_pos+option_delimiter.size(), end_pos-option_pos-1 );
            }
            else
            {
                parameter = _private->_input_parameters.substr( input_pos, end_pos-input_pos );
                option = "";
            }

            if ( end_pos != std::string::npos )
                input_pos = end_pos+1;//option_delimiter.size();
            else
                input_pos = input_size;

            // Now, check if this is valid:
            bool valid = false;
            for ( std::map<std::string, CmdLineParameter>::iterator p = _private->_expected_parameters.begin();
                  !valid && (p != _private->_expected_parameters.end());
                  ++p )
            {
                CmdLineParameter defined_parameter = p->second;
                if ( defined_parameter._private != NULL )
                {
                    if ( ( defined_parameter._private->_name == parameter) ||
                         ( defined_parameter._private->_short_name == parameter ) )
                    {
                        valid = true;
                        uint32_t min_params = defined_parameter._private->_min_options;
                        uint32_t max_params = defined_parameter._private->_max_options;
                        std::vector<std::string> options = FrameworkUtils::string_split( option, _private->_delimiter_multioption );

                        if ( options.size() < max_params )
                        {   // try add default options...
                            for ( uint32_t no = options.size(); no < max_params; no++ )
                            {
                                std::map<uint32_t, std::string>::iterator f =  defined_parameter._private->_options_values.find( no );
                                if ( f != defined_parameter._private->_options_values.end() )
                                    options.push_back( f->second );
                                else
                                    no = max_params;
                            }
                        }
                        uint32_t n_options = options.size();
                        if ( n_options == 1 )
                            if ( options[0] == "" )
                                n_options = 0;
                        if ( ( n_options >= min_params ) && ( n_options <= max_params ) )
                        {
                            for ( uint32_t np = 0; np < n_options; np++ )
                                defined_parameter._private->_options_values[ np ] = options[ np ];

                            // Check multeplicity:
                            if ( !hasParameter( defined_parameter.getName()) || ( defined_parameter._private->_multiple == CmdLineParameter::multiple ) )
                            {
                                _private->_decoded_parameters_list.push_back( defined_parameter );
                                customParsedParameter( defined_parameter );
                            }
                            else
                            {
                                error = true;
                                debugPrintError( "CmdlineParser" ) << "ERROR parameter: " << parameter << " specified more than once!\n\n";
                            }
                        }
                        else
                        {
                            error = true;
                            debugPrintError( "CmdlineParser" ) << "ERROR OPTION mismatch for parameter: " << parameter << "\n\n";
                        }
                    }
                }
            }
            if ( _private->_check_unknowns && !valid )
            {
                error = true;
                debugPrintError( "CmdlineParser" ) << "ERROR INVALID PARAMETER: " << parameter << "\n\n";
            }
            prev_c = _private->_delimiter;
            in_parameter = false;
            last_parameter_pos = input_pos;
            last_delimiter_size = 0;
        } // if in parameter
    } // while

    // Split&save remaining extras:
    std::string extra =  _private->_input_parameters.substr( last_parameter_pos, input_pos - last_parameter_pos - last_delimiter_size );
    FrameworkUtils::string_trim( extra );
    std::vector<std::string> tokens = FrameworkUtils::string_split( extra, _private->_delimiter );
    for ( std::vector<std::string>::iterator e = tokens.begin(); e != tokens.end(); ++e )
    {
        std::string token = *e;
        FrameworkUtils::string_trim( token );
        if ( token != "" )
        {
            if ( _private->_allow_extra )
            {
                _private->_decoded_extras.push_back( token );
                customParsedExtra( token );
            }
            else
            {
                error = true;
                debugPrintError( "CmdlineParser" ) << "ERROR detected unsupported extra option\n";
            }
        }
    }

    // Copy default values, add the missing ones:
    for ( std::map<std::string, CmdLineParameter>::iterator p = _private->_expected_parameters.begin();
          p != _private->_expected_parameters.end();
          ++p )
    {
        CmdLineParameter defined_parameter = p->second;
        if ( defined_parameter._private != NULL )
            if ( defined_parameter._private->_has_default )
                if( !hasParameter( defined_parameter.getName() ) )
                    _private->_decoded_parameters_list.push_back( defined_parameter );
    }

    // Check if we got all the mandatories or not:
    for ( std::map<std::string, CmdLineParameter>::iterator p = _private->_expected_parameters.begin();
          p != _private->_expected_parameters.end();
          ++p )
    {
        CmdLineParameter defined_parameter = p->second;
        if ( defined_parameter._private != NULL )
        {
            if ( defined_parameter._private->_mandatory )
            {
                if ( !hasParameter( defined_parameter.getName() ) )
                {
                    debugPrintError( "CmdlineParser" ) << "ERROR MISSING MANDATORY PARAMETER: " << defined_parameter._private->_name << "\n\n";
                    error = true;
                }
            }
        }
    }

    if ( _private->_at_least_one_mandatory && ( _private->_decoded_parameters_list.size() == 0 ) )
    {
        debugPrintError( "CmdlineParser" ) << "You must specify at least one parameter!\n\n";
        error = true;
    }

    // Print help if requested
    if ( error || ( hasParameter("help") ) )
    {
        printHelp();
        error = true;
    }
    return !error;
}

void CmdlineParser::printHelp()
{
    std::string header = _private->_help_title;
    std::list<std::string> examples;
    std::list<std::string> summaries;
    std::string footer = _private->_help_note;

    debugPrintUntagged() << "\t " << _private->_argv0 << " ";
    for ( std::map<std::string, CmdLineParameter>::iterator p = _private->_expected_parameters.begin(); p != _private->_expected_parameters.end(); ++p )
    {
        CmdLineParameter param( p->second );
        if ( param._private != NULL )
        {
            bool is_mandatory = param._private->_mandatory;
            bool is_multiple = param._private->_multiple == CmdLineParameter::multiple;

            std::string example = "";
            std::string short_example = param._private->buildShortHelp( _private->_delimiter_short, _private->_delimiter_short_option, _private->_delimiter_multioption );
            if ( short_example != "" )
                example += short_example + " | ";
            example += param._private->buildHelp( _private->_delimiter_param, _private->_delimiter_option, _private->_delimiter_multioption );

            examples.push_back( "\t" + _private->_argv0 + " " + (is_mandatory ? "":"[") + example + (is_mandatory ? "":"]") );
            summaries.push_back( "\t" + param._private->_description + ":\t" + example + (is_mandatory ? " [REQUIRED]":"") + (is_multiple ? " (only once) " : ""));
        }
    }
    if ( _private->_allow_extra )
    {
        examples.push_back( "\t" + _private->_argv0 + " [" + _private->_help_extra_name + "...]" );
        summaries.push_back( "\t" + _private->_help_extra + ": " + _private->_help_extra_name );
    }

    debugPrintUntagged() << header << "\n";
    debugPrintUntagged() << "\n";
    debugPrintUntagged() << "Example usages:\n";
    for ( std::list<std::string>::iterator i = examples.begin(); i != examples.end(); ++i )
        debugPrintUntagged() << *i << "\n";
    debugPrintUntagged() << "\n";
    debugPrintUntagged() << "Summary of all available commands:\n";
    for ( std::list<std::string>::iterator i = summaries.begin(); i != summaries.end(); ++i )
        debugPrintUntagged() << *i << "\n";
    debugPrintUntagged() << "\n";
    debugPrintUntagged() << footer << "\n";
    debugPrintUntagged() << "\n";
}

void CmdlineParser::disableCheckUnknowns(bool disable_check)
{
    _private->_check_unknowns = !disable_check;
}

void CmdlineParser::enableOneParameterMandatory(bool mandatory)
{
    _private->_at_least_one_mandatory = mandatory;
}

std::string CmdlineParser::getArgv0() const
{
    return _private->_argv0;
}

std::list<std::string> CmdlineParser::getExtras() const
{
    return _private->_decoded_extras;
}

CmdLineParameter CmdlineParser::consumeParameter(const std::string &param_name) const
{    
    for ( std::list<CmdLineParameter>::iterator i = _private->_decoded_parameters_list.begin(); i != _private->_decoded_parameters_list.end(); ++i )
    {
        if ( (param_name == "") || ( (*i).getName() == param_name) )
        {
            CmdLineParameter param = *i;
            _private->_decoded_parameters_list.erase( i );
            return param;
        }
    }
    return CmdLineParameter();
}

bool CmdlineParser::hasParameter(const std::string &param_name) const
{
    for ( std::list<CmdLineParameter>::iterator i = _private->_decoded_parameters_list.begin(); i != _private->_decoded_parameters_list.end(); ++i )
    {
        CmdLineParameter &param = *i;
        if ( param.getName() == param_name)
            return true;
    }
    return false;
}

uint32_t CmdlineParser::getNumParameters() const
{
    return _private->_decoded_parameters_list.size();
}

void CmdlineParser::customParsedParameter(const CmdLineParameter &param)
{
    std::string opts;
    for ( uint32_t p = 0; p < param.getNumOptions(); p++ )
    {
        if ( p > 0 )
            opts += _private->_delimiter_multioption;
        opts += param.getOption( p );
    }
}

void CmdlineParser::customParsedExtra(const std::string &extra)
{
}
