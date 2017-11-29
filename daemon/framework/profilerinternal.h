#ifndef PROFILERINTERNAL_H
#define PROFILERINTERNAL_H

#include <common_defs.h>
#include <profiler.h>

#include <string>

namespace FrameworkLibrary {

/**
  @addtogroup DEBUG_HELPERS
  @{
  */

/** @brief provide internal profiling to the Simulation Framework
 *
 * The internal Simulation Framework profiling will enable deep profiling inside the framework function calls and is strongly NOT
 * recomended. It is mostly used for debug of the Simulation Framework and it's not intended to be used by end-users. Anyway, in
 * your build it will most probably be disabled, so enabling it it's useless anyway.
 *
 * To enable internal profiling set FRAMEWORK_PROFILE_INTERNAL = 1.
 * @note that you need a build with internal profiling enabled for this to work.
 *
 */
class DLLEXPORT ProfilerInternal
{
public:
    /** @brief Create the internal profiler
     * @param name name of the profiler
     */
    ProfilerInternal( const std::string& name );
    virtual ~ProfilerInternal();

private:
    class __privateProfilerInternal;
    __privateProfilerInternal* _private;
};

#if defined(FRAMEWORK_PLATFORM_WINDOWS) && !defined(FRAMEWORK_PLATFORM_WINDOWS_MINGW32)
#define PROFILE_INTERNAL FrameworkLibrary::ProfilerInternal __function_profiler( __FUNCSIG__ )
#elif defined(FRAMEWORK_PLATFORM_LINUX) || defined(FRAMEWORK_PLATFORM_WINDOWS_MINGW32)
#define PROFILE_INTERNAL FrameworkLibrary::ProfilerInternal __function_profiler( __PRETTY_FUNCTION__ )
#endif

/**
  @}
  */

}

#endif // PROFILERINTERNAL_H
