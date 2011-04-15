#include <barzer_shell.h>
#include <barzer_server.h>
#include <ay/ay_cmdproc.h>
#include <iostream>
#include <stdlib.h>
#include <strings.h>
#include <signal.h>
#include "ay/ay_logger.h"

#include "barzer_config.h"


extern "C" void block_ctrlc () 
{
	sigset_t newsigset;
 
   if ((sigemptyset(&newsigset) == -1) ||
       (sigaddset(&newsigset, SIGINT) == -1))
      perror("Failed to initialize the signal set");
   else if (sigprocmask(SIG_BLOCK, &newsigset, NULL) == -1)
      perror("Failed to block SIGINT");
}

struct TaskEnv {
    ay::CommandLineArgs cmdlProc;
    barzer::BarzerShell        barzerShell;
    
    int init(int argc, char* argv[]);
    int run();
};

int TaskEnv::init( int argc, char* argv[] )
{
    cmdlProc.init( argc, argv );
    return 0;
}
int TaskEnv::run( )
{
    return barzerShell.run();
}

int run_shell(int argc, char * argv[]) {
    TaskEnv env;
	// block_ctrlc();
    int rc = env.init( argc, argv );
    if( rc ) {
      std::cerr << "FATAL: Initialization failed.. exiting\n";
      exit(rc);
    }

    rc= env.run();
    if( rc ) {
        std::cerr  << "FATAL: Execution failed\n";
        exit(rc);
    }
	
    return rc;
}

void print_usage(const char* prg_name) {
    std::cerr << "Usage: " << prg_name << " [shell|server <port>]" << std::endl;
}

int main( int argc, char * argv[] ) {
	//ay::Logger::init(ay::Logger::WARNING);
	//AYLOGINIT(DEBUG);
	AYLOGINIT(WARNING);
    try {
        if (argc >= 2) {
            if (strcasecmp(argv[1], "shell") == 0) {
                // ay shell
                return run_shell(argc, argv);
            } else if (strcasecmp(argv[1], "server") == 0) {
                if (argc >= 3) {
                    // port is specified on command line
                    return barzer::run_server(std::atoi(argv[2]));
                } else {
                    // run on default port
                    return barzer::run_server(SERVER_PORT);
                }
            }
        }

        print_usage(argv[0]);
        return 0;

    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    
}
