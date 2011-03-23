#ifndef AY_UTIL_H
#define AY_UTIL_H
#include <ay_headers.h>

#include <map>
#include <vector>
#include <iostream>
#include <fstream>

#include <ay_util_char.h>

namespace ay {

/// general pointer utilities 
#define ARR_BEGIN( a ) (a)
#define ARR_END( a ) ((a)+sizeof(a)/sizeof((a)[0]))
#define ARR_SZ( a ) (sizeof(a)/sizeof((a)[0]))

/// wide char utilities 

typedef wchar_t* wchar_p;
typedef const wchar_t* wchar_cp;

struct wchar_cp_compare_less {
	bool operator ()( wchar_cp l, wchar_cp r ) const
	{ 
		if( !l )
			return (r!=0);
		else if(!r )
			return false;
		else
			return (wcscmp( l, r ) < 0 ); 
	}
};
struct wchar_cp_compare_eq {
	bool operator ()( wchar_cp l, wchar_cp r ) const
	{ 
		if( !l )
			return !r;
		else if(!r )
			return false;
		else
			return (!wcscmp( l, r )); 
	}
};

/// map of arbitrary values with const wchar_t* as key
template <typename T>
struct wchar_cp_map {
	typedef std::map< wchar_cp, T,wchar_cp_compare_less> Type;
};

/// ascii char utilities 

/// map of arbitrary values with const char_t* as key
template <typename T>
struct char_cp_map {
	typedef std::map< char_cp, T,char_cp_compare_less> Type;
};

/// gets lines one after the other from input stream which 
/// is either constructed as a file or set to stdin
struct InputLineReader {
	std::ifstream fs;
	std::istream* fp;

	std::string str;

	bool isStdin() const
		{ return (fp == &std::cin); };

	InputLineReader( std::istream& ss ) ;
	InputLineReader( const char* s ) ;
		
	bool nextLine();
};

typedef std::vector<std::wstring> WstringVec;
/// streaming utilities
int wstring_vec_read( WstringVec& vec, std::wistream& in );

}

#endif
