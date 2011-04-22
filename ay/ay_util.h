#ifndef AY_UTIL_H
#define AY_UTIL_H
#include <ay_headers.h>

#include <map>
#include <vector>
#include <iostream>
#include <fstream>

#include <ay_debug.h>
#include <ay_util_char.h>

namespace ay {

/// general pointer utilities 
#define ARR_BEGIN( a ) (a)
#define ARR_END( a ) ((a)+sizeof(a)/sizeof((a)[0]))
#define ARR_SZ( a ) (sizeof(a)/sizeof((a)[0]))

#define ARR_NONNULL_STR(a,x) ( ((x)< (sizeof(a)/sizeof((a)[0]) ) && (x)>=0 ) ? a[x] : 0 )

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

struct range_comp {
	template <typename T1,typename T2>
	inline bool less_than( const T1& l1, const T2& l2, const T1& r1, const T2& r2 ) const
	{
		return ( l1< r1 ? true :(r1< l1 ? false: (l2< r2) )  );
	}
	template <typename T1,typename T2>
	inline bool greater_than( const T1& l1, const T2& l2, const T1& r1, const T2& r2 ) const
	{
		return less_than( r1,r2,l1,l2 );
	}

	template <typename T1,typename T2,typename T3>
	inline bool less_than( 
		const T1& l1, const T2& l2, const T3& l3, 
		const T1& r1, const T2& r2, const T3& r3 )
	{
		if( l1< r1 ) 
			return true;
		else if( r1 < l1 ) 
			return false;

		return less_than( l2,l3, r2,r3 );
	}
	template <typename T1,typename T2,typename T3>
	inline bool greater_than( 
		const T1& l1, const T2& l2, const T3& l3, 
		const T1& r1, const T2& r2, const T3& r3 )
	{
		return less_than( r1,r2,r3,l1,l2,l3 );
	}
	template <typename T1,typename T2,typename T3,typename T4>
	inline bool less_than( 
		const T1& l1, const T2& l2, const T3& l3, const T4& l4,
		const T1& r1, const T2& r2, const T3& r3, const T4& r4 )
	{
		if( l1< r1 ) return true; else if( r1 < l1 ) return false;
		return less_than( l2,l3,l4, r2,r3,r4 );
	}

	template <typename T1,typename T2,typename T3,typename T4>
	inline bool greater_than( 
		const T1& l1, const T2& l2, const T3& l3, const T4& l4,
		const T1& r1, const T2& r2, const T3& r3, const T4& r4 )
	{
		return less_than( r1,r2,r3,r4, l1,l2,l3,l4 );
	}

	template <typename T1,typename T2,typename T3,typename T4, typename T5>
	inline bool less_than( 
		const T1& l1, const T2& l2, const T3& l3, const T4& l4, const T5& l5,
		const T1& r1, const T2& r2, const T3& r3, const T4& r4, const T5& r5 )
	{
		if( l1< r1 ) return true; else if( r1 < l1 ) return false;
		return less_than( l2,l3,l4,l5, r2,r3,r4,r5 );
	}
	template <typename T1,typename T2,typename T3,typename T4,typename T5>
	inline bool greater_than( 
		const T1& l1, const T2& l2, const T3& l3, const T4& l4, const T5& l5,
		const T1& r1, const T2& r2, const T3& r3, const T4& r4, const T5& r5 )
	{
		return less_than( r1,r2,r3,r4,r5, l1,l2,l3,l4,l5 );
	}
};
/// RAII
template <typename T>
struct vector_raii {
	T& v;
	vector_raii(T& vv, const typename T::value_type& x ):
		v(vv)
	{ v.push_back(x); }

	~vector_raii() { v.pop_back(); }
};

}

#endif
