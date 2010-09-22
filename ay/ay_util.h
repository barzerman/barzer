#ifndef AY_UTIL_H
#define AY_UTIL_H

#include <string.h>
#include <map>

namespace ay {

typedef char* char_p;
typedef const char* char_cp;

struct char_cp_compare_less {
	bool operator ()( char_cp l, char_cp r ) const
	{ 
		if( !l )
			return (r!=0);
		else if(!r )
			return false;
		else
			return (strcmp( l, r ) < 0 ); 
	}
};
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

/// map of arbitrary values with const char* as key
template <typename T>
struct char_cp_map {
	typedef std::map< char_cp, T,char_cp_compare_less> Type;
};


}

#endif
