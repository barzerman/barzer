#ifndef AY_UTIL_H
#define AY_UTIL_H

#include <string.h>
#include <map>

namespace ay {

/// general pointer utilities 
#define ARR_BEGIN( a ) (a)
#define ARR_END( a ) ((a)+sizeof(a)/sizeof((a)[0]))
#define ARR_SZ( a ) (sizeof(a)/sizeof((a)[0]))

typedef const char* char_cp;
typedef char* char_p;

struct char_cp_compare_eq {
	bool operator ()( char_cp l, char_cp r ) const
	{ 
		if( !l )
			return !r;
		else if(!r )
			return false;
		else
			return (!strcmp( l, r )); 
	}
};
struct char_cp_compare_less {
	bool operator ()( char_cp l, char_cp r ) const
	{ 
		if( l ) {
			if( r ) 
				return ( strcmp(l,r) < 0 );
			else 
				return false;
		} else
			return false;
	}
};
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


}

#endif
