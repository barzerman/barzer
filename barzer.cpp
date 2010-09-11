#include <barzer_shell.h>
#include <barzer_server.h>
#include <ay/ay_cmdproc.h>
#include <iostream>


struct TaskEnv {
	ay::CommandLineArgs cmdlProc;
	barzer::Shell        barzerShell;

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


int main( int argc, char * argv[] )
{
	TaskEnv env;
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
