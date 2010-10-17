#include <ay_shell.h>
#include <iostream>
#include <fstream>
#include <sstream>


namespace ay {

typedef Shell::CmdData CmdData;

static const CmdData g_cmd[] = {
	CmdData( Shell::cmd_help, L"help", L"get help on a function" ),
	CmdData( Shell::cmd_exit, L"exit", L"exit the shell" )
};

int Shell::setupStreams()
{
	/// input stream 
	if( inStream && inStream != & std::wcin ) {
		delete inStream;
		inStream = 0;
	} 
	
	if( inF.length() ) {
		std::wifstream* tmp = new std::wifstream();
		tmp->open( inF.c_str() );
		inStream = tmp;
	} else
		inStream = &std::wcin;

	/// output stream 
	if( outStream && outStream != & std::wcout )  {
		delete outStream;
		outStream = 0;
	} 
	
	if( outF.length() ) {
		std::wofstream* tmp = new std::wofstream();
		tmp->open( outF.c_str() );
		outStream = tmp;
	} else
		outStream = &std::wcout;

	/// error  stream 
	if( errStream && errStream != & std::wcerr ) {
		delete errStream;
		errStream = 0 ;
	}
	if( errF.length() ) {
		std::wofstream* tmp = new std::wofstream();
		tmp->open( errF.c_str() );
		errStream = tmp;
	} else
		errStream = &std::wcerr;
}

int Shell::indexCmdDataRange( const CmdDataRange& rng )
{
	for( const CmdData* i = rng.first; i != rng.second; ++i ) {
		CmdDataMap::iterator mi = cmdMap.find( i->name );
		if( mi == cmdMap.end() ) {
			cmdMap.insert( CmdDataMap::value_type( i->name, i ) );
		} else {
			std::wcerr << "command #" << i-rng.first << " name " << 
			i->name << " is duplicate" << std::endl;
		}
	}
	return 0;
}

int Shell::run()
{
	int rc = indexCmdDataRange(CmdDataRange( ARR_BEGIN(g_cmd),ARR_END(g_cmd)));

	setupStreams();
	std::wstring curStr;
	while( std::getline( *inStream, curStr ) ) {
		std::wstringstream sstr;
		sstr.str() = curStr;
		std::wstring cmdStr;
		sstr >> cmdStr;
		int cmdRc = 0;
		cmdInvoke( rc, cmdStr.c_str(), sstr );
		if( rc ) 
			break;
	}
	return rc;
}

}
