#include <ay_shell.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <ay/ay_cmdproc.h>


namespace ay {

typedef Shell::CmdData CmdData;

namespace {

int Shell_cmd_wait( Shell* sh, char_cp cmd, std::istream& in, const std::string&argStr )
{
    for(auto &i : sh->bgThreads()) { 
        i->join();
        delete i;
    }
    sh->bgThreads().clear();
    return 0;
}
int Shell_cmd_echo( Shell* sh, char_cp cmd, std::istream& in, const std::string&argStr )
{
    sh->getOutStream()  << argStr << std::endl;
    return 0;
}
int Shell_cmd_system( Shell* sh, char_cp cmd, std::istream& in, const std::string&argStr )
{
    system( argStr.c_str() );
    return 0;
}

} // end of anon namespace

static const CmdData g_cmd[] = {
	CmdData( Shell::cmd_help, "help", "get help on a function" ),
	CmdData( Shell::cmd_exit, "exit", "exit the shell" ),
	CmdData( Shell::cmd_quit, "quit", "exit current barzer shell" ),
	CmdData( Shell::cmd_run, "run", "run scripts" ),
	CmdData( Shell_cmd_wait, "wait", "waits for all background processes to complete (joins them)" ),
	CmdData( Shell::cmd_set, "set", "sets parameters" ),

	CmdData( Shell_cmd_system, "!", "runs command in system shell" ),
	CmdData( Shell_cmd_echo, "echo", "echo-s a string" )
};

std::ostream& Shell::CmdData::print( std::ostream& fp ) const
{
	return( fp << name << " - " << desc );
}

int Shell::cmd_set( Shell* sh, char_cp cmd, std::istream& in, const std::string&argStr  )
{
	std::cerr << "Shell::cmd_set unimplemented\n";
	return 0;
}


int Shell::cmd_run( Shell* sh, char_cp cmd, std::istream& in, const std::string&argStr )
{
    ay::CmdLineArgsContainer arg( cmd, in );

	if( arg.argv().size() < 2) {
		std::cerr << "must specify valid file name\n";
		return 0;
	}
	std::string fName( arg.argv()[1] );
	std::ifstream fp;
	fp.open( fName.c_str() ) ;
	if( !fp.is_open() )  {
		std::cerr << "couldnt open " << fName <<  "for reading\n";
		return 0;
	}

	Shell * newShell = sh->cloneShell();
	newShell->init();
	int rc = newShell->runCmdLoop(&fp);
	delete newShell;

	return rc;
}
int Shell::cmd_help( Shell* sh, char_cp cmd, std::istream& in, const std::string&argStr )
{
	std::string srchStr;
	std::getline( in, srchStr );
	char_cp srch = srchStr.c_str();
	for( CmdDataMap::const_iterator i = sh->cmdMap.begin(); i!= sh->cmdMap.end(); ++i ) {
		const char *name = i->second->name;
		const char *desc = i->second->desc;
		if( strstr( name, srch ) || strstr( desc, srch )) {
			std::cout << *(i->second) << std::endl;
		}
	}
	return 0;
}

int Shell::cmd_quit( Shell*, char_cp cmd, std::istream& in, const std::string&argStr ) { return -1; }
int Shell::cmd_exit( Shell*, char_cp cmd, std::istream& in, const std::string&argStr ) { return -1; }

int Shell::printPrompt()
{
    if( !d_bit.check( SHBIT_NOPROMPT ) ) 
	    getErrStream() << "cmd>";
	return 0;
}

std::istream& Shell::setInFile( const char* fn)
{
	if( inStream && inStream != &std::cin ) {
		delete inStream;
		inStream = &std::cin;
	} 
	
    if( fn ) {
        inF.assign(fn);
	    if( inF.length() ) {
		    std::ifstream* tmp = new std::ifstream();
		    tmp->open( inF.c_str() );
            if( !tmp->is_open() ) {
                getErrStream() << "failed to open input file " << fn << std::endl;
            } else
		        inStream = tmp;
	    }
    }
    return *inStream;
}
std::ostream& Shell::setOutFile( const char* fn)
{
	if( outStream && outStream != & std::cout )  {
		delete outStream;
		outStream = &std::cout;
	} 
	
    if( fn ) {
        outF.assign(fn);
	    if( outF.length() ) {
		    std::ofstream* tmp = new std::ofstream();
		    tmp->open( outF.c_str() );
            if( !tmp->is_open() ) 
                getErrStream() << "failed to open output file " << fn << std::endl;
            else 
		        outStream = tmp;
	    }
    }
    return *outStream;
}
std::ostream& Shell::setErrFile(const char* fn)
{
	if( errStream && errStream != & std::cerr ) {
		delete errStream;
		errStream = &std::cerr;
	}
    if( fn ) {
        errF.assign(fn);
	    if( errF.length() ) {
		    std::ofstream* tmp = new std::ofstream();
		    tmp->open( errF.c_str() );
            if( !tmp->is_open() ) 
                getErrStream() << "failed to open error file " << fn << std::endl;
            else 
		        errStream = tmp;
	    }
    }
    return *errStream;
}

std::ostream& Shell::printStreamState( std::ostream& fp ) const
{
    return (
	    fp << "OUT: " << ( outStream == (&std::cout) ? "<stdout>": outF ) << std::endl <<
	    "ERR: " << ( errStream == (&std::cerr) ? "<stderr>": errF ) << std::endl <<
	    "IN: " << ( inStream == (&std::cin) ? "<stdin>": inF ) << std::endl
    );

}
int Shell::setupStreams()
{
	/// input stream 
	if( inStream && inStream != & std::cin ) {
		delete inStream;
		inStream = &std::cin;
	} 
	
	if( inF.length() ) {
		std::ifstream* tmp = new std::ifstream();
		tmp->open( inF.c_str() );
		inStream = tmp;
	}

	/// output stream 
	if( outStream && outStream != & std::cout )  {
		delete outStream;
		outStream = &std::cout;
	} 
	
	if( outF.length() ) {
		std::ofstream* tmp = new std::ofstream();
		tmp->open( outF.c_str() );
		outStream = tmp;
	}

	/// error  stream 
	if( errStream && errStream != & std::cerr ) {
		delete errStream;
		errStream = &std::cerr;
	}
	if( errF.length() ) {
		std::ofstream* tmp = new std::ofstream();
		tmp->open( errF.c_str() );
		errStream = tmp;
	}
	return 0;
}

int Shell::indexCmdDataRange( const CmdDataRange& rng, bool reportDups )
{
	for( const CmdData* i = rng.first; i != rng.second; ++i ) {
		CmdDataMap::iterator mi = cmdMap.find( i->name );
		if( mi == cmdMap.end() ) {
			cmdMap.insert( CmdDataMap::value_type( i->name, i ) );
		} else if( reportDups ) {
			std::cerr << "command #" << i-rng.first << " name " << 
			i->name << " is duplicate" << std::endl;
		}
	}
	return 0;
}
int Shell::indexDefaultCommands()
{
    return indexCmdDataRange(CmdDataRange( ARR_BEGIN(g_cmd),ARR_END(g_cmd)));
}

int Shell::init()
{
	int rc = 0;
	if( !cmdMap.size() )
		rc = indexCmdDataRange(CmdDataRange( ARR_BEGIN(g_cmd),ARR_END(g_cmd)));

	if( context ) {
		delete context;
		context = 0;
	}

	context = mkContext();
	return rc;
}

namespace {
struct CmdCallable {
    const CmdData* cd;
    Shell* shell;
    char_cp cmd; 
    std::istream& in; 
    const std::string& argStr;
    int rc; 

    CmdCallable( const CmdData* cd, Shell* sh, char_cp cmd, std::istream& in, const std::string& argStr, int rc=0) : 
        cd(cd),
        shell(0),
        cmd(cmd),
        in(in),
        argStr(argStr),
        rc(rc)
    {
        shell = sh->cloneShell();
        shell->init();
    }
    ~CmdCallable() { };

    void operator()( ) { 
        cd->func( shell, cmd, in, argStr ); 
        delete shell;
        shell =0;
    }
};
} // anon namespace

int Shell::cmdInvoke( int& rc, char_cp cmd, std::istream& in, const std::string& argStr, bool background )
{
	const CmdData* cd = getCmdDta( cmd );
	if( cd ) {
        if( background ) {
            CmdCallable callable( cd, this, cmd, in, argStr );
            bgThreads().push_back( new boost::thread(callable) );
		    return (rc=0);
        } else {
		    return( (rc=cd->func( this, cmd, in, argStr ), 0) );
        }
	} else if( inStream == &std::cin && *cmd ) {
		*errStream << "command " << cmd << " not found. Run 'help [text]' for help. Falling back to `process`.\n";
	}
    return 0;
}

Shell::~Shell()
{
    for(auto &i : bgThreads()) { 
         i->join(); 
         delete i; 
    }

    delete context;
}

int Shell::runCmdLoop(std::istream* fp, const char* runScript )
{
	int rc = 0;

	bool isScript = (fp!= &std::cin );
	if( !isScript ) {
		fp = inStream;
		printPrompt();
	}	

	std::string curStr;
    if( runScript ) {
        std::cerr << "executing script " << runScript << std::endl;
        curStr = std::string("run ") + runScript;
    } else
        std::getline( *fp, curStr );
	do {
		std::stringstream sstr;
		if( echo ) {
			std::cerr << curStr << std::endl; 
		}
        std::string argStr;
        const char* spc = strchr( curStr.c_str(), ' ' );
        if( spc ) 
            argStr.assign( ++spc );

		sstr<< curStr;
		std::string cmdStr;
		std::getline( sstr, cmdStr, ' ');
        bool background = ( *(cmdStr.rbegin()) == '&' );
		cmdInvoke( rc, cmdStr.c_str(), sstr, argStr, background );
		if( rc ) 
			break;
		if( !isScript ) 
			printPrompt();
	} while( std::getline( *fp, curStr ) );
	return ( rc < 0 ? rc: 0 );
}
int Shell::run( const char* runScript )
{
	int rc = init();
	rc = setupStreams();
	rc = runCmdLoop(&std::cin, runScript);
	return ( rc < -1 ? rc : 0 );
}

}
