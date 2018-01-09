#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/stat.h>
#include <stdio.h>

#include "frameworkutils.h"
#include "cmdlineparser.h"
#include "frameworksighandler.h"
#include "udpsocket.h"
#include "debugprint.h"

#include "runnerthread.h"
#include "command.h"

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
    void customHandler(FrameworkSigHandler::signal_type ,
                       int32_t  )
    {
        _keepRunning = false;
    }

    bool _keepRunning;
};

int main(int argc, char** argv)
{
    Command::init();
    DebugPrint::setDefaultTag("PiThermoDaemon");
    int ret = 255;
    CmdlineParser cmd("PiThermo Daemon");
    cmd.defineParameter( CmdLineParameter("daemon", "Start as a daemon", CmdLineParameter::single ) );
    cmd.defineParameter( CmdLineParameter("config", "Config file", CmdLineParameter::single, true )
                         .setOptions(1));
    cmd.defineParameter( CmdLineParameter("xchange", "Exchange path", CmdLineParameter::single, true )
                         .setOptions(1));
    cmd.defineParameter( CmdLineParameter("history", "History file", CmdLineParameter::single, true )
                         .setOptions(1));
    cmd.defineParameter( CmdLineParameter("log", "log file", CmdLineParameter::single, true )
                         .setOptions(1));

    cmd.setCommandLine( argc, argv );

    if ( cmd.parse() )
    {
        std::string config_file = cmd.consumeParameter( "config" ).getOption();
        std::string exchange_path = cmd.consumeParameter( "xchange" ).getOption();
        std::string history_file = cmd.consumeParameter( "history" ).getOption();
        std::string log_file = cmd.consumeParameter( "log" ).getOption();
        Logger logger( log_file );
        if ( logger.isValid() )
        {
            if ( FrameworkUtils::fileExist( exchange_path ) )
            {
                if ( cmd.consumeParameter( "daemon" ).isValid() )
                    daemonize();

                SigHandler sig_handler;
                FrameworkSigHandler::setHandler( &sig_handler, FrameworkSigHandler::SIGINT_SIGNAL );

                UdpSocket command_server("CommandServer","", "127.0.0.1",0,5555);
                if ( command_server.activateInterface() )
                {
                    RunnerThread runner(config_file, exchange_path, history_file, &logger);
                    if ( runner.isRunning() )
                    {
                        while ( runner.isRunning() &&
                                sig_handler.keepRunning() &&
                                command_server.isActive() )
                        {
                            if ( command_server.waitForIncomingData(10*1000) )
                            {
                                uint32_t frame_size = 0;
                                char* frame = static_cast<char*>(command_server.getFrame(frame_size));
                                Command* cmd = new Command( frame, frame_size );
                                runner.appendCommand( cmd );
                                delete [] frame;
                            }
                        }
                        ret = 0;
                    }
                    else
                        logger.logEvent( "Unable to start runner thread!" );
                }
                else
                    logger.logEvent( "Unable to open socket!" );
            }
            else
                logger.logEvent( "Exchange path does not exist!" );
        }
        else
            debugPrintError() << "Unable to open log file!\n";
    }
    return ret;
}
