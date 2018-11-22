#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/stat.h>
#include <stdio.h>
#include <sstream>

#include "frameworkutils.h"
#include "cmdlineparser.h"
#include "frameworksighandler.h"
#include "udpsocket.h"
#include "debugprint.h"

#include "runnerthread.h"
#include "command.h"

#ifdef DEMO
#include <demowiringpi.h>
#define DEMO_MODE true
#else
#include <wiringPi.h>
#define DEMO_MODE false
#endif

using namespace FrameworkLibrary;

#if defined(FRAMEWORK_PLATFORM_LINUX)
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
#endif



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
    Command::init();
    DebugPrint::setDefaultTag("PiThermoDaemon");
    int ret = 255;
    CmdlineParser cmd("PiThermo Daemon");
    // "common" options:
    cmd.defineParameter( CmdLineParameter("mode", "Operating mode (main, ext)", CmdLineParameter::single, true )
                         .setOptions(1));
    cmd.defineParameter( CmdLineParameter("daemon", "Start as a daemon", CmdLineParameter::single ) );
    cmd.defineParameter( CmdLineParameter("config", "Config file", CmdLineParameter::single, true )
                         .setOptions(1));
    cmd.defineParameter( CmdLineParameter("logs", "Path to history, logs and debug files", CmdLineParameter::single, true )
                         .setOptions(1));

    // "main" mode options:
    cmd.defineParameter( CmdLineParameter("xchange", "Exchange path", CmdLineParameter::single, false )
                         .setOptions(1));

    // "ext" mode options:
    cmd.defineParameter( CmdLineParameter("remote", "Send data to this remote IP", CmdLineParameter::single, false )
                         .setOptions(1));
    cmd.defineParameter( CmdLineParameter("ext-id", "External sensor ID", CmdLineParameter::single, false )
                         .setOptions(1));

    cmd.setCommandLine( static_cast<uint32_t>(argc), argv );

    if ( cmd.parse() )
    {
        bool allowed_to_start = false;
        bool start_as_daemon = cmd.consumeParameter( "daemon" ).isValid();

        std::string operating_mode = cmd.consumeParameter("mode").getOption();
        std::string config_file = cmd.consumeParameter( "config" ).getOption();
        std::string log_path = cmd.consumeParameter( "logs" ).getOption();

        std::string exchange_path = "";
        std::string remote_host = "";
        std::string ext_id = "";

        if ( FrameworkUtils::fileExist( log_path, true ) )
        {
            if ( operating_mode == "main" )
            {
                exchange_path = cmd.consumeParameter( "xchange" ).getOption();
                if ( FrameworkUtils::fileExist( exchange_path, true ) )
                    allowed_to_start = true;
                else
                    debugPrintError() << "ERROR: invalid exchange path (" << exchange_path << ")\n";
            }
            else if ( operating_mode == "ext" )
            {
                ext_id = cmd.consumeParameter( "ext-id" ).getOption();
                if ( ext_id != "" )
                {
                    remote_host = cmd.consumeParameter( "remote" ).getOption();
                    if ( remote_host != "" )
                        allowed_to_start = true;
                    else
                        debugPrintError() << "ERROR: remote IP not specified\n";
                }
                else
                    debugPrintError() << "ERROR: missing ext-id tag!\n";
            }
            else
                debugPrintError() << "ERROR: invalid operating mode (valid are: main/ext)\n";
        }
        else
            debugPrintError() << "ERROR: invalid log path (" << log_path << ")\n";

        if ( allowed_to_start )
        {
            if (wiringPiSetup () != -1)
            {
                bool enable_debug = false;
                ConfigFile config( config_file );
                enable_debug = config.getValueBool("debug");

                Logger logger( log_path, exchange_path );
                if ( logger.isValid() )
                {
                    logger.enableDebug( enable_debug );
                    logger.logMessage(operating_mode + " start");
                    logger.logEvent( LogItem::START );

#if defined(FRAMEWORK_PLATFORM_LINUX)
                    if ( start_as_daemon )
                        daemonize();
                    else if ( DEMO_MODE )
#endif
                        debugPrintNotice("") << "DemoMode\n";

                    SigHandler sig_handler;
                    FrameworkSigHandler::setHandler( &sig_handler, FrameworkSigHandler::SIGINT_SIGNAL );

                    if ( operating_mode == "main" )
                    {
                        UdpSocket command_server("CommandServer","", "",0,5555);
                        if ( command_server.activateInterface() )
                        {
                            RunnerThread runner( &config, exchange_path, log_path + "/history", &logger);
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
                                logger.logMessage( "Unable to start runner thread!" );
                        }
                        else
                            logger.logMessage( "Unable to open socket!" );
                    }
                    else if ( operating_mode == "ext" )
                    {
                        float temp_correction = static_cast<float>(FrameworkUtils::string_tof( config.getValue( "temp_correction" ) ) );

                        UdpSocket remote_client("RemoteClient",remote_host, "",5555,0);
                        if ( remote_client.activateInterface() )
                        {
                            TempSensor* temp_sensor = new TempSensor( &logger, 1, temp_correction );

                            FrameworkTimer timer;
                            timer.setLoopTime( 30 * 1000 * 1000 );
                            timer.waitLoop();
                            while ( sig_handler.keepRunning() )
                            {
                                if ( temp_sensor->readSensor() )
                                {
                                    std::stringstream ss;
                                    ss << "ext-temp"
                                       << ext_id << ":"
                                       << temp_sensor->getTemp() << ":"
                                       << temp_sensor->getHimidity();
                                    std::string send_data = ss.str();
                                    remote_client.writeData( send_data.c_str(), static_cast<int>( send_data.length() ) );
                                    temp_sensor->printStatus();
                                }
                                // Check for a sensor reading which is too old...
                                uint64_t last_temp_read = temp_sensor->getTimestamp();
                                if ( (FrameworkTimer::getTimeEpoc() - last_temp_read) > 60 )
                                {
                                    // Let's reset the sensor:
                                    delete temp_sensor;
                                    logger.logDebug("Resetting sensor...");
                                    temp_sensor = new TempSensor( &logger, 1, temp_correction );
                                }
                                timer.waitLoop();
                            }
                            delete temp_sensor;
                        }
                        else
                            logger.logMessage( "Unable to open socket!" );
                    }

                    logger.logEvent( LogItem::STOP );
                    logger.logMessage(operating_mode + " stop");
                }
                else
                    debugPrintError() << "Unable to open log file!\n";
            }
            else
                debugPrintError() << "WiringPi initialization error";
        }
    }
    return ret;
}
