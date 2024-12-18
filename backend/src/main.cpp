#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/stat.h>
#include <stdio.h>

#include "pithermosighandler.h"

#include "configfile.h"
#include "runnerthread.h"

#ifdef DEMO
#include <demowiringpi.h>
#define DEMO_MODE true
#else
#include <wiringPi.h>
#define DEMO_MODE false
#endif

#include <string>

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

class SigHandler: public PithermoSignal
{
public:
    SigHandler():
        PithermoSignal(),
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
    void customHandler(int32_t  );
    bool _keepRunning;
};

void SigHandler::customHandler(int32_t  )
{
    _keepRunning = false;
}

void printUsage()
{
    printf("Pithermo\n");
    printf("Usage: pithermo_daemon --config=path_to_file --logs=path_to_file\n");
}

int main(int argc, char** argv)
{
    int ret = 255;

    std::string config_file = "./config";
    std::string log_path = "./";

    bool read_config = false;
    bool read_logs = false;
    if ( argc > 1 )
    {
        for ( int i = 1; i < argc; i++ )
        {
            std::string msg = std::string(argv[i]);
            if ( read_config )
            {
                config_file = msg;
                read_config = false;
            }
            if ( read_logs )
            {
                log_path = msg;
                read_logs = false;
            }
            if (  msg == "--config" )
                read_config = true;
            else if ( msg == "--logs"  )
                read_logs = true;

            if ( (msg == "-h") ||
                (msg == "--help" ) )
                printUsage();
        }
    }

    printf("Config file: '%s'\n", config_file.c_str() );
    printf("Log path: '%s'\n", log_path.c_str() );

    if (wiringPiSetup() != -1)
    {
        bool enable_debug = false;
        ConfigFile config( config_file );
        enable_debug = config.getValueBool("debug");

        Logger logger( log_path + "/events" );
        if ( logger.isValid() )
        {
            logger.enableDebug( enable_debug );
            logger.logDebug( "system start" );
            logger.logEvent( LogItem::START );

            if ( DEMO_MODE )
                printf("DemoMode - not daemonizing\n");
            else
                daemonize();

            SigHandler sig_handler;
            PithermoSigHandler::setHandler( &sig_handler );

            RunnerThread runner( &config, log_path + "/history", &logger);
            if ( runner.isRunning() )
            {
                while ( runner.isRunning() &&
                       sig_handler.keepRunning() )
                    PithermoTimer::msleep_s( 500 );
                {
                    ret = 0;
                }
            }
            else
                printf( "Unable to start runner thread!" );

            logger.logEvent( LogItem::STOP );
        }
        else
            printf("logger initialization error\n");
    }
    else
        printf("WiringPi initialization error\n");

    return ret;
}
