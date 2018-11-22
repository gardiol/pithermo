#ifndef COMMON_DEFS_H
#define COMMON_DEFS_H

#undef FRAMEWORK_PLATFORM_32BIT
#undef FRAMEWORK_PLATFORM_64BIT


// Remember that _WIN32 MUST be defined both for 32 and 64bit windows versions.
#if defined(_WIN32)

//**********************************************
// ********** Windows specific defines *********
//**********************************************

#define FRAMEWORK_PLATFORM_WINDOWS

// Required for further usage in the code, since sometimes WIN32 might not be defined at all.
#if !defined(WIN32)
#define WIN32
#endif

// Avoid issues with circular includes inside the windows.h...
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

// Include Windows API's
#include <windows.h>

#define DLLEXPORT __declspec(dllexport)
#define DEPRECATED_DEF __declspec(deprecated)

// MinGW is a GCC, so this syntax is GCC style:
#if defined(__MINGW32__)
//**********************************************
// ********** Mingw32 specific defines *********
//**********************************************

#define FRAMEWORK_PLATFORM_32BIT
#define FRAMEWORK_PLATFORM_WINDOWS_MINGW
#define FRAMEWORK_PLATFORM_WINDOWS_MINGW32

#define ____func_def__ __func__
#define ____pretty_func_def__ __PRETTY_FUNCTION__

#else // Visual Studio syntax
//**********************************************
// ****** Visual Studio specific defines *******
//**********************************************

#if defined(_WIN64)
#define FRAMEWORK_PLATFORM_64BIT
#else
#define FRAMEWORK_PLATFORM_32BIT
#endif

#define ____func_def__ __FUNCTION__
#define ____pretty_func_def__ __FUNCSIG__

// Definition of Visual Studio version
#if defined(_MSC_VER) && _MSC_VER<=1310
#define FRAMEWORK_PLATFORM_WINDOWS_VS_TOO_OLD
#elif defined(_MSC_VER) && _MSC_VER==1400
#define FRAMEWORK_PLATFORM_WINDOWS_VS2005
#elif defined(_MSC_VER) && _MSC_VER==1500
#define FRAMEWORK_PLATFORM_WINDOWS_VS2008
#elif defined(_MSC_VER) && _MSC_VER==1600
#define FRAMEWORK_PLATFORM_WINDOWS_VS2010
#elif defined(_MSC_VER) && _MSC_VER==1700
#define FRAMEWORK_PLATFORM_WINDOWS_VS2012
#elif defined(_MSC_VER) && _MSC_VER==1800
#define FRAMEWORK_PLATFORM_WINDOWS_VS2013
#elif defined(_MSC_VER) && _MSC_VER==1900
#define FRAMEWORK_PLATFORM_WINDOWS_VS2015
#endif

#if defined(FRAMEWORK_PLATFORM_WINDOWS_VS2015)
#if !defined(HAVE_STRUCT_TIMESPEC)
#define HAVE_STRUCT_TIMESPEC
#endif
#endif

#if defined(RTX64)
#define FRAMEWORK_PLATFORM_WINDOWS_RTX64
#endif

#endif // which windows compiler


#elif defined(__linux__)
//**********************************************
// ********** Linux specific defines *********
//**********************************************

#define FRAMEWORK_PLATFORM_LINUX

#define DLLEXPORT __attribute__((visibility ("default")))
#define DEPRECATED_DEF __attribute__((deprecated))
#define ____func_def__ __func__
#define ____pretty_func_def__ __PRETTY_FUNCTION__

#if defined(__x86_64__)
#define FRAMEWORK_PLATFORM_64BIT
#else
#define FRAMEWORK_PLATFORM_32BIT
#endif

#if defined( XENOMAI )
#define FRAMEWORK_PLATFORM_XENOMAI
#endif

#elif defined(__ANDROID__)
//**********************************************
// ********** Linux specific defines *********
//**********************************************

#define FRAMEWORK_PLATFORM_ANDROID
#define FRAMEWORK_PLATFORM_32BIT
// TODO


#endif // which operating system

#include <stdint.h>

#ifdef __cplusplus
/** @brief The Framework Library
 *
 * This namespace includes the entire librframework
 * 
 */
namespace FrameworkLibrary {}
#endif


// Test facilities and requirement tracing
#define TESTPOINT(req)
#define TESTCOVER(arg)

#endif // COMMON_DEFS_H

