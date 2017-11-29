#include "frameworkutils.h"
#include "debugprint.h"

using namespace FrameworkLibrary;

#if defined(FRAMEWORK_PLATFORM_WINDOWS)
#include <snmp.h>
#include <conio.h>
#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <direct.h>
#include <iphlpapi.h>
#include <iprtrmib.h>
#elif defined(FRAMEWORK_PLATFORM_LINUX)
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
#endif

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

#if defined(FRAMEWORK_PLATFORM_WINDOWS)
#ifndef SNMPAPI
#define SNMPAPI INT
#endif
#ifndef SNMP_FUNC_TYPE
#define SNMP_FUNC_TYPE WINAPI
#endif

SNMPAPI
SNMP_FUNC_TYPE
SnmpUtilOidCpy(
        OUT AsnObjectIdentifier *DstObjId,
        IN  AsnObjectIdentifier *SrcObjId
        )
{
    DstObjId->ids = (UINT *)GlobalAlloc(GMEM_ZEROINIT,SrcObjId->idLength *
                                        sizeof(UINT));
    if(!DstObjId->ids){
        SetLastError(1);
        return 0;
    }

    memcpy(DstObjId->ids,SrcObjId->ids,SrcObjId->idLength*sizeof(UINT));
    DstObjId->idLength = SrcObjId->idLength;

    return 1;
}

VOID
SNMP_FUNC_TYPE
SnmpUtilOidFree(
        IN OUT AsnObjectIdentifier *ObjId
        )
{
    GlobalFree(ObjId->ids);
    ObjId->ids = 0;
    ObjId->idLength = 0;
}

SNMPAPI
SNMP_FUNC_TYPE
SnmpUtilOidNCmp(
        IN AsnObjectIdentifier *ObjIdA,
        IN AsnObjectIdentifier *ObjIdB,
        IN UINT                 Len
        )
{
    UINT CmpLen;
    UINT i;
    int  res;

    CmpLen = Len;
    if(ObjIdA->idLength < CmpLen)
        CmpLen = ObjIdA->idLength;
    if(ObjIdB->idLength < CmpLen)
        CmpLen = ObjIdB->idLength;

    for(i=0;i<CmpLen;i++){
        res = ObjIdA->ids[i] - ObjIdB->ids[i];
        if(res!=0)
            return res;
    }
    return 0;
}

VOID
SNMP_FUNC_TYPE
SnmpUtilVarBindFree(
        IN OUT RFC1157VarBind *VarBind
        )
{
    BYTE asnType;
    // free object name
    SnmpUtilOidFree(&VarBind->name);

    asnType = VarBind->value.asnType;

    if(asnType==ASN_OBJECTIDENTIFIER){
        SnmpUtilOidFree(&VarBind->value.asnValue.object);
    }
    else if(
            (asnType==ASN_OCTETSTRING) ||
            (asnType==ASN_RFC1155_IPADDRESS) ||
            (asnType==ASN_RFC1155_OPAQUE) ||
            (asnType==ASN_SEQUENCE)){
        if(VarBind->value.asnValue.string.dynamic){
            GlobalFree(VarBind->value.asnValue.string.stream);
        }
    }

    VarBind->value.asnType = ASN_NULL;

}

typedef BOOL(WINAPI * pSnmpExtensionInit) (
        IN DWORD dwTimeZeroReference,
        OUT HANDLE * hPollForTrapEvent,
        OUT AsnObjectIdentifier * supportedView);

typedef BOOL(WINAPI * pSnmpExtensionTrap) (
        OUT AsnObjectIdentifier * enterprise,
        OUT AsnInteger * genericTrap,
        OUT AsnInteger * specificTrap,
        OUT AsnTimeticks * timeStamp,
        OUT RFC1157VarBindList * variableBindings);

typedef BOOL(WINAPI * pSnmpExtensionQuery) (
        IN BYTE requestType,
        IN OUT RFC1157VarBindList * variableBindings,
        OUT AsnInteger * errorStatus,
        OUT AsnInteger * errorIndex);

typedef BOOL(WINAPI * pSnmpExtensionInitEx) (
        OUT AsnObjectIdentifier * supportedView);

static int getmacs(std::set<std::string> &macs)
{
    WSADATA WinsockData;
    if (WSAStartup(MAKEWORD(2, 0), &WinsockData) != 0) {
        fprintf(stderr, "This program requires Winsock 2.x!\n");
        return 0;
    }

    HINSTANCE m_hInst;
    pSnmpExtensionInit m_Init;
    //    pSnmpExtensionInitEx m_InitEx;
    pSnmpExtensionQuery m_Query;
    //    pSnmpExtensionTrap m_Trap;
    HANDLE PollForTrapEvent;
    AsnObjectIdentifier SupportedView;
    UINT OID_ifEntryType[] = {
        1, 3, 6, 1, 2, 1, 2, 2, 1, 3
    };
    UINT OID_ifEntryNum[] = {
        1, 3, 6, 1, 2, 1, 2, 1
    };
    UINT OID_ipMACEntAddr[] = {
        1, 3, 6, 1, 2, 1, 2, 2, 1, 6
    };                          //, 1 ,6 };
    AsnObjectIdentifier MIB_ifMACEntAddr =
    { sizeof(OID_ipMACEntAddr) / sizeof(UINT), OID_ipMACEntAddr };
    AsnObjectIdentifier MIB_ifEntryType = {
        sizeof(OID_ifEntryType) / sizeof(UINT), OID_ifEntryType
    };
    AsnObjectIdentifier MIB_ifEntryNum = {
        sizeof(OID_ifEntryNum) / sizeof(UINT), OID_ifEntryNum
    };
    RFC1157VarBindList varBindList;
    RFC1157VarBind varBind[2];
    AsnInteger errorStatus;
    AsnInteger errorIndex;
    AsnObjectIdentifier MIB_NULL = {
        0, 0
    };
    int ret;
    int dtmp;
    int /*i = 0,*/ j = 0;
    //    BOOL found = FALSE;
    char TempEthernet[13];
    m_Init = NULL;
    //    m_InitEx = NULL;
    m_Query = NULL;
    //    m_Trap = NULL;

    /* Load the SNMP dll and get the addresses of the functions
       necessary */
    m_hInst = LoadLibrary("inetmib1.dll");
    if (m_hInst < (HINSTANCE) HINSTANCE_ERROR) {
        m_hInst = NULL;
        return 0;
    }
    m_Init =
            (pSnmpExtensionInit) GetProcAddress(m_hInst, "SnmpExtensionInit");
    /* m_InitEx =*/
    (pSnmpExtensionInitEx) GetProcAddress(m_hInst,
                                          "SnmpExtensionInitEx");
    m_Query =
            (pSnmpExtensionQuery) GetProcAddress(m_hInst,
                                                 "SnmpExtensionQuery");
    /*  m_Trap =*/
    (pSnmpExtensionTrap) GetProcAddress(m_hInst, "SnmpExtensionTrap");
    m_Init(GetTickCount(), &PollForTrapEvent, &SupportedView);

    /* Initialize the variable list to be retrieved by m_Query */
    varBindList.list = varBind;
    varBind[0].name = MIB_NULL;
    varBind[1].name = MIB_NULL;

    /* Copy in the OID to find the number of entries in the
       Inteface table */
    varBindList.len = 1;        /* Only retrieving one item */
    SNMP_oidcpy(&varBind[0].name, &MIB_ifEntryNum);
    ret =
            m_Query(ASN_RFC1157_GETNEXTREQUEST, &varBindList, &errorStatus,
                    &errorIndex);
    //printf("# of adapters in this system : %i\n", varBind[0].value.asnValue.number);
    varBindList.len = 2;

    /* Copy in the OID of ifType, the type of interface */
    SNMP_oidcpy(&varBind[0].name, &MIB_ifEntryType);

    /* Copy in the OID of ifPhysAddress, the address */
    SNMP_oidcpy(&varBind[1].name, &MIB_ifMACEntAddr);

    do {

        /* Submit the query.  Responses will be loaded into varBindList.
           We can expect this call to succeed a # of times corresponding
           to the # of adapters reported to be in the system */
        ret =
                m_Query(ASN_RFC1157_GETNEXTREQUEST, &varBindList, &errorStatus,
                        &errorIndex);
        if (!ret)
            ret = 1;
        else
            /* Confirm that the proper type has been returned */
            ret =
                    SNMP_oidncmp(&varBind[0].name, &MIB_ifEntryType,
                    MIB_ifEntryType.idLength);
        if (!ret) {
            j++;
            dtmp = varBind[0].value.asnValue.number;
            //printf("Interface #%i type : %i\n", j, dtmp);

            /* Type 6 describes ethernet interfaces */
            if (dtmp == 6) {

                /* Confirm that we have an address here */
                ret =
                        SNMP_oidncmp(&varBind[1].name, &MIB_ifMACEntAddr,
                        MIB_ifMACEntAddr.idLength);
                if ((!ret)
                        && (varBind[1].value.asnValue.address.stream != NULL)) {
                    if (
                            (varBind[1].value.asnValue.address.stream[0] ==
                             0x44)
                            && (varBind[1].value.asnValue.address.stream[1] ==
                                0x45)
                            && (varBind[1].value.asnValue.address.stream[2] ==
                                0x53)
                            && (varBind[1].value.asnValue.address.stream[3] ==
                                0x54)
                            && (varBind[1].value.asnValue.address.stream[4] ==
                                0x00)) {

                        /* Ignore all dial-up networking adapters */
                        //printf("Interface #%i is a DUN adapter\n", j);
                        continue;
                    }
                    if (
                            (varBind[1].value.asnValue.address.stream[0] ==
                             0x00)
                            && (varBind[1].value.asnValue.address.stream[1] ==
                                0x00)
                            && (varBind[1].value.asnValue.address.stream[2] ==
                                0x00)
                            && (varBind[1].value.asnValue.address.stream[3] ==
                                0x00)
                            && (varBind[1].value.asnValue.address.stream[4] ==
                                0x00)
                            && (varBind[1].value.asnValue.address.stream[5] ==
                                0x00)) {

                        /* Ignore NULL addresses returned by other network
                           interfaces */
                        //printf("Interface #%i is a NULL address\n", j);
                        continue;
                    }
                    sprintf(TempEthernet, "%02x%02x%02x%02x%02x%02x",
                            varBind[1].value.asnValue.address.stream[0],
                            varBind[1].value.asnValue.address.stream[1],
                            varBind[1].value.asnValue.address.stream[2],
                            varBind[1].value.asnValue.address.stream[3],
                            varBind[1].value.asnValue.address.stream[4],
                            varBind[1].value.asnValue.address.stream[5]);
                    //printf("MAC Address of interface #%i: %s\n", j, TempEthernet);
                }

                std::string mac="";
                std::stringstream sstream;
                sstream << TempEthernet;
                sstream >> mac;

                macs.insert(mac);
            }
        }
    } while (!ret);         /* Stop only on an error.  An error will occur
                               when we go exhaust the list of interfaces to
                               be examined */
    //getch();

    /* Free the bindings */
    SNMP_FreeVarBind(&varBind[0]);
    SNMP_FreeVarBind(&varBind[1]);
    return 1;
}
#endif

std::set<std::string> FrameworkUtils::listFolder(const std::string &path, bool list_folders )
{
    std::set<std::string> items;
#if defined(FRAMEWORK_PLATFORM_WINDOWS)
    WIN32_FIND_DATAA ffd;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    //    DWORD dwError = 0;
    hFind = FindFirstFileA( path.c_str(), &ffd );
    if ( hFind != INVALID_HANDLE_VALUE )
    {
        do
        {
            if ( (!list_folders && !(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ) || list_folders )
            {
                items.insert( ffd.cFileName );
            }
        }
        while (FindNextFileA( hFind, &ffd ) != 0 );
        FindClose(hFind);
    }
#elif defined(FRAMEWORK_PLATFORM_LINUX)
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
//                if ( (!list_folders && !S_ISDIR(statbuf.st_mode)) ||
//                     list_folders ) original check
                if ( S_ISDIR(statbuf.st_mode) || list_folders )
                {
                    items.insert( dp->d_name );
                }
            }
        }
        closedir(dirp);
    }
#endif
    return items;
}

std::string FrameworkUtils::resolvePath(const std::string &path)
{
#if defined(FRAMEWORK_PLATFORM_WINDOWS)
    return path;
#elif defined(FRAMEWORK_PLATFORM_LINUX)
    char buff[4096] = "";
    ssize_t len = 0;
    if ( (len = readlink( path.c_str(), buff, 4095)) != -1 )
    {
        buff[len] = '\0';
        return buff;
    }
    return path;
#endif
}

bool FrameworkUtils::fileExist(const std::string &filename, bool check_folder)
{
    bool ret = false;
#if defined(FRAMEWORK_PLATFORM_WINDOWS)
    DWORD a = GetFileAttributes( filename.c_str() );
    if ( a != INVALID_FILE_ATTRIBUTES )
    {
        if ( check_folder )
            ret = a & FILE_ATTRIBUTE_DIRECTORY;
        else
            ret = true;
    }
#elif defined(FRAMEWORK_PLATFORM_LINUX)
    std::string fn = filename;
    if ( check_folder )
        fn += "/";
    if ( access( fn.c_str(), R_OK) == 0 )
        ret = true;
#endif
    return ret;
}

bool FrameworkUtils::deleteFile(const std::string &filename)
{
#if defined(FRAMEWORK_PLATFORM_WINDOWS)
    return ::DeleteFile( filename.c_str() ) != 0;
#elif defined(FRAMEWORK_PLATFORM_LINUX)
    return ::unlink( filename.c_str() ) == 0;
#endif
}


std::string FrameworkUtils::getOsArch()
{
	std::string platform_bit = "";
	std::string platform_compiler = "";

#if defined(FRAMEWORK_PLATFORM_LINUX)

#if defined(FRAMEWORK_PLATFORM_64BIT)
    platform_bit = "linux64";
#elif defined(FRAMEWORK_PLATFORM_32BIT)
    platform_bit = "linux32";
#else
    platform_bit = "linux";
    platform_compiler = "-unknown";
#endif // linux 32 or 64

#elif defined(FRAMEWORK_PLATFORM_WINDOWS)

#if defined(FRAMEWORK_PLATFORM_64BIT)
    platform_bit = "win64";
#elif defined(FRAMEWORK_PLATFORM_32BIT)
    platform_bit = "win32";
#else
    platform_bit = "win";
#endif

#if defined(FRAMEWORK_PLATFORM_WINDOWS_MINGW32)
    platform_compiler = "-mingw";
#elif defined(FRAMEWORK_PLATFORM_WINDOWS_VS2010)
    platform_compiler = "-vs2010";
#elif defined(FRAMEWORK_PLATFORM_WINDOWS_VS2013)
    platform_compiler = "-vs2013";
#else
    platform_compiler = "-unknown";
#endif // windows mingw/vs/32/64...

#else // unknown arch!

    platform_bit = "arch";
    platform_compiler = "-unknown";
#error unknown architecture!

#endif
    return platform_bit + platform_compiler;
}

int32_t FrameworkUtils::getPID()
{
    int32_t ret = 0;
#if defined(FRAMEWORK_PLATFORM_WINDOWS)
#if defined(FRAMEWORK_PLATFORM_WINDOWS_MINGW)
    ret = getpid();
#else
    PROCESS_INFORMATION processInfo;
    ret = static_cast<int32_t>( processInfo.dwProcessId );
#endif
#else
    ret = getpid();
#endif
    return ret;
}

int32_t FrameworkUtils::getParentPID()
{
    int32_t ret = 0;
#if defined(FRAMEWORK_PLATFORM_WINDOWS)
    ret = -1;
#else
    ret = getppid();
#endif
    return ret;
}

std::string FrameworkUtils::getDirSep()
{
    // Windows ARCH
#if defined(FRAMEWORK_PLATFORM_WINDOWS)
    return "\\";
#else
    return "/";
#endif
}

bool FrameworkUtils::mkDir(const std::string &filename)
{
    bool ret = false;
#if defined(FRAMEWORK_PLATFORM_WINDOWS)
    ret = ( CreateDirectory( filename.c_str(), NULL ) != 0 );
#elif defined(FRAMEWORK_PLATFORM_LINUX)
    ret = ( ::mkdir( filename.c_str(), 0750 ) == 0);
#endif
    return ret;
}

bool FrameworkUtils::rmDir(const std::string &filename)
{
    bool ret = false;
#if defined(FRAMEWORK_PLATFORM_WINDOWS)
    ret = ( RemoveDirectory( filename.c_str() ) != 0 );
#elif defined(FRAMEWORK_PLATFORM_LINUX)
    ret = ( ::rmdir( filename.c_str() ) == 0);
#endif
    return ret;
}

void FrameworkUtils::string_trim( std::string& original_str )
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

std::string FrameworkUtils::string_trim(const std::string& str )
{
    std::string trimmed = str;
    string_trim( trimmed );
    return trimmed;
}

std::string FrameworkUtils::string_tolower(const std::string& str )
{
    std::string ret = str;
    std::transform( ret.begin(), ret.end(), ret.begin(), ::tolower );
    return ret;
}

std::string FrameworkUtils::string_toupper(const std::string& str )
{
    std::string ret = str;
    std::transform( ret.begin(), ret.end(), ret.begin(), ::toupper );
    return ret;
}

double FrameworkLibrary::FrameworkUtils::string_tof(const std::string &str)
{
    return atof( str.c_str() );
}

int FrameworkUtils::string_toi(const std::string &str)
{
    return atoi( str.c_str() );
}

std::vector<std::string> FrameworkUtils::string_split(const std::string &str, const std::string &sep)
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

std::vector<std::string> FrameworkUtils::vector_shift(const std::vector<std::string> &vector, uint32_t pos)
{
    std::vector<std::string> ret;
    for ( uint32_t p = pos; p < vector.size(); ++p )
        ret.push_back( vector[p] );
    return ret;
}

std::list<std::string> FrameworkUtils::string_split_list(const std::string &str, const std::string &sep)
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

std::set<std::string> FrameworkUtils::string_split_set(const std::string &str, const std::string &sep)
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

std::vector<std::string> FrameworkUtils::parse_csv(const std::string &row, const std::string& sep)
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
                    ret.push_back( /*FrameworkUtils::string_replace(*/ token /*, "\"", "" )*/ );
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
    ret.push_back( /*FrameworkUtils::string_replace(*/ token/*, "\"", "" )*/ );
    return ret;
}

std::string FrameworkUtils::string_replace(const std::string &src, const std::string &token, const std::string &value )
{
    std::string ret_str = "";
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

std::vector<std::string> FrameworkUtils::string_split_quotes(const std::string& str)
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

std::string FrameworkUtils::read_string(FILE* file)
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

void FrameworkUtils::write_string(FILE* file, const std::string &str )
{
    uint32_t size = str.length();
    fwrite( &size, sizeof(size), 1, file );
    fwrite( str.c_str(), size, 1, file );
}

bool FrameworkUtils::isBlank(char c)
{
    return ( c == ' ' ) || ( c == '\t' );
}

double FrameworkUtils::rint(double a)
{
#if defined(FRAMEWORK_PLATFORM_LINUX)
    return ::rint(a);
#else
    const double two_to_52 = 4.5035996273704960e+15;
    double fa = fabs(a);
    double r = two_to_52 + fa;
    if (fa >= two_to_52)
    {
        r = a;
    } else
    {
        r = r - two_to_52;
        r = _copysign(r, a);
    }
    return r;
#endif
}

void FrameworkUtils::print_errno_string(const std::string &msg , const std::string &extra)
{
    std::string errmsg = get_errno_string(extra);
    debugPrintUntagged() << msg << ":" << errmsg << "\n";
}

std::string FrameworkUtils::get_errno_string(const std::string &extra)
{
    std::string err_str;
    char buffer[4096];
    buffer[0] = '\0';
#if defined(FRAMEWORK_PLATFORM_WINDOWS)
    err_str = std::string( strerror( errno ) );
#elif defined(FRAMEWORK_PLATFORM_LINUX)
    err_str = strerror_r(errno, buffer, 4096);
#endif
    sprintf(buffer, "%s (%d)-[%s]\n", err_str.c_str(), errno, extra.c_str() );
    return std::string(buffer);
}

bool FrameworkUtils::file_to_str( std::string name, std::string& str )
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

bool FrameworkUtils::str_to_file( std::string name, std::string str )
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

int FrameworkUtils::simulationSessionId()
{
    int uid = -1;
    std::string tmp = read_env( "FRAMEWORK_USER_ID" );
    if ( tmp != "" )
        uid = atoi( tmp.c_str() );
#if defined(FRAMEWORK_PLATFORM_LINUX)
    else
        uid = getuid();
#endif
    if ( uid < 0 )
    {
        debugPrintError( "simulation_session_id" ) << "ERROR: unable to get an user-id! Please set FRAMEWORK_USER_ID!\n";
        exit(0);
    }
    else if ( uid == 0 )
        debugPrintWarning( "simulation_session_id" ) << "WARNING: running as ROOT is NOT recomended! Continue at your own risk.\n\n\n\n\n\n\n";

    return uid;
}

std::string FrameworkUtils::getCurrentUsername()
{
#if defined(FRAMEWORK_PLATFORM_WINDOWS)
    TCHAR name [ UNLEN + 1 ];
    DWORD size = UNLEN + 1;
    GetUserName((TCHAR*)name, &size);
    std::string str = "";
#ifndef UNICODE
    str = name;
#else
    std::wstring wStr = name;
    str = std::string(wStr.begin(), wStr.end());
#endif
    return str;
#elif defined(FRAMEWORK_PLATFORM_LINUX)
    uid_t uid = geteuid();
    passwd *pw = getpwuid( uid );
    if ( pw )
        return pw->pw_name;
    else
        return "";
#endif
}

std::string FrameworkUtils::simulationSessionIdStr()
{
    std::stringstream ss;
    ss << simulationSessionId();
    return ss.str();
}

std::string FrameworkUtils::read_env(const std::string &str)
{
    const char* ev = getenv( str.c_str() );
    if ( ev != NULL )
        return ev;
    else
        return "";
}

bool FrameworkUtils::check_env(const std::string &str)
{
#if defined(FRAMEWORK_PLATFORM_WINDOWS)
    DWORD bufferSize = 65535; // Limit according to msdn
    std::string buff;
    buff.resize(bufferSize);
    bufferSize = GetEnvironmentVariable( str.c_str(), &buff[0], bufferSize );
    if( bufferSize ==  0 )
    {
        if( GetLastError() == ERROR_ENVVAR_NOT_FOUND )
            return false;			
	}
    //buff.resize(bufferSize);
    return true;
#elif defined(FRAMEWORK_PLATFORM_LINUX)
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
#endif
}

std::string FrameworkUtils::env_replace(const std::string &str)
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

std::string FrameworkUtils::human_readable_number(int64_t number,
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

std::string FrameworkUtils::human_readable_time(int64_t number_seconds, uint64_t milliseconds)
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

void FrameworkUtils::reset_env(const std::string &str)
{
#if defined(FRAMEWORK_PLATFORM_WINDOWS)
    _putenv( (str + "=").c_str() );
#elif defined(FRAMEWORK_PLATFORM_LINUX)
    unsetenv(str.c_str());
#endif
}

void FrameworkUtils::write_env(const std::string &str, const std::string &val)
{
#if defined(FRAMEWORK_PLATFORM_WINDOWS)
    std::string s = str + "=" + val;
    _putenv( s.c_str() );
#elif defined(FRAMEWORK_PLATFORM_LINUX)
    setenv( str.c_str(), val.c_str(), 1 );
#endif
}

std::set<std::string> FrameworkUtils::list_MACs()
{
    std::set<std::string> macs;
#if defined(FRAMEWORK_PLATFORM_WINDOWS)
    getmacs( macs );
#elif defined(FRAMEWORK_PLATFORM_LINUX)
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
        debugPrint( "getMacs", DebugPrint::LICENSE_CLASS ) << "Found MAC: " << mac << " for interface " << interface_name << "\n";
        macs.insert( mac );
    }
    /*
    FILE* fp;
    char output[4096];
    fp = popen("/sbin/ifconfig -a", "r" );
    if ( fp != NULL )
    {
        std::string full_output;
        while ( fgets( output, sizeof(output)-1, fp) != NULL )
            full_output += output;
        pclose(fp);
        full_output = string_tolower( full_output );

        size_t found_pos = 0;
        while ( (found_pos = full_output.find( "hwaddr", found_pos+7 )) != std::string::npos )
        {
            std::string mac = full_output.substr( found_pos+7, 17 );
            string_trim(mac);
            mac = string_replace( mac, ":", "" );
            macs.insert( mac );
        }
    }*/
#endif
    return macs;
}

std::set<std::string> FrameworkUtils::list_IPs()
{
    std::set<std::string> ret;
#if defined(FRAMEWORK_PLATFORM_WINDOWS)
    int i;

    /* Variables used by GetIpAddrTable */
    PMIB_IPADDRTABLE pIPAddrTable;
    DWORD dwSize = 0;
    DWORD dwRetVal = 0;
    IN_ADDR IPAddr;

    /* Variables used to return error message */
    LPVOID lpMsgBuf;

    // Before calling AddIPAddress we use GetIpAddrTable to get
    // an adapter to which we can add the IP.
    pIPAddrTable = (MIB_IPADDRTABLE *) malloc(sizeof (MIB_IPADDRTABLE));

    if (pIPAddrTable) {
        // Make an initial call to GetIpAddrTable to get the
        // necessary size into the dwSize variable
        if (GetIpAddrTable(pIPAddrTable, &dwSize, 0) ==
                ERROR_INSUFFICIENT_BUFFER) {
            free(pIPAddrTable);
            pIPAddrTable = (MIB_IPADDRTABLE *) malloc(dwSize);

        }
        if (pIPAddrTable == NULL) {
            debugPrintError( "Ncfs2" ) << "Memory allocation failed for GetIpAddrTable\n";
            exit(1);
        }
    }
    // Make a second call to GetIpAddrTable to get the
    // actual data we want
    if ( (dwRetVal = GetIpAddrTable( pIPAddrTable, &dwSize, 0 )) != NO_ERROR ) {
        debugPrintUntagged() << "GetIpAddrTable failed with error " << (unsigned int)dwRetVal << "\n";
        if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, dwRetVal, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),       // Default language
                          (LPTSTR) & lpMsgBuf, 0, NULL)) {
            debugPrintUntagged() << "\tError:" << (char*)lpMsgBuf;
            LocalFree(lpMsgBuf);
        }
        exit(1);
    }

    for (i=0; i < (int) pIPAddrTable->dwNumEntries; i++) {
        IPAddr.S_un.S_addr = (u_long) pIPAddrTable->table[i].dwAddr;
        IPAddr.S_un.S_addr = (u_long) pIPAddrTable->table[i].dwMask;
        IPAddr.S_un.S_addr = (u_long) pIPAddrTable->table[i].dwBCastAddr;
        /* specific */
        ret.insert( inet_ntoa(IPAddr) );
    }

    if (pIPAddrTable) {
        free(pIPAddrTable);
        pIPAddrTable = NULL;
    }
#elif defined(FRAMEWORK_PLATFORM_LINUX)
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
#endif
    return ret;
}

std::string FrameworkUtils::getHostname()
{
    std::string hostname = "";
    char host_name[1024];
#if defined(FRAMEWORK_PLATFORM_WINDOWS)
    WSADATA WinsockData;
    WSAStartup(MAKEWORD(2, 0), &WinsockData);
#endif
    if ( gethostname( host_name, 1024 ) == 0 )
        hostname = std::string(host_name);
    return hostname;
}

std::string FrameworkUtils::getExecutablePath()
{
#if defined(FRAMEWORK_PLATFORM_WINDOWS)
    if ( _pgmptr == NULL )
        return "<exec path unknown>";
    else
        return _pgmptr;
#elif defined(FRAMEWORK_PLATFORM_LINUX)
    char result[ PATH_MAX ];
    ssize_t count = readlink( "/proc/self/exe", result, PATH_MAX );
    return std::string( result, (count > 0) ? count : 0 );
#endif
}

std::string FrameworkUtils::getCwd()
{
    char tmp[4096];
    return _getcwd( tmp, 4096 );
}

std::string FrameworkUtils::resolve_hostname(const std::string &hostname)
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

std::string FrameworkUtils::tostring( const int64_t t )
{
    std::ostringstream ss;
    ss << t;
    return ss.str();
}

bool FrameworkUtils::chDir(const std::string &path)
{
    bool ret = false;
#if defined(FRAMEWORK_PLATFORM_WINDOWS)
    ret = ( SetCurrentDirectory( path.c_str() ) != 0 );
#elif defined(FRAMEWORK_PLATFORM_LINUX)
    ret = ( ::chdir( path.c_str() ) == 0);
#endif
    return ret;
}

FrameworkUtils::FrameworkUtils()
{
}
