
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#pragma once 
#include <ay_headers.h>


/// command line processor 
#include <string>
#include <vector>
#include <ay_util.h>
namespace ay {

const int CMDLINE_ARG_NOTFOUND=-1;
struct CommandLineArgs {
	typedef const char* char_cp;
	int argc;
	const const char_cp* argv;

	const char* getArgVal( bool& hasArg, const char* an, int*argPos=0 ) const;
	bool 		hasArg( const char* an ) const;
	int			getArgVal_int( bool& hasArg, const char* an ) const;
	double		getArgVal_double( bool& hasArg, const char* an ) const;

	// returns from/to positions of argv[] following 'an' and ending 
	// either with the last arg or with the first argv[] starting with -
	std::pair<int,int> getArgVal_list( bool& hasArg, const char* an ) const;
	// returns 
	int getArgVal_list( bool& hasArg, std::vector<std::string>& out, const char* an ) const;

	CommandLineArgs() : argc(0), argv(0){}
	CommandLineArgs( int ac, char* av[]) : argc(ac), argv(av){}

	void init( int ac, char* av[] )
	{
		argc = ac; 
		argv = av;
	}
    void clear() { argc = 0; argv= 0; }
    
    std::ostream& print( std::ostream& ) const;
};

// cout or named file
template <typename STREAM_TYPE, typename FSTREAM_TYPE>
class file_stream {
    STREAM_TYPE *d_fallbackFp;
    STREAM_TYPE *d_fp; 
    file_stream( const file_stream<STREAM_TYPE,FSTREAM_TYPE>& ) {}
public:
    std::string name;
    STREAM_TYPE& fp() { return *d_fp; }
    file_stream( STREAM_TYPE* fb ) :
        d_fallbackFp(fb),
        d_fp(fb)
    { }

    bool open( const char* fn ) {
        if( d_fp != d_fallbackFp ) {
            delete d_fp;
            d_fp = d_fallbackFp;
        }
        if( fn && *fn ) {
            name.assign(fn);
            FSTREAM_TYPE* x = new FSTREAM_TYPE();
            x->open(fn);
            if( x->is_open() ) {
                d_fp = x;
            } else {
                delete x;
                d_fp= d_fallbackFp;
                return false;
            }
        }
        return true;
    }

    ~file_stream() { if( d_fallbackFp != d_fp ) delete d_fp; }        
};
typedef file_stream<std::ostream,std::ofstream> file_ostream;
typedef file_stream<std::istream,std::ifstream> file_istream;

/// safe type to store a list of strings and treat it as a bunch of command line arguments 
/// used in shell
class CmdLineArgsContainer {
    std::vector< std::string > strVec;
    std::vector< const char* > argV;

    /// syncs strVec, argV and cmd
    void syncPtr();
public:
    CommandLineArgs cmd;

    /// argv0 is the "commandline" , str is the stream containing the rest of the arguments
    CmdLineArgsContainer( const char* argv0, std::istream& str );
    CmdLineArgsContainer( ) {}

    void appendArg( const std::vector< std::string >& vec ) 
        { strVec = vec; syncPtr(); }

    void appendArg( const std::string& str );
    void appendArg( std::istream& str );

    /// FS must be a file_stream
    template <typename FS>
    bool getFileFromArg( FS& fp, const char* opt )
    {
        bool status = false;
        if( const char* path = cmd.getArgVal( status, opt ) ) 
            return fp.open( path );
        else 
            return status;
    }
};

} // end of namespace
