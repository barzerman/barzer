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


extern "C" void block_ctrlc () 
{
	sigset_t newsigset;
 
   if ((sigemptyset(&newsigset) == -1) ||
       (sigaddset(&newsigset, SIGINT) == -1))
      perror("Failed to initialize the signal set");
   else if (sigprocmask(SIG_BLOCK, &newsigset, NULL) == -1)
      perror("Failed to block SIGINT");
}

int run_shell(barzer::GlobalPools & globPool,  ay::CommandLineArgs &cmdlProc) {
	barzer::BarzerShell shell(0,globPool);;
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

void print_usage(const char* prg_name) {
    std::cerr << "Usage: " << prg_name << " [shell|test [-i <input file> -o <output file>]|server <port>]" << std::endl;
	std::cerr << "Other options: [-anlt] [-cfg <config file>| -cfglist <cfg file list file>]\n";
}


void init_gpools(barzer::GlobalPools &gp, ay::CommandLineArgs &cmdlProc) {
    barzer::BarzerSettings &st = gp.getSettings();
	if( cmdlProc.hasArg("-anlt") ) { /// analytical mode is set
		std::cerr << "RUNNING IN ANALYTICAL MODE!\n";
		gp.setAnalyticalMode();
	}

    bool hasArg = false;
    const char *fname = cmdlProc.getArgVal(hasArg, "-cfglist", 0);
    if (hasArg && fname)
    	st.loadListOfConfigs(fname);
    else {

        fname = cmdlProc.getArgVal(hasArg, "-cfg", 0);
        if (hasArg && fname)
    	    st.load(fname);
        else
    	    st.load();
    }
}

int main( int argc, char * argv[] ) {
	if( argc == 1 ) {
		print_usage( argv[0] );
		return 1;
	}
	AYLOGINIT(DEBUG);
	//ay::Logger::getLogger()->setFile("barzer.log");
	ay::CommandLineArgs cmdlProc;
	cmdlProc.init(argc, argv);
	barzer::GlobalPools globPool;
	barzer::StoredUniverse &universe = globPool.produceUniverse(0);

	init_gpools(globPool, cmdlProc);

	//g_universe = &universe;

	//AYLOGINIT(WARNING);
    try {
        if (argc >= 2) {
            if (strcasecmp(argv[1], "shell") == 0) {
                // ay shell
                //return run_shell(argc, argv);
            	return run_shell(globPool, cmdlProc);
            } else if (strcasecmp(argv[1], "server") == 0) {
                if (argc >= 3) {
                    // port is specified on command line
                    return barzer::run_server(globPool, std::atoi(argv[2]));
                } else {
                    // run on default port
                    return barzer::run_server(globPool, SERVER_PORT);
                }
            } else if (strcasecmp(argv[1], "test") == 0) {
            	return run_test(universe, cmdlProc);
            }
        } 

        // print_usage(argv[0]);
        return 0;

    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    
}
