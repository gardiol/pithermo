#ifndef BASESEMAPHOR_H
#define BASESEMAPHOR_H

#include "common_defs.h"

#include <string>

namespace FrameworkLibrary {

/** @brief Implements a semaphor for thread synchronization
 *
 * This is a classic broadcast semaphor.
 * You can wait for it with waitForSignal(), and activate all the waiting threads with signal().
 *
 * @since 4.0.3
 *
 * @include example_cpp_semaphor.cpp
 *
 */
class DLLEXPORT BaseSemaphor
{
public:
       BaseSemaphor( const std::string& name );
       virtual ~BaseSemaphor();

       void waitForSignal();

       void signal();

private:
       class __private_BaseSemaphor;
       __private_BaseSemaphor* _private;

};

}

#endif // BASESEMAPHOR_H
