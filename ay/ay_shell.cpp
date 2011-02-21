#include <ay_shell.h>
#include <iostream>
#include <fstream>
#include <sstream>


namespace ay {

typedef Shell::CmdData CmdData;

static const CmdData g_cmd[] = {
	CmdData( Shell::cmd_help, "help", "get help on a function" ),
	CmdData( Shell::cmd_exit, "exit", "exit the shell" )
};

std::ostream& Shell::CmdData::print( std::ostream& fp ) const
{
	fp << name << " - " << desc;
}

int Shell::cmd_help( Shell* sh, char_cp cmd, std::istream& in )
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

int Shell::cmd_exit( Shell*, char_cp cmd, std::istream& in )
{
	exit(0);
	return 0;
}

int Shell::printPrompt()
{
	if( outStream == & std::cout )
		std::cout << "cmd>";
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
int Shell::runCmdLoop()
{
	int rc = 0;

	std::string curStr;
	printPrompt();
	while( std::getline( *inStream, curStr ) ) {
		std::stringstream sstr;
		sstr<< curStr;
		std::string cmdStr;
		std::getline( sstr, cmdStr, ' ');
		// sstr >> cmdStr;
		int cmdRc = 0;
		cmdInvoke( rc, cmdStr.c_str(), sstr );
		if( rc ) 
			break;
		printPrompt();
	}
	return rc;
}
int Shell::run()
{
	int rc = init();
	rc = setupStreams();
	rc = runCmdLoop();
	return rc;
}

}
