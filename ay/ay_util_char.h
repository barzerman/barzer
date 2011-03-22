#ifndef AY_UTIL_CHAR_H
#define AY_UTIL_CHAR_H

#include <vector>
#include <map>
#include <set>

namespace ay {
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
			return r;
	}
};

/// templatized std::map keyed by char_cp 
/// map_by_char_cp< MyType > or map_by_char_cp< MyType, comparator >  
template <typename T, class comparer=char_cp_compare_less>
struct map_by_char_cp { typedef std::map< char_cp, T, comparer > Type; };

typedef map_by_char_cp<int>::Type char2int_map_t;

typedef std::vector<char_cp> 						char_cp_vec;
typedef std::set< char_cp,char_cp_compare_less > 	    char_cp_set;


} // namespace ay

#endif // AY_UTIL_CHAR_H
