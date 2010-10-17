#ifndef AY_CMDPROC_H
#define AY_CMDPROC_H


/// command line processor 
#include <string>
#include <vector>
#include <ay_util.h>
namespace ay {

const int CMDLINE_ARG_NOTFOUND=-1;
struct CommandLineArgs {
	typedef const char* char_cp;
	int argc;
	const char_cp* argv;

	const char* getArgVal( bool& hasArg, const char* an, int*argPos=0 ) const;
	int			getArgVal_int( bool& hasArg, const char* an ) const;
	double		getArgVal_double( bool& hasArg, const char* an ) const;

	// returns from/to positions of argv[] following 'an' and ending 
	// either with the last arg or with the first argv[] starting with -
	std::pair<int,int> getArgVal_list( bool& hasArg, const char* an ) const;
	// returns 
	int getArgVal_list( bool& hasArg, std::vector<std::string>& out, const char* an ) const;

	CommandLineArgs() : argc(0), argv(0){}
	void init( int ac, char* av[] )
	{
		ac = argc; 
		argv = av;
	}
};

} // end of namespace

#endif
