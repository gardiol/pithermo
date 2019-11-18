
#include <cmdlineparser.h>
#include <frameworkutils.h>
#include <frameworktimer.h>

#include "gpiodevice.h"

using namespace FrameworkLibrary;

int main(int argc, char** argv)
{
    int ret = 255;
    CmdlineParser cmd("TurtleController");
    cmd.defineParameter( CmdLineParameter("heater_gpio", "GPIO for the heater", CmdLineParameter::single, true )
                         .setOptions(1));
    cmd.defineParameter( CmdLineParameter("light_gpio", "GPIO for the light", CmdLineParameter::single, true )
                         .setOptions(1));

    cmd.setCommandLine( argc, argv );
    if ( cmd.parse() )
    {
        int heater_gpio = FrameworkUtils::string_toi( cmd.consumeParameter("heater_gpio").getOption() );
        int light_gpio = FrameworkUtils::string_toi( cmd.consumeParameter("light_gpio").getOption() );



    }
}
