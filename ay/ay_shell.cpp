#include <ay_shell.h>


namespace ay {

static const CmdData *g_cmd[] = {
	Shell::CmdData( Shell::cmd_help, L"help", L"get help on a function" ),
	Shell::CmdData( Shell::cmd_exit, L"exit", L"exit the shell" )
};

int Shell::setupStream()
{
	/// input stream 
	if( inStream && inStream != & std::wcin ) 
		inStream = (delete inStream, 0 );
	if( inF.length() ) {
		inStream = new std::ofstream();
		ofstr->open( inF.c_str() );
	} else
		inStream = &std::wcin;

	/// output stream 
	if( outStream && outStream != & std::wcout ) 
		outStream = (delete outStream, 0 );
	if( outF.length() ) {
		outStream = new std::ofstream();
		ofstr->open( outF.c_str() );
	} else
		outStream = &std::wcout;

	/// error  stream 
	if( errStream && errStream != & std::wcerr ) 
		errStream = (delete errStream, 0 );
	if( errF.length() ) {
		errStream = new std::ofstream();
		ofstr->open( errF.c_str() );
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
		std::wstringstream sstr( stringstream::in );
		sstr.str() = curStr;
		std::string cmdStr;
		sstr >> cmdStr;
		int cmdRc = 0;
		rc = cmdInvoke( cmdStr.c_str(), sstr );
		if( rc ) 
			break;
	}
	return rc;
}

}
