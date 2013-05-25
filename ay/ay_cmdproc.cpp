#include <ay_cmdproc.h>
#include <stdlib.h>
#include <string.h>


namespace ay {

bool CommandLineArgs::hasArg( const char* an ) const
{
	const char_cp * end = argv + argc;
	const char_cp* i = argv;
	for( ; i!= end; ++i ) {
		if( !strcmp( *i, an ) )
			return true;
	}
	return false;
}
const char* CommandLineArgs::getArgVal( bool& hasArg, const char* an, int*argPos ) const
{
	const char_cp * end = argv + argc;
	const char_cp* i = argv;
    hasArg = false;
	for( ; i!= end; ++i ) {
		if( !strcmp( *i, an ) ) {
			++i;
            hasArg= true;
			break;
		}
	}
	if( argPos ) 
		*argPos = i-argv;

	if( i!= end && *i && **i != '-' ) {
		return *i;
	} else {
		return 0;
	}
}

int			CommandLineArgs::getArgVal_int( bool& hasArg, const char* an ) const
{
	const char* tmp = getArgVal(hasArg,an);
	if( hasArg ) 
		return( tmp ? atoi(tmp) : 0 );
	else 
		return 0;
}

double		CommandLineArgs::getArgVal_double( bool& hasArg, const char* an ) const
{
	const char* tmp = getArgVal(hasArg,an);
	if( hasArg ) 
		return( tmp ? atof(tmp) : 0 );
	else 
		return 0;
}

std::pair<int,int> CommandLineArgs::getArgVal_list( bool& hasArg, const char* an ) const
{
	int argPos = 0;
	getArgVal( hasArg, an, &argPos );

	if( hasArg ) {
		return std::pair<int,int>(CMDLINE_ARG_NOTFOUND,CMDLINE_ARG_NOTFOUND);
	} else 
		return std::pair<int,int>(CMDLINE_ARG_NOTFOUND,CMDLINE_ARG_NOTFOUND);
}

int CommandLineArgs::getArgVal_list( bool& hasArg, std::vector<std::string>& out, const char* an ) const
{
	return 0;
}
std::ostream&  CommandLineArgs::print( std::ostream& fp ) const 
{
    for( int i = 0; i< argc; ++i ) {
        fp<< argv[i] << std::endl;
    }
    return fp;
}

CmdLineArgsContainer::CmdLineArgsContainer( const char* argv0, std::istream& str )
{
    strVec.push_back( std::string(argv0) );
    appendArg(str);
}

void CmdLineArgsContainer::syncPtr()
{
    argV.clear();
    cmd.clear();

    if( strVec.empty() ) 
        return;

    argV.reserve( strVec.size() );
    for( const auto& i : strVec ) 
        argV.push_back( i.c_str() );

    cmd.init( argV.size(), (char**)(&(argV[0])) );
}
void CmdLineArgsContainer::appendArg( const std::string& str ) 
{
    strVec.push_back( str );
    syncPtr();
}
void CmdLineArgsContainer::appendArg( std::istream& str )
{
    std::string tmp;
    while( str >>tmp ) 
        strVec.push_back( tmp );
    syncPtr();
}



}

