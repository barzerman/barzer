#include <ay_cmdproc.h>
#include <stdlib.h>
#include <string.h>


namespace ay {

const char* CommandLineArgs::getArgVal( bool& hasArg, const char* an, int*argPos ) const
{
	const char_cp * end = argv + argc;
	const char_cp* i = argv;
	for( ; i!= end; ++i ) {
		if( !strcmp( *i, an ) ) {
			++i;
			break;
		}
	}
	if( argPos ) 
		*argPos = i-argv;

	if( i!= end && *i && **i != '-' ) {
		return (hasArg= true,*i);
	} else {
		hasArg= false;
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


}

