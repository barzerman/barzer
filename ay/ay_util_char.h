#ifndef AY_UTIL_CHAR_H
#define AY_UTIL_CHAR_H

#include <arch/barzer_arch.h>
#include <ay_headers.h>
#include <vector>
#include <map>
#include <set>
#include <ctype.h>
#include <ay_ru_chars.h>

namespace ay {
typedef const char* char_cp;
typedef char* char_p;

#define AY_MAYBE_RUSSIAN(s) ((static_cast<uint8_t>((s)[0])==0xd0) || (static_cast<uint8_t>((s)[0])==0xd1))
#define AY_TOUPPER(c) ( ('a'<=(c) && 'z'>=(c)) ? ((c)-32) : (c) )
#define AY_TOLOWER(c) ( ('A'<=(c) && 'Z'>=(c)) ? ((c)+32) : (c) )

inline int ay_strcmp( const char* l, const char* r )
{
    for( ; *l && *r; ++l, ++r ) {
        if( *l < *r )
            return -1;
        else if( *r< *l ) 
            return 1;
    }

    return( *l ? 1 : (*r ? -1: 0)  );
}
inline bool ay_strcmp_eq( const char* l, const char* r )
{
    for( ; *l && *r; ++l, ++r ) {
        if( *l != *r )
            return false;
    }

    return !( *l || *r );
}

inline int ay_strcasecmp( const char* l, const char* r )
{
    for( ; *l && *r; ++l, ++r ) {
        if( AY_MAYBE_RUSSIAN(l) && AY_MAYBE_RUSSIAN(r) ) {
            russian::RuChar ll= russian::single_char_tolower(l);
            russian::RuChar rl= russian::single_char_tolower(r);
            if( ll < rl ) 
                return -1;
            else if( rl< ll ) 
                return 1;
            else if( !ll.second )  // ll == rr and this is the end
                    return 0;
            else { // ll ==rr and this is not the end - skipping to next char
                ++l;
                ++r;
            }
        } else {
            char ul = AY_TOUPPER(*l), ur=AY_TOUPPER(*r);
            if( ul < ur )
                return -1;
            else if( ur< ul ) 
                return 1;
        }
    }

    return( *l ? 1 : (*r ? -1: 0)  );
}

struct char_cp_compare_eq_nocase {
	bool operator ()( char_cp l, char_cp r ) const
	{ 
		if( !l )
			return !r;
		else if(!r )
			return false;
		else
			return (!ay_strcasecmp( l, r )); 
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
			return (ay_strcmp_eq( l, r )); 
	}
};
struct char_cp_compare_nocase_less {
	bool operator ()( char_cp l, char_cp r ) const
	{ 
		if( l ) {
			if( r ) 
				return ( ay_strcasecmp(l,r) < 0 );
			else 
				return false;
		} else
			return r;
	}
};
struct char_cp_compare_less {
	bool operator ()( char_cp l, char_cp r ) const
	{ 
		if( l ) {
			if( r ) 
				return ( ay_strcmp(l,r) < 0 );
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

//// char* string parse utils 

namespace strparse {
inline char* find_separator( char* buf, const char* sep ) {
	for( ; *buf; ++buf ) {
		for( const char* s = sep; *s; ++s ) {
			if( *buf == *s ) 
				return buf;
		}
	}
	return 0;
}
/// returns:
///    0 - for all ascii string 
///    1 - all non-ascii
///   -1 - mixed
inline int is_string_ascii( const char* s ) 
{
	bool hasAscii = false, hasNonAscii = false;
	for( ; *s; ++s ) {
		if( isascii(*s) ) {
			if( !hasAscii ) hasAscii = true;
		} else  {
			if( !hasNonAscii ) hasNonAscii = true;
		}
	}
	return ( hasNonAscii ?  (hasAscii ? -1: 1) : (hasAscii ? 0: -1) );
}

}

struct char_cp_hash {
    inline size_t operator()( const char* s ) const
    {
        if( !s ) return 0;
        size_t h=0;
        for (; *s; ++s)
            h = 37 * h + *s;
        return h; // or, h % ARRAY_SIZE;
    }
};
struct char_cp_hash_nocase {
    inline size_t operator()( const char* s ) const
    {
        if( !s ) return 0;
        size_t h=0;
        for (; *s; ++s) {
            if( AY_MAYBE_RUSSIAN(s) ) {
                russian::RuChar sl= russian::single_char_tolower(s);
                if( sl.second ) 
                    ++s;
                h = 37 * h + sl.second;
            } else {
                h = 37 * h + AY_TOLOWER((*s));
            }
        }
        return h; // or, h % ARRAY_SIZE;
    }
};

} // namespace ay

#endif // AY_UTIL_CHAR_H
