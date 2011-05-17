#include <barzer_shell.h>
#include <barzer_server.h>
#include <ay/ay_cmdproc.h>
#include <iostream>
#include <stdlib.h>
#include <strings.h>
#include <signal.h>
#include <ay/ay_logger.h>

#include <barzer_config.h>
#include <barzer_universe.h>

/*
namespace {
static barzer::StoredUniverse*  g_universe = 0;
} //*/

extern "C" void block_ctrlc () 
{
	sigset_t newsigset;
 
   if ((sigemptyset(&newsigset) == -1) ||
       (sigaddset(&newsigset, SIGINT) == -1))
      perror("Failed to initialize the signal set");
   else if (sigprocmask(SIG_BLOCK, &newsigset, NULL) == -1)
      perror("Failed to block SIGINT");
}
/*
struct TaskEnv {
    ay::CommandLineArgs cmdlProc;
    barzer::BarzerShell        barzerShell;
    
	TaskEnv() : barzerShell(0){}
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
//*/


int run_shell(barzer::StoredUniverse &u, ay::CommandLineArgs &cmdlProc) {
	barzer::BarzerShell shell;
	shell.setUniverse(&u);
	if (int rc = shell.run()) {
		std::cerr  << "FATAL: Execution failed\n";
		exit(rc);
	}
	return 0;
}

int process_input(barzer::StoredUniverse &u, ay::InputLineReader &reader, std::ostream &out) {
	using namespace barzer;

	Barz barz;
	QParser parser(u);
	QuestionParm qparm;
	BarzStreamerXML bs(barz, u);

	out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
	out << "<testset>\n";
	while( reader.nextLine() && reader.str.length() ) {
		const char* q = reader.str.c_str();
		out << "<test>\n";
		out << "    <query>" << q << "</query>\n";
		parser.parse( barz, q, qparm );
		bs.print(out);
		out << "</test>\n";
	}
	out << "</testset>\n";
	return 0;
}

int run_test(barzer::StoredUniverse &u, ay::CommandLineArgs &cmdlProc) {
	std::ofstream ofile;
	std::ostream *out = &std::cout;

	std::stringstream ifname;

	bool hasArg = false;
	const char *fname = cmdlProc.getArgVal(hasArg, "-i", 0);
	if (hasArg && fname) ifname << fname;

	ay::InputLineReader reader(ifname);

	fname = cmdlProc.getArgVal(hasArg, "-o", 0);
	if (hasArg && fname) {
		ofile.open(fname);
		out = &ofile;
	}
	return process_input(u, reader, *out);
}

/*
int run_shell(int argc, char * argv[]) {
    TaskEnv env;
	if( !g_universe )
		g_universe = new barzer::StoredUniverse();
	env.barzerShell.setUniverse( g_universe );

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
*/

void print_usage(const char* prg_name) {
    std::cerr << "Usage: " << prg_name << " [shell|test [-i <input file> -o <output file>]|server <port>]" << std::endl;
}


void init_universe(barzer::StoredUniverse &u, ay::CommandLineArgs &cmdlProc) {
    barzer::BarzerSettings &st = u.getSettings();

    bool hasArg = false;
    const char *fname = cmdlProc.getArgVal(hasArg, "-cfg", 0);

    if (hasArg && fname)
    	st.load(fname);
    else
    	st.load();
}

int main( int argc, char * argv[] ) {
	AYLOGINIT(DEBUG);
	ay::Logger::getLogger()->setFile("barzer.log");
	ay::CommandLineArgs cmdlProc;
	cmdlProc.init(argc, argv);
	barzer::GlobalPools globPool;
	barzer::StoredUniverse universe(globPool);

	init_universe(universe, cmdlProc);

	//g_universe = &universe;

	//AYLOGINIT(WARNING);
    try {
        if (argc >= 2) {
            if (strcasecmp(argv[1], "shell") == 0) {
                // ay shell
                //return run_shell(argc, argv);
            	return run_shell(universe, cmdlProc);
            } else if (strcasecmp(argv[1], "server") == 0) {
                if (argc >= 3) {
                    // port is specified on command line
                    return barzer::run_server(universe, std::atoi(argv[2]));
                } else {
                    // run on default port
                    return barzer::run_server(universe, SERVER_PORT);
                }
            } else if (strcasecmp(argv[1], "test") == 0) {
            	return run_test(universe, cmdlProc);
            }
        }

        print_usage(argv[0]);
        return 0;

    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    
}
