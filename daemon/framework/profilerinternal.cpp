#include "profilerinternal.h"

#include "frameworkutils.h"
#include "debugprint.h"

using namespace FrameworkLibrary;

namespace FrameworkLibrary {

class ProfilerInternal::__privateProfilerInternal: public Profiler
{
public:

    explicit __privateProfilerInternal( const std::string& name ):
        Profiler(name)
    {
    }

    virtual ~__privateProfilerInternal()
    {
    }

    static bool internal_profile_enabled;
    static bool internal_profile_initialized;
};

bool ProfilerInternal::__privateProfilerInternal::internal_profile_enabled = false;
bool ProfilerInternal::__privateProfilerInternal::internal_profile_initialized = false;

}

ProfilerInternal::ProfilerInternal(const std::string &name):
    _private( NULL )
{
#if defined(PROFILE_FRAMEWORK)
    if ( !ProfilerInternal::__privateProfilerInternal::internal_profile_initialized )
    {
        ProfilerInternal::__privateProfilerInternal::internal_profile_initialized = true;
        if ( FrameworkUtils::check_env( "FRAMEWORK_PROFILE_INTERNAL" ) )
        {
            if ( FrameworkUtils::read_env( "FRAMEWORK_PROFILE_INTERNAL" ) == "1" )
            {
                debugPrintNotice( "ProfilerInternal" ) << "Environment variable FRAMEWORK_PROFILE_INTERNAL set, internal profiling is ENABLED!\n";
                ProfilerInternal::__privateProfilerInternal::internal_profile_enabled = true;
            }
        }
    }
    if ( ProfilerInternal::__privateProfilerInternal::internal_profile_enabled )
        _private = new __privateProfilerInternal( name );
#endif
}

ProfilerInternal::~ProfilerInternal()
{
    if ( _private != NULL )
    {
        delete _private;
        _private = NULL;
    }
}
