#ifndef PROFILER_H
#define PROFILER_H

#include <common_defs.h>

#include <string>

namespace FrameworkLibrary {

/**
  @addtogroup DEBUG_HELPERS
  @{
  */

/** @brief provide some tools to profile your code
 *
 * This class help you profile your code, find bottlenecks, identify where and what needs to be optimized to actually
 * increase your code speed.
 *
 * Profiling is disabled by default. When profiling is disabled, your code will run almost unaffected even if you leave all the
 * profiling calls in it.
 *
 * Profiling must be enabled by setting the *FRAMEWORK_PROFILE* environment variable, which can assume the following values:
 * - 0 (or not set): default, profling is totally disabled.
 * - 1: enable profiling.
 *
 * Profiling times will be as accurate as possible. Actual time spent internally to the profiling class (including printouts) is not
 * calculated in the profiled time, but of course will slow down a bit your application. If you do nested profiling (profile a piece of
 * code which will contain more calls to profiling) then the profiling overhead will be counted in the outer profilings.
 *
 * @warning profiling is thread safe. This also means that all profiling printouts are serialized by a mutex, so a performance loss is to
 * be expected while profiling in addition to the obvious time spent to do the actual profiling and printouts.
 *
 * OUTPUT
 * ======
 *
 * You have three possible outputs for the profiling:
 * - screen, over standard output
 * - file on disk, as human-readable text (same output as on screen, but on file)
 * - raw on disk, in CSV format, raw timing data per each profiled item, to be read by a spreadsheet or computer processed.
 *
 * To control these output you can use the following environment variables:
 * - FRAMEWORK_PROFILE_SCREEN: enable (any value but 0, including unset), or disable (0) screen output. Enabled by default.
 * - FRAMEWORK_PROFILE_FILE: output file (default if set to "" or unset: framework_profile.txt). Set to "none" to disable disk output.
 * - FRAMEWORK_PROFILE_RAW: output file (default if set to "" or unset: framework_profile.csv). Set to "none" to disable raw disk output.
 * - FRAMEWORK_PROFILE_SUMMARY: enable summary-only reporting (by default enabled. Set to 0 to enable full output)
 *
 * When SUMMARY only is enabled, only summarized output is printed. If it is disabled, then each profiler will be print on screen or file.
 *
 * FUNCTION PROFILING
 * ==================
 *
 * Profiling the total execution time of a single function is easy. Just call the PROFILE_FUNCTION macro at the very beginning of
 * the function you want to profile. This will add all the needed code (including automatic namespace/class/function name determination).
 *
 * The profiling system will also collect statistics on number of calls and frequency which will be printed at program termination or
 * whenever the Profiler::profilerSummary() static method is called.
 *
 * @code
 * void myFunction()
 * {
 *      PROFILE_FUNCTION;
 *      ... here your code ...;
 * }
 * @endcode
 *
 * CUSTOM PROFILING
 * ================
 *
 * If you need to profile a specific piece of code, you can explictly create a Profiler item. Profiling will start immediately and will
 * terminate wither when you call Profiler::stopTiming() or the item goes out of scope.
 *
 * It is not recomended to create Profiler items in the heap, just create them in the stack.
 *
 * Each Profiler item must have a name, this name shall be unique and will be used to calculate statistics on recurrent calls. So
 * if you need to profile each iteration of a loop, for example, you can just do this:
 * @code
 * for (int i = 0; i < 100; i++ )
 * {
 *     Profiler pfl("My for loop");
 *     ... do your stuff ...
 * }
 * @endcode
 *
 * and it will profiled for each iteration.
 *
 * The profiling system will also collect statistics on number of custom profilings and frequency which will be printed at program termination or
 * whenever the Profiler::profilerSummary() static method is called. The given name will be used in the statistics collection.
 *
 * 
 *
 */
class DLLEXPORT Profiler
{
public:
    /** @brief Print profiling summary both to screen and to disk
     * @since 3.4
     *
     * It's called automatically at program termination, call manually if you need profling info to be
     * printed also at some point BEFORE program termination.
     */
    static void profilerSummary();

    /** @brief Create a profiler with a specific name which will be printed on screen
     * @since 4.0.2 remove "internal" mode.
     * @param name Name of this profiling operation
     */
    Profiler( const std::string& name );

    /** @brief If profiling, terminate and print.
     */
    virtual ~Profiler();

    /** @brief terminate profiling and print
     * @since 3.4
     */
    void stopTiming();

    class __privateProfilerData;
private:
    __privateProfilerData* _pdata;
};

#if defined(FRAMEWORK_PLATFORM_WINDOWS) && !defined(FRAMEWORK_PLATFORM_WINDOWS_MINGW32)
#define PROFILE_FUNCTION FrameworkLibrary::Profiler __function_profiler( __FUNCSIG__ )
#elif defined(FRAMEWORK_PLATFORM_LINUX) || defined(FRAMEWORK_PLATFORM_WINDOWS_MINGW32)
#define PROFILE_FUNCTION FrameworkLibrary::Profiler __function_profiler( __PRETTY_FUNCTION__ )
#endif

/**
  @}
  */

}

#endif // PROFILER_H
