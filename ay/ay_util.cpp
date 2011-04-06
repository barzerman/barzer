#include <sstream>
#include <ay_util.h>
namespace ay {
int wstring_vec_read( WstringVec& vec, std::wistream& in )
{
	size_t sz = 0;
	while( std::getline( in,(vec.resize(vec.size()+1 ),vec.back()), L' ') )  
		++sz;

	return ( vec.resize(sz), vec.size() );
}


	InputLineReader::InputLineReader( std::istream& ss ) 
	{
		std::string inFN;
		std::getline( ss, inFN, ' ' );
		if( inFN.length() && !isspace(inFN[0]) ) {
			fp = &fs;
			fs.open( inFN.c_str() );
		} else
			fp = &std::cin;
	}

	InputLineReader::InputLineReader( const char* s ) :
		fp( (s&&*s)? &fs : &std::cin )
	{
		if( !isStdin() ) 
			fs.open( s );
	}
	bool InputLineReader::nextLine()
	{
		if( !isStdin() && !fs.is_open() ) 
			return false;
		return( std::getline( *fp, str ) );
	}
} // end of ay namespace 
