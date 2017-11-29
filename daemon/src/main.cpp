#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/stat.h>
#include <stdio.h>

#include "cmdlineparser.h"
#include "frameworksighandler.h"
#include "runnerthread.h"
#include "udpsocket.h"
#include "debugprint.h"

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
    for (fd = sysconf(_SC_OPEN_MAX); fd > 0; fd--) {
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
    void customHandler(FrameworkSigHandler::signal_type type,
                       int32_t user_defined_signal )
    {

    }

    bool _keepRunning;
};

int main(int argc, char** argv)
{
    DebugPrint::setDefaultTag("PiThermoDaemon");
    int ret = 255;
    CmdlineParser cmd("PiThermo Daemon");
    cmd.defineParameter( CmdLineParameter("daemon", "Start as a daemon", CmdLineParameter::single ) );
    cmd.setCommandLine( argc, argv );

    if ( cmd.parse() )
    {
        if ( cmd.consumeParameter( "daemon" ).isValid() )
            daemonize();

        SigHandler sig_handler;
        FrameworkSigHandler::setHandler( &sig_handler, FrameworkSigHandler::SIGINT_SIGNAL );

        UdpSocket command_server("CommandServer","", "127.0.0.1",0,5555);
        if ( command_server.activateInterface() )
        {
            RunnerThread runner;
            runner.startThread();
            if ( runner.isRunning() )
            {
                while ( runner.isRunning() &&
                        sig_handler.keepRunning() &&
                        command_server.isActive() )
                {
                    if ( command_server.waitForIncomingData(1000) )
                    {
                        uint32_t frame_size = 0;
                        char* frame = command_server.getFrame(frame_size);
                        ...;
                        delete [] frame;
                    }
                }
                ret = 0;
            }
            else
                debugPrintError() << "Unable to start runner thread!\n";
        }
        else
            debugPrintError() << "Unable to open socket!\n";
    }
    return ret;
}
