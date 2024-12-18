#include "pithermoutils.h"

#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <netinet/in.h>
#include "linux/kd.h"
#include "termios.h"
#include "fcntl.h"
#include <sys/ioctl.h>
#include <pwd.h>
#define _getcwd getcwd
extern char **environ;

#include <stdio.h>
#include <math.h>
#include <algorithm>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <math.h>

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <set>

std::set<std::string> PithermoUtils::listFolder(const std::string &path, bool list_folders )
{
    std::set<std::string> items;
    struct stat statbuf;
    struct dirent* dp;
    DIR* dirp = opendir( path.c_str() );
    if ( dirp != NULL )
    {
        while ( (dp = readdir(dirp)) != NULL )
        {
            std::string file_path = path + "/" + dp->d_name;
            if ( stat( file_path.c_str(), &statbuf ) == 0 )
            {
                if ( S_ISDIR(statbuf.st_mode) || list_folders )
                {
                    items.insert( dp->d_name );
                }
            }
        }
        closedir(dirp);
    }
    return items;
}

std::string PithermoUtils::resolvePath(const std::string &path)
{
    char buff[4096] = "";
    ssize_t len = 0;
    if ( (len = readlink( path.c_str(), buff, 4095)) != -1 )
    {
        buff[len] = '\0';
        return buff;
    }
    return path;
}

bool PithermoUtils::fileExist(const std::string &filename, bool check_folder)
{
    bool ret = false;
    std::string fn = filename;
    if ( check_folder )
        fn += "/";
    if ( access( fn.c_str(), R_OK) == 0 )
        ret = true;
    return ret;
}

bool PithermoUtils::deleteFile(const std::string &filename)
{
    return ::unlink( filename.c_str() ) == 0;
}


std::string PithermoUtils::getOsArch()
{
	std::string platform_bit = "";
	std::string platform_compiler = "";

    platform_bit = "linux";
    platform_compiler = "-unknown";
    return platform_bit + platform_compiler;
}

int32_t PithermoUtils::getPID()
{
    int32_t ret = 0;
    ret = getpid();
    return ret;
}

int32_t PithermoUtils::getParentPID()
{
    int32_t ret = 0;
    ret = getppid();
    return ret;
}

std::string PithermoUtils::getDirSep()
{
    return "/";
}

bool PithermoUtils::mkDir(const std::string &filename)
{
    bool ret = false;
    ret = ( ::mkdir( filename.c_str(), 0750 ) == 0);
    return ret;
}

bool PithermoUtils::rmDir(const std::string &filename)
{
    bool ret = false;
    ret = ( ::rmdir( filename.c_str() ) == 0);
    return ret;
}

void PithermoUtils::string_trim( std::string& original_str )
{
    if ( original_str == "" )
        return;

    while ( (original_str.at(0) ==  ' ') ||
            (original_str.at(0) == '\n') ||
            (original_str.at(0) == '\r') ||
            (original_str.at(0) == '\t') )
    {
        original_str.erase(0, 1);
        if ( original_str == "" )
            return;
    }
    while ( (original_str.at( original_str.length()-1 ) == ' ')  ||
            (original_str.at( original_str.length()-1 ) == '\n') ||
            (original_str.at( original_str.length()-1 ) == '\r') ||
            (original_str.at( original_str.length()-1 ) == '\t') )
    {
        original_str.erase( original_str.length()-1 );
        if ( original_str == "" )
            return;
    }
}

std::string PithermoUtils::string_trim(const std::string& str )
{
    std::string trimmed = str;
    string_trim( trimmed );
    return trimmed;
}

std::string PithermoUtils::string_tolower(const std::string& str )
{
    std::string ret = str;
    std::transform( ret.begin(), ret.end(), ret.begin(), ::tolower );
    return ret;
}

std::string PithermoUtils::string_toupper(const std::string& str )
{
    std::string ret = str;
    std::transform( ret.begin(), ret.end(), ret.begin(), ::toupper );
    return ret;
}

double PithermoUtils::string_tod(const std::string &str)
{
    return atof( str.c_str() );
}

float PithermoUtils::string_tof(const std::string &str)
{
    return static_cast<float>(atof( str.c_str() ));
}

int PithermoUtils::string_toi(const std::string &str)
{
    return atoi( str.c_str() );
}

uint64_t PithermoUtils::string_tou(const std::string &str)
{
    return static_cast<uint64_t>(atoll( str.c_str() ));
}

std::vector<std::string> PithermoUtils::string_split(const std::string &str, const std::string &sep)
{
    std::string s = str;
    std::vector<std::string> items;
    size_t pos = 0;
    while ( (pos = s.find_first_of( sep ) ) != std::string::npos )
    {
        items.push_back( s.substr( 0, pos ) );
        s = s.substr( pos+1 );
    }
    items.push_back( s );
    return items;
}

std::vector<std::string> PithermoUtils::vector_shift(const std::vector<std::string> &vector, uint32_t pos)
{
    std::vector<std::string> ret;
    for ( uint32_t p = pos; p < vector.size(); ++p )
        ret.push_back( vector[p] );
    return ret;
}

std::list<std::string> PithermoUtils::string_split_list(const std::string &str, const std::string &sep)
{
    std::string s = str;
    std::list<std::string> items;
    size_t pos = 0;
    while ( (pos = s.find_first_of( sep ) ) != std::string::npos )
    {
        items.push_back( s.substr( 0, pos ) );
        s = s.substr( pos+1 );
    }
    items.push_back( s );
    return items;
}

std::set<std::string> PithermoUtils::string_split_set(const std::string &str, const std::string &sep)
{
    std::string s = str;
    std::set<std::string> items;
    size_t pos = 0;
    while ( (pos = s.find_first_of( sep ) ) != std::string::npos )
    {
        items.insert( s.substr( 0, pos ) );
        s = s.substr( pos+1 );
    }
    items.insert( s );
    return items;
}

std::vector<std::string> PithermoUtils::parse_csv(const std::string &row, const std::string& sep)
{
    int len = row.length();
    std::vector<std::string> ret;
    const char* row_str = row.c_str();
    bool escape = false;
    bool between_quotes = false;
    int pos = 0;
    std::string token;

    for ( pos = 0; pos < len; pos++ )
    {
        char character = row_str[ pos ];
        bool skip_character = true;

        switch ( character )
        {
        case '\\':
            if ( escape )
            {
                escape = false;
                skip_character = false;
            }
            else
                escape = true;
            break;

        case '"':
            if ( escape ) // ignore escaped "
            {
                skip_character = false;
                escape = false;
            }
            else
                between_quotes = !between_quotes;
            break;

        default:
            if ( character == sep.at(0) )
            {
                if ( escape || between_quotes ) // ignore escaped separators and separators inside quoted strings
                {
                    skip_character = false;
                    escape = false;
                }
                else
                {                    
                    ret.push_back( token );
                    token = "";
                }
            }
            else
            {
                if ( escape )
                {
                    escape = false;
                    token += '\\';
                }
                skip_character = false;
            }
        }

        if ( !skip_character )
            token += character;
    }
    ret.push_back( token );
    return ret;
}

std::string PithermoUtils::string_replace(const std::string &src, const std::string &token, const std::string &value, bool only_one )
{
    std::string ret_str;
    ret_str.reserve( src.size() + value.size() );
    std::string temp = "";
    int token_pos = 0;
    int src_len = src.length();
    int token_len = token.length();
    int pos = 0;

    while ( pos < src_len )
    {
        char src_char = src.at( pos++ );
        if ( src_char == token.at( token_pos ) )
        {
            temp += src_char;
            token_pos++;
            if ( token_pos == token_len )
            {
                ret_str += value;
                token_pos = 0;
                temp.clear();
                if ( only_one )
                {
                    ret_str += src.substr( pos );
                    return ret_str;
                }
            }
        }
        else
        {
            if ( token_pos > 0 )
            {
                ret_str += temp;
                temp.clear();
                token_pos = 0;
            }
            ret_str += src_char;
        }
    }
    return ret_str;
}

std::vector<std::string> PithermoUtils::string_split_quotes(const std::string& str)
{
    std::vector<std::string> tokens;
    bool skip_spaces = false;
    std::string token;
    uint32_t n = 0;
    while ( n < str.length() )
    {
        int chr = str.at( n++ );
        if ( (chr == ' ') && !skip_spaces )
        {
            if ( token.length() > 0 )
                tokens.push_back( token );
            token = "";
        }
        else if ( chr == '"' )
            skip_spaces = !skip_spaces;
        else
            token += chr;
    }
    if ( token.length() > 0 )
        tokens.push_back( token );
    return tokens;
}

std::string PithermoUtils::read_string(FILE* file)
{
    std::string ret = "";
    uint32_t size = 0;
    fread( &size, sizeof(size), 1, file );
    if ( (size > 0) && (size < std::string::npos) )
    {
        char * tmp = new char[size+1];
        fread( tmp, size, 1, file );
        tmp[size] = '\0';
        ret = std::string( tmp, size );
        delete [] tmp;
    }
    return ret;
}

void PithermoUtils::write_string(FILE* file, const std::string &str )
{
    uint32_t size = str.length();
    fwrite( &size, sizeof(size), 1, file );
    fwrite( str.c_str(), size, 1, file );
}

bool PithermoUtils::isBlank(char c)
{
    return ( c == ' ' ) || ( c == '\t' );
}

void PithermoUtils::print_errno_string(const std::string &msg , const std::string &extra)
{
    std::string errmsg = get_errno_string(extra);
    printf("%s: %s\n", msg.c_str(), errmsg.c_str() );
}

std::string PithermoUtils::get_errno_string(const std::string &extra)
{
    std::string err_str;
    char buffer[4096];
    buffer[0] = '\0';
    err_str = strerror_r(errno, buffer, 4096);
    sprintf(buffer, "%s (%d)-[%s]\n", err_str.c_str(), errno, extra.c_str() );
    return std::string(buffer);
}

bool PithermoUtils::file_to_str( std::string name, std::string& str )
{
    std::ifstream file;
    std::stringstream ss;
    file.open(name.c_str());
    if(!file.is_open())
    {
        str = "";
        return false;
    }
    if( ss << file.rdbuf() )
    {
        str = ss.str();
        file.close();
        return true;
    }
    return false;
}

bool PithermoUtils::str_to_file( std::string name, std::string str )
{
    if( str == "" )
    {
        return false;
    }
    std::ofstream file;
    std::stringstream ss( str );
    file.open(name.c_str());
    if(!file.is_open())
    {
        return false;
    }
    file << ss.str();
    file.close();

    return true;
}

std::string PithermoUtils::getCurrentUsername()
{
    uid_t uid = geteuid();
    passwd *pw = getpwuid( uid );
    if ( pw )
        return pw->pw_name;
    else
        return "";
}

std::string PithermoUtils::read_env(const std::string &str)
{
    const char* ev = getenv( str.c_str() );
    if ( ev != NULL )
        return ev;
    else
        return "";
}

bool PithermoUtils::check_env(const std::string &str)
{
    char** env = NULL;
    for ( env = environ; *env != 0; env++ )
    {
        std::string env_var_str( env[0], strlen( env[0] ) );
        size_t eq_pos = env_var_str.find_first_of('=');
        env_var_str = env_var_str.substr( 0, eq_pos );
        if (  env_var_str == str )
            return true;
    }
    return false;
}

std::string PithermoUtils::env_replace(const std::string &str)
{
    if ( str.length() == 0 )
        return str;

    std::string ret = "";
    bool in_var = false;
    std::string::size_type prev_sep_pos = 0;
    std::string::size_type sep_pos = str.find_first_of('%');
    while ( sep_pos != std::string::npos )
    {
        if ( !in_var )
        {
            in_var = true;
            ret += str.substr( prev_sep_pos, sep_pos-prev_sep_pos );
        }
        else
        {
            in_var = false;
            std::string var = str.substr( prev_sep_pos+1, sep_pos-prev_sep_pos-1 );
            sep_pos += 1;
            std::string subst = read_env( var );
            ret += subst;
        }
        prev_sep_pos = sep_pos;
        sep_pos = str.find('%', sep_pos+1);
    }
    ret += str.substr( prev_sep_pos );
    return ret;
}

std::string PithermoUtils::human_readable_number(int64_t number,
                                                  const std::string &unit,
                                                  bool print_decimals,
                                                  human_readable_type type,
                                                  int8_t level)
{
    std::string ret = "";
    std::string sign = "";

    // Make the number always positive:
    if ( number < 0 )
    {
        sign = "-";
        number *= -1;
    }

    std::vector<char> coeffs;
    coeffs.push_back( ' ' );
    coeffs.push_back( 'K' );
    coeffs.push_back( 'M' );
    coeffs.push_back( 'G' );
    coeffs.push_back( 'T' );
    coeffs.push_back( 'P' );
    int64_t base_dividend = (type == DECIMAL_TYPE ? 1000 : 1024 );

    if ( level == -1 )
    {
        level = 0;
        int64_t tmp = number;
        while ( tmp > base_dividend )
        {
            level++;
            tmp /= base_dividend;
        }
    }
    if ( (uint32_t)level > coeffs.size() )
        level = coeffs.size()-1;

    int64_t current_dividend = pow( (double)base_dividend, (int)level );

    // Calculate decimals:
    double value_d = (double)number / (double)current_dividend;
    int64_t value = (int64_t)value_d;
    double leftover = value_d - (double)value;
    std::string decimal_str = "";
    if ( print_decimals )
    {
        decimal_str = "0";
        if ( leftover > 0.0 )
        {
            std::string step = "";
            int64_t leftover_int = 0;
            std::stringstream ss;
            if ( leftover < 1.0 )
                leftover *= 1000.0;
            if ( leftover > 1.0 )
            {
                leftover_int = (int64_t)leftover;
                ss << leftover_int;
                step = ss.str();
                if ( step.length() < 2 )
                    step = "00" + step;
                if ( step.length() < 3 )
                    step = "0" + step;
                decimal_str = step;
            }
        }
    }

    // When printing thousands and up, we humans always expect 1000 and not 1024, afterall the number is in base-10.
    std::vector<std::string> values;
    while ( value >= 1000 )
    {
        std::string step = "";
        std::stringstream ss;
        ss << (value % 1000);
        step = ss.str();
        if ( step.length() < 2 )
            step = "00" + step;
        if ( step.length() < 3 )
            step = "0" + step;
        values.push_back( step );
        value /= 1000;
    }
    std::stringstream ss;
    ss << (value % 1000);
    values.push_back( ss.str() );

    // Recompose the entire string:
    ret = sign;
    for ( std::vector<std::string>::iterator s = values.begin(); s != values.end(); ++s )
    {
        if ( s != values.begin() )
            ret = "." + ret;
        std::string step = *s;
        ret = step + ret;
    }
    if ( decimal_str != "" )
        ret += "," + decimal_str;
    if ( level > 0 )
    ret += coeffs[level];
    ret += unit;
    return ret;
}

std::string PithermoUtils::human_readable_time(int64_t number_seconds, uint64_t milliseconds)
{
    std::string ret = "";
    int64_t weeks = number_seconds / (3600*24*7);
    int64_t days = (number_seconds / (3600*24)) % 7;
    int64_t hours = (number_seconds / 3600) % 24;
    int64_t minutes = (number_seconds / 60) % 60;
    int64_t seconds = number_seconds % 60;
    if ( weeks > 0 )
    {
        std::stringstream sw;
        sw << weeks;
        ret += sw.str() + "w ";
    }
    if ( days > 0 )
    {
        std::stringstream sd;
        sd << days;
        ret += sd.str() + "d ";
    }
    std::stringstream sh;
    sh << hours;
    std::string h = sh.str();
    if ( h.length() < 2 )
        h = "0" + h;
    std::stringstream sm;
    sm << minutes;
    std::string m = sm.str();
    if ( m.length() < 2 )
        m = "0" + m;
    std::stringstream ss;
    ss << seconds;
    std::string s = ss.str();
    if ( s.length() < 2 )
        s = "0" + s;
    ret += h + ":" + m + ":" + s;
    if ( milliseconds > 0 )
    {
        std::stringstream sms;
        sms << milliseconds;
        std::string ms = sms.str();
        if ( ms.length() < 2 )
            ms = "00" + ms;
        else if ( ms.length() < 3 )
            ms = "0" + ms;
        ret += " " + ms + "ms";
    }
    return ret;
}

void PithermoUtils::reset_env(const std::string &str)
{
    unsetenv(str.c_str());
}

void PithermoUtils::write_env(const std::string &str, const std::string &val)
{
    setenv( str.c_str(), val.c_str(), 1 );
}

std::set<std::string> PithermoUtils::list_MACs()
{
    std::set<std::string> macs;
    struct ifaddrs *addrs,*tmp;
    std::set<std::string> ifaces;
    getifaddrs(&addrs);
    tmp = addrs;

    while (tmp)
    {
        if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_PACKET)
            ifaces.insert( tmp->ifa_name );
        tmp = tmp->ifa_next;
    }
    freeifaddrs(addrs);

    int fd;
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    fd = socket(AF_INET, SOCK_DGRAM, 0);

    for ( std::set<std::string>::iterator i = ifaces.begin(); i != ifaces.end(); ++i )
    {
        std::string interface_name = *i;
        std::string mac;
        ifr.ifr_addr.sa_family = AF_INET;
        strncpy(ifr.ifr_name , interface_name.c_str() , IFNAMSIZ-1);
        if (0 == ioctl(fd, SIOCGIFHWADDR, &ifr))
        {
            char tmp[25];
            snprintf(tmp, 25, "%.2X%.2X%.2X%.2X%.2X%.2X" ,
                     ((unsigned char*)ifr.ifr_hwaddr.sa_data)[0],
                    ((unsigned char*)ifr.ifr_hwaddr.sa_data)[1],
                    ((unsigned char*)ifr.ifr_hwaddr.sa_data)[2],
                    ((unsigned char*)ifr.ifr_hwaddr.sa_data)[3],
                    ((unsigned char*)ifr.ifr_hwaddr.sa_data)[4],
                    ((unsigned char*)ifr.ifr_hwaddr.sa_data)[5] );
            mac = tmp;
        }
        macs.insert( mac );
    }
    return macs;
}

std::set<std::string> PithermoUtils::list_IPs()
{
    std::set<std::string> ret;
    struct ifaddrs * addresses;
    if ( getifaddrs( &addresses ) != -1 )
    {
        struct ifaddrs * tmp = addresses;
        while ( tmp != NULL )
        {
            if ( tmp->ifa_addr != NULL )
            {
                if ( tmp->ifa_addr->sa_family == AF_INET )
                {
                    char ifip[1024];
                    if ( getnameinfo( tmp->ifa_addr, sizeof(struct sockaddr_in), ifip, 1024, NULL, 0, NI_NUMERICHOST ) != -1 )
                        ret.insert( ifip );
                }
            }
            tmp = tmp->ifa_next;
        }
        freeifaddrs( addresses );
    }
    return ret;
}

std::string PithermoUtils::getHostname()
{
    std::string hostname = "";
    char host_name[1024];
    if ( gethostname( host_name, 1024 ) == 0 )
        hostname = std::string(host_name);
    return hostname;
}

std::string PithermoUtils::getExecutablePath()
{
    char result[ PATH_MAX ];
    ssize_t count = readlink( "/proc/self/exe", result, PATH_MAX );
    return std::string( result, (count > 0) ? count : 0 );
}

std::string PithermoUtils::getCwd()
{
    char tmp[4096];
    return _getcwd( tmp, 4096 );
}

std::string PithermoUtils::resolve_hostname(const std::string &hostname)
{
    std::string ret = "";
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_in *h;
    int rv;

    memset( &hints, 0, sizeof( hints ) );
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ( (rv = getaddrinfo( hostname.c_str(), "http", &hints, &servinfo)) == 0 )
    {
        for ( p = servinfo; p != NULL; p = p->ai_next )
        {
            h = (struct sockaddr_in*)p->ai_addr;
            ret = inet_ntoa( h->sin_addr );
        }
        freeaddrinfo(servinfo);
    }
    return ret;
}

std::string PithermoUtils::tostring( const int64_t t )
{
    std::ostringstream ss;
    ss << t;
    return ss.str();
}

std::string PithermoUtils::utostring( const uint64_t t )
{
    std::ostringstream ss;
    ss << t;
    return ss.str();
}

std::string PithermoUtils::dtostring( const double t )
{
    std::ostringstream ss;
    ss << t;
    return ss.str();
}

std::string PithermoUtils::ftostring( const float t )
{
    std::ostringstream ss;
    ss << t;
    return ss.str();
}

bool PithermoUtils::chDir(const std::string &path)
{
    bool ret = false;
    ret = ( ::chdir( path.c_str() ) == 0);
    return ret;
}

PithermoUtils::PithermoUtils()
{
}
