#include "profiler.h"

#include <stdio.h>
#include <string.h>
#include <map>
#include <list>

#include "basethread.h"
#include "frameworkutils.h"
#include "debugprint.h"
#include "frameworktimer.h"
#include "basemutex.h"

using namespace FrameworkLibrary;

namespace FrameworkLibrary {

class Profiler::__privateProfilerData
{
public:
    explicit __privateProfilerData( const std::string& name ):
        _name( name ),
        _start_time( FrameworkTimer::getCurrentTime() ),
        _end_time(0),
        _thread_id( BaseThread::getThreadId() )
    {
    }

    inline uint64_t stopProfiling()
    {
        _end_time = FrameworkTimer::getCurrentTime();
        if ( BaseThread::getThreadId() != _thread_id )
        {
            debugPrintError( "Profiler" ) << "ERROR: start and stop profiling called from different threads! (" << _name << ")\n";
            return 0;
        }
        else
            return _end_time - _start_time;
    }

    inline uint64_t getStartTime() const
    {
        return _start_time;
    }

    inline uint64_t getEndTime() const
    {
        return _end_time;
    }

    inline uint64_t getElapsedTime() const
    {
        if ( !isRunning() )
            return _end_time - _start_time;
        else
            return 0;
    }

    inline bool isRunning() const
    {
        return _end_time == 0;
    }

    inline std::string getName() const
    {
        return _name;
    }

    inline BaseThreadId getThreadId() const
    {
        return _thread_id;
    }

private:
    std::string _name;
    uint64_t _start_time;
    uint64_t _end_time;
    BaseThreadId _thread_id;
};

class __profilerPrivate
{
public:
    static __profilerPrivate* _profiler;
    static bool _initialized;

    class __profilerStats
    {
        friend class __profilerPrivate;
    public:
        __profilerStats():
            _min_time(0),
            _max_time(0),
            _avg_time(0),
            _iterations(0)
        {
        }

    private:
        void addIteration( uint64_t elapsed )
        {
            if ( _iterations == 0 )
            {
                _max_time = elapsed;
                _avg_time = elapsed;
                _min_time = elapsed;
                _iterations = 1;
            }
            else
            {
                if ( elapsed < _min_time )
                    _min_time = elapsed;
                if ( elapsed > _max_time )
                    _max_time = elapsed;
                _avg_time = ( _avg_time + elapsed ) / 2;
                _iterations++;
            }
        }

        uint64_t _min_time;
        uint64_t _max_time;
        uint64_t _avg_time;
        uint64_t _iterations;

    };

    static void initialize()
    {
        static __profilerPrivate pvt_singleton;
        _initialized = true;

        int32_t n_outputs = 0;
        std::string raw_file = "";
        std::string output_file = "";

        if ( FrameworkUtils::check_env( "FRAMEWORK_PROFILE" ) )
        {
            std::string enable_profiling = FrameworkUtils::read_env( "FRAMEWORK_PROFILE" );
            if ( (enable_profiling != "") && (enable_profiling != "0") )
            {
                debugPrintNotice( "Profiler" ) << "Environment variable FRAMEWORK_PROFILE set, profiling is ENABLED!\n";

                pvt_singleton._disable_screen = FrameworkUtils::read_env( "FRAMEWORK_PROFILE_SCREEN" ) == "0" ? true : false;
                if ( !pvt_singleton._disable_screen )
                    n_outputs++;

                if ( FrameworkUtils::check_env( "FRAMEWORK_PROFILE_SUMMARY" ))
                {
                    if ( FrameworkUtils::read_env( "FRAMEWORK_PROFILE_SUMMARY" ) == "0" )
                    {
                        debugPrintNotice( "FrameworkInterface" ) << "All profiling data will be printed, FRAMEWORK_PROFILE_SUMMARY is set to 0\n";
                        pvt_singleton._only_summary = false;
                    }
                    else
                        debugPrintNotice( "FrameworkInterface" ) << "Only profiling summary will be printed! If you need all profiling output, set FRAMEWORK_PROFILE_SUMMARY=0\n";
                }
                if ( FrameworkUtils::check_env( "FRAMEWORK_PROFILE_FILE" ))
                {
                    output_file = FrameworkUtils::read_env( "FRAMEWORK_PROFILE_FILE" );
                    if ( output_file == "" )
                    {
                        debugPrintNotice( "FrameworkInterface" ) << "Using default profile output file. You can customize it by setting FRAMEWORK_PROFILE_FILE environment variable.\n";
                        output_file = FrameworkUtils::getCwd() + "/framework_profile.txt";
                    }
                }
                if ( FrameworkUtils::check_env( "FRAMEWORK_PROFILE_RAW" ))
                {
                    raw_file = FrameworkUtils::read_env( "FRAMEWORK_PROFILE_RAW" );
                    if ( raw_file == "" )
                    {
                        debugPrintNotice( "FrameworkInterface" ) << "Using default profile output raw file. You can customize it by setting FRAMEWORK_PROFILE_RAW environment variable.\n";
                        raw_file = FrameworkUtils::getCwd() + "/framework_profile.csv";
                    }
                }

                if ( output_file != "" )
                {
                    pvt_singleton._output_file = fopen( output_file.c_str(), "w" );
                    if ( pvt_singleton._output_file != NULL )
                        n_outputs++;
                    else
                        debugPrintError( "Profiler" ) << "error: unable to open '" << output_file << "' file for writing!\n";
                }
                if ( raw_file != "" )
                {
                    pvt_singleton._raw_file = fopen( raw_file.c_str(), "w" );
                    if ( pvt_singleton._raw_file != NULL )
                    {
                        fprintf( pvt_singleton._raw_file, "time, name, start, end, delta, thread_id\n");
                        n_outputs++;
                    }
                    else
                        debugPrintError( "Profiler" ) << "error: unable to open '" << raw_file << "' file for writing!\n";
                }

                if ( n_outputs == 0 )
                    debugPrintError( "Profiler" ) << "ERROR: screen and file (including raw data) output is DISABLED! This is probably an ERROR!\n";
                else
                    _profiler = &pvt_singleton;

                // We disable the profiling environment variables now... otherwise they get propagated to any child process
                // we start and that will be a bit messy:
                FrameworkUtils::write_env( "FRAMEWORK_PROFILE", "" );
                FrameworkUtils::write_env( "FRAMEWORK_PROFILE_SCREEN", "" );
                FrameworkUtils::write_env( "FRAMEWORK_PROFILE_FILE", "" );
                FrameworkUtils::write_env( "FRAMEWORK_PROFILE_RAW", "" );
                FrameworkUtils::write_env( "FRAMEWORK_PROFILE_SUMMARY", "" );
            }
            else
                debugPrintNotice( "Profiler" ) << "Environment variable FRAMEWORK_PROFILE not set, profiling is DISABLED.\n\n";
        }
    }

    void printSummary()
    {
        char tmpBuff[4096];
        _mutex.lock();
        for ( std::map<std::string, __profilerStats>::iterator m = _data.begin();
              m != _data.end();
              ++m )
        {
            if ( m->second._iterations > 0 )
            {
                sprintf( tmpBuff, "Profiling summary for item: %s\n", m->first.c_str() );
                if ( _output_file != NULL )
                    fwrite( tmpBuff, strlen( tmpBuff ), 1, _output_file );
                if ( !_disable_screen )
                    debugPrintNotice( "[ProfilerSummary]" ) << tmpBuff << "\n";
                sprintf( tmpBuff, "     (Max: %lluus, Min: %lluus) -- AVERAGE: %lluus -- (repeated %llu times)\n",
                         (unsigned long long int)m->second._max_time,
                         (unsigned long long int)m->second._min_time,
                         (unsigned long long int)m->second._avg_time,
                         (unsigned long long int)m->second._iterations );
                if ( _output_file != NULL )
                {
                    fwrite( tmpBuff, strlen( tmpBuff ), 1, _output_file );
                    fflush( _output_file );
                }
                if ( !_disable_screen )
                {
                    debugPrintNotice( "[ProfilerSummary]" ) <<  tmpBuff << "\n";
                    debugPrintNotice( "[ProfilerSummary]" ) << "\n";
                }
            }
        }
        _mutex.unlock();
    }

    __profilerPrivate():
        _mutex("Profiler"),
        _initial_time( FrameworkTimer::getCurrentTime() ),
        _disable_screen(false),
        _only_summary(true),
        _output_file(NULL),
        _raw_file(NULL)
    {
    }

    ~__profilerPrivate()
    {
        printSummary();
        if ( _output_file != NULL )
        {
            fclose( _output_file );
            _output_file = NULL;
        }
        if ( _raw_file != NULL )
        {
            fclose( _raw_file );
            _raw_file = NULL;
        }
        _mutex.lock();
        _data.clear();
        _mutex.unlock();
    }

    void storeData( Profiler::__privateProfilerData* data )
    {
        _mutex.lock();
        std::string name = data->getName();
        std::map<std::string, __profilerStats>::iterator s = _data.find( name );
        if ( s == _data.end() )
        {
            _data.insert( std::pair<std::string, __profilerStats>( name, __profilerStats() ) );
            s = _data.find( name );
        }

        uint64_t elapsed_time = data->getElapsedTime();
        s->second.addIteration( elapsed_time );
        if ( !_only_summary )
        {
            char tmpBuff[4096];
            BaseThreadId thread_id = data->getThreadId();
            uint64_t cur_time = FrameworkTimer::getCurrentTime()-_initial_time;
            sprintf(tmpBuff, "(%llu-%llu) %s (%llu): %lluus\n",
                    (long long unsigned int)(cur_time),
                    (long long unsigned int)(thread_id),
                    name.c_str(),
                    (long long unsigned int)s->second._iterations,
                    (long long unsigned int)elapsed_time );
            if ( _output_file != NULL )
                fprintf( _output_file, "%s", tmpBuff );
            if ( !_disable_screen )
                debugPrintNotice( "[PROFILER]" ) << tmpBuff << "\n";
            if ( _raw_file != NULL )
            {
                sprintf( tmpBuff, "%llu,\"%s\",%llu,%llu,%llu,%llu\n",
                         (long long unsigned int)(cur_time),
                         name.c_str(),
                         (long long unsigned int)(data->getStartTime()-_initial_time),
                         (long long unsigned int)(data->getEndTime()-_initial_time),
                         (long long unsigned int)(elapsed_time),
                         (long long unsigned int)(thread_id) );
                fprintf( _raw_file, "%s", tmpBuff );
            }
        }
        _mutex.unlock();
    }

private:
    BaseMutex _mutex;
    uint64_t _initial_time;
    bool _disable_screen;
    bool _only_summary;
    FILE* _output_file;
    FILE* _raw_file;
    std::map<std::string, __profilerStats> _data;
};

}

bool __profilerPrivate::_initialized = false;
__profilerPrivate* __profilerPrivate::_profiler = NULL;

void Profiler::profilerSummary()
{
    if ( __profilerPrivate::_profiler != NULL )
        __profilerPrivate::_profiler->printSummary();
    else
        debugPrintWarning( "Profiler" ) << "Profiler not enabled. Cannot print summary.\n";
}

Profiler::Profiler(const std::string &name):
    _pdata( NULL )
{
    if ( !__profilerPrivate::_initialized )
        __profilerPrivate::initialize();

    if ( __profilerPrivate::_profiler != NULL )
        _pdata = new __privateProfilerData( name );
}

Profiler::~Profiler()
{
    if ( _pdata != NULL )
    {
        _pdata->stopProfiling();
        __profilerPrivate::_profiler->storeData( _pdata );
        delete _pdata;
        _pdata = NULL;
    }
}

void Profiler::stopTiming()
{
    if ( _pdata != NULL )
    {
        _pdata->stopProfiling();
        __profilerPrivate::_profiler->storeData( _pdata );
        delete _pdata;
        _pdata = NULL;
    }
}
