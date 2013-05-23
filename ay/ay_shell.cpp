#include <ay_shell.h>
#include <iostream>
#include <fstream>
#include <sstream>


namespace ay {

typedef Shell::CmdData CmdData;

static const CmdData g_cmd[] = {
	CmdData( Shell::cmd_help, "help", "get help on a function" ),
	CmdData( Shell::cmd_exit, "exit", "exit the shell" ),
	CmdData( Shell::cmd_quit, "quit", "exit current barzer shell" ),
	CmdData( Shell::cmd_run, "run", "run scripts" ),
	CmdData( Shell::cmd_set, "set", "sets parameters" ),
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
	std::string fName;
	if( !(in >> fName) ) {
		std::cerr << "must specify valid file name\n";
		return 0;
	}
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

int Shell::indexCmdDataRange( const CmdDataRange& rng )
{
	for( const CmdData* i = rng.first; i != rng.second; ++i ) {
		CmdDataMap::iterator mi = cmdMap.find( i->name );
		if( mi == cmdMap.end() ) {
			cmdMap.insert( CmdDataMap::value_type( i->name, i ) );
		} else {
			std::cerr << "command #" << i-rng.first << " name " << 
			i->name << " is duplicate" << std::endl;
		}
	}
	return 0;
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
int Shell::cmdInvoke( int& rc, char_cp cmd, std::istream& in, const std::string& argStr )
{
	const CmdData* cd = getCmdDta( cmd );
	if( cd ) {
		return( (rc=cd->func( this, cmd, in, argStr ), 0) );
	} else {
		*errStream << "command " << cmd << " not found. Run 'help [text]' for help. Falling back to `process`.\n";
		return cmdInvoke(rc, "help", in, argStr );
	}
}

int Shell::runCmdLoop(std::istream* fp )
{
	int rc = 0;

	std::string curStr;
	bool isScript = fp; 
	if( !isScript ) {
		fp = inStream;
		printPrompt();
	}	
	while( std::getline( *fp, curStr ) ) {
		std::stringstream sstr;
		if( isScript ) {
			std::cerr << curStr << std::endl; 
		}
        std::string argStr;
        const char* spc = strchr( curStr.c_str(), ' ' );
        if( spc ) 
            argStr.assign( ++spc );

		sstr<< curStr;
		std::string cmdStr;
		std::getline( sstr, cmdStr, ' ');
		// sstr >> cmdStr;
		// int cmdRc = 0;
		cmdInvoke( rc, cmdStr.c_str(), sstr, argStr );
		if( rc ) 
			break;
		if( !isScript ) 
			printPrompt();
	}
	return ( rc < 0 ? rc: 0 );
}
int Shell::run()
{
	int rc = init();
	rc = setupStreams();
	rc = runCmdLoop();
	return ( rc < -1 ? rc : 0 );
}

}
