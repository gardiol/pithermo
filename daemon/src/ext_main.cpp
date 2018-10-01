#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>

#include "frameworkutils.h"
#include "cmdlineparser.h"
#include "frameworksighandler.h"
#include "udpsocket.h"
#include "debugprint.h"

#include "runnerthread.h"
#include "command.h"

#include "tempsensor.h"

#ifndef DEMO
#include <wiringPi.h>
#endif

using namespace FrameworkLibrary;

static void daemonize()
{
    pid_t pid = 0;
    int fd;

    /* Fork off the parent process */
    pid = fork();

    /* An error occurred */
    if (pid < 0) {
        exit(EXIT_FAILURE);
    }

    /* Success: Let the parent terminate */
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    /* On success: The child process becomes session leader */
    if (setsid() < 0) {
        exit(EXIT_FAILURE);
    }

    /* Ignore signal sent from child to parent process */
    signal(SIGCHLD, SIG_IGN);

    /* Fork off for the second time*/
    pid = fork();

    /* An error occurred */
    if (pid < 0) {
        exit(EXIT_FAILURE);
    }

    /* Success: Let the parent terminate */
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    /* Set new file permissions */
    umask(0);

    /* Change the working directory to the root directory */
    /* or another appropriated directory */
    chdir("/");

    /* Close all open file descriptors */
    for (fd = static_cast<int>(sysconf(_SC_OPEN_MAX)); fd > 0; fd--) {
        close(fd);
    }

    /* Reopen stdin (fd = 0), stdout (fd = 1), stderr (fd = 2) */
    stdin = fopen("/dev/null", "r");
    stdout = fopen("/dev/null", "w+");
    stderr = fopen("/dev/null", "w+");
}

class SigHandler: public FrameworkSignal
{
public:
    SigHandler():
        FrameworkSignal(),
        _keepRunning(true)
    {
    }

    virtual ~SigHandler()
    {
    }

    bool keepRunning() const
    {
        return _keepRunning;
    }

private:
    void customHandler(FrameworkSigHandler::signal_type ,
                       int32_t  );
    bool _keepRunning;
};

void SigHandler::customHandler(FrameworkSigHandler::signal_type ,
                   int32_t  )
{
    _keepRunning = false;
}

int main(int argc, char** argv)
{
    DebugPrint::setDefaultTag("PiThermoExtTemp");
    int ret = 255;
    CmdlineParser cmd("PiThermo ExtTemp");
    cmd.defineParameter( CmdLineParameter("daemon", "Start as a daemon", CmdLineParameter::single ) );
    cmd.defineParameter( CmdLineParameter("debug", "Debug on", CmdLineParameter::single ) );
    cmd.defineParameter( CmdLineParameter("remote", "Send data to this remote IP", CmdLineParameter::single, true )
                         .setOptions(1));
    cmd.defineParameter( CmdLineParameter("logs", "Path to history, logs and debug files", CmdLineParameter::single, true )
                         .setOptions(1));

    cmd.setCommandLine( static_cast<uint32_t>(argc), argv );

    if ( cmd.parse() )
    {
        std::string remote_host = cmd.consumeParameter( "remote" ).getOption();
        std::string log_path = cmd.consumeParameter( "logs" ).getOption();
        Logger logger( log_path, "" );
        if ( logger.isValid() )
        {
#ifdef DEMO
            debugPrintNotice("") << "DemoMode\n";
            if ( true )
#else
            if (wiringPiSetup () != -1)
#endif
            {
                TempSensor temp_sensor( &logger, 1, 0 );

                logger.logMessage("Started");
                if ( cmd.hasParameter( "debug" ) )
                    logger.enableDebug(true);
                if ( cmd.consumeParameter( "daemon" ).isValid() )
                    daemonize();

                SigHandler sig_handler;
                FrameworkSigHandler::setHandler( &sig_handler, FrameworkSigHandler::SIGINT_SIGNAL );

                UdpSocket remote_client("RemoteClient",remote_host, "",5555,0);
                if ( remote_client.activateInterface() )
                {
                    char send_data[2048];
                    memcpy( send_data, "ext-temp", 9 );
                    FrameworkTimer timer;
                    timer.setLoopTime( 30 * 1000 * 1000 );
                    while ( sig_handler.keepRunning() )
                    {
                        if ( temp_sensor.readSensor() )
                        {
                            sprintf( &send_data[8], "%f:%f",
                                    temp_sensor.getTemp(),
                                    temp_sensor.getHimidity() );
                            remote_client.writeData( send_data, static_cast<int>(strlen( send_data )) );
                        }
                        temp_sensor.printStatus();
                        timer.waitLoop();
                    }
                }
                else
                    logger.logMessage( "Unable to open socket!" );

                logger.logMessage("Stopped");
            }
#ifndef DEMO
            else
                debugPrintError() << "WiringPi initialization error";
#endif
        }
        else
            debugPrintError() << "Unable to open log file!\n";
    }
    return ret;
}
