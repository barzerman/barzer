/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#include <arch/barzer_arch.h>
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
#include <barzer_server_response.h>
#include <barzer_relbits.h>
#include <barzer_http.h>


#if  defined(_WINDOWS_)
extern "C" void block_ctrlc () 
{
	sigset_t newsigset;
 
   if ((sigemptyset(&newsigset) == -1) ||
       (sigaddset(&newsigset, SIGINT) == -1))
      perror("Failed to initialize the signal set");
   else if (sigprocmask(SIG_BLOCK, &newsigset, NULL) == -1)
      perror("Failed to block SIGINT");
}
#endif
int run_shell(barzer::GlobalPools & globPool,  ay::CommandLineArgs &cmdlProc) {
	barzer::BarzerShell shell(0,globPool);
    bool hasArg = false;
    const char* runScript = cmdlProc.getArgVal(hasArg, "-exec", 0);
	if (int rc = shell.run(runScript)) {
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
	BarzStreamerXML bs(barz, u,qparm);

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

#ifndef BARZER_VERSION_STR
    #define BARZER_VERSION_STR "UNKNOWN VERSION"
#endif

void print_usage(const char* prg_name) {
	std::cerr << "Barzer " << BARZER_VERSION_STR << "\n" << std::endl;
	std::cerr << "Usage: " << prg_name << " [shell|test [-i <input file> -o <output file>]|server <port>]" << std::endl;
	std::cerr << "Other options: [-anlt] [-home BarzerHomeDir] [-cfg <config file>| -cfglist <cfg file list file>]\n";
}


////////// MAIN
int main( int argc, char * argv[] ) {
	if( argc == 1 ) {
		print_usage( argv[0] );
		exit(1);
	} else if( argc== 2 && !strcasecmp("-v",argv[1]) ) {
        std::cerr << "Barzer " << BARZER_VERSION_STR << std::endl;
        exit(0);
    }
	AYLOGINIT(DEBUG);

    const char *barzerHome = std::getenv("BARZER_HOME");
    std::string relbitsFile;
    if( barzerHome ) 
        relbitsFile.assign(barzerHome);
    if( relbitsFile.size() && relbitsFile[ relbitsFile.size()-1 ] != '/' ) {
        relbitsFile.push_back( '/' );
    }
    relbitsFile.append( "relbits.txt" );
    std::cerr << "loading release bits from " << relbitsFile << std::endl; 
    barzer::RelBitsMgr::inst().reparse( std::cerr, relbitsFile.c_str() );

	//ay::Logger::getLogger()->setFile("barzer.log");
	ay::CommandLineArgs cmdlProc;
	cmdlProc.init(argc, argv);
	barzer::GlobalPools globPool;
	ay::StemThreadPool::inst().createThreadStemmer(pthread_self());

    globPool.init_cmdline( cmdlProc );

	//g_universe = &universe;

	//AYLOGINIT(WARNING);
    try {
        if (argc >= 2) {
            if (strcasecmp(argv[1], "shell") == 0) {
                // ay shell
                //return run_shell(argc, argv);
            	return run_shell(globPool, cmdlProc);
            } else if (strcasecmp(argv[1], "server") == 0) {
                return barzer::run_server(globPool, ( argc>2 ? std::atoi(argv[2]):  SERVER_PORT) );
            } else if (strcasecmp(argv[1], "http") == 0) {
                barzer::BarzerHttpServer::mkInstance( globPool );
                return barzer::BarzerHttpServer::run(cmdlProc, argc, argv );
            } else if (strcasecmp(argv[1], "test") == 0) {
            	return run_test(globPool.produceUniverse(0), cmdlProc);
            }
            else
			{
				AYLOG(WARNING) << "no run mode given, falling back to shell";
				return run_shell(globPool, cmdlProc);
			}
        } 

        // print_usage(argv[0]);
        return 0;

    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    
}
