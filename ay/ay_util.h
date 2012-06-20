#ifndef AY_UTIL_H
#define AY_UTIL_H
#include <ay_headers.h>

#include <map>
#include <vector>
#include <iostream>
#include <fstream>

#include <ay_debug.h>
#include <ay_util_char.h>
#include <stdint.h>
#include <cstdlib>

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
/// reads from file and splits on a separator  
class FileReader {
    size_t d_buf_sz;
    char * d_buf;
    FILE*  d_file;  
    /// default separator is '|'
    char d_separator, 
        d_comment; // default comment char is '#'

    std::vector< const char* > d_tok;
public:
    const std::vector< const char* >& tok() const { return d_tok; }

    enum { DEFAULT_MAX_LINE_WIDTH = 256 };
    void setBufSz( size_t newSz ) ;
    FILE* openFile( const char* fname=0 ) ;
    void closeFile( ) 
        { if( d_file && d_file!= stdin ) { fclose(d_file); } d_file = 0; }

    FileReader( char sep='|', size_t bufSz = DEFAULT_MAX_LINE_WIDTH );
    ~FileReader();
    void setSeparator(char s ) { d_separator = s; }
    void setComment(char s ) { d_comment = s; }
    
    double getTok_double( size_t t ) const { return ( (t< d_tok.size())? atof(d_tok[t]): 0  ); }
    int getTok_int( size_t t ) const { return ( (t< d_tok.size())? atoi(d_tok[t]): 0  ); }
    const char* getTok_char( size_t t ) const { return( (t< d_tok.size())? d_tok[t]: ""  ); }
    FILE* getFile() {return d_file; }
    /// file reading stops immediately after callback returns non 0
    template <typename CB>
    size_t readFile( CB& callback, const char* fname )
    {
        d_file= openFile(fname);
        if( !d_file )
            return 0;

        size_t numRec = 0;
        while( fgets( d_buf, d_buf_sz, d_file ) ) {
            ++numRec;
            d_buf[ d_buf_sz-1 ] = 0;
            size_t len = strlen(d_buf);
            if( len > 0 )  {
                if( *d_buf == d_comment ) 
                    continue;
                d_buf[ len-1] =0;
            } else 
                continue;
            
            d_tok.clear();
            for( char* s = d_buf; s; s= (s=strchr(s,d_separator),(s?(*s=0,s+1):0)) ) 
                d_tok.push_back(s);
            if( callback( *this ) ) 
                break;
        }
        closeFile();
        return numRec;
    }

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

	template <typename T1,typename T2,typename T3,typename T4, typename T5, typename T6>
	inline bool less_than( 
		const T1& l1, const T2& l2, const T3& l3, const T4& l4, const T5& l5, const T6& l6,
		const T1& r1, const T2& r2, const T3& r3, const T4& r4, const T5& r5, const T6& r6 )
	{
		if( l1< r1 ) return true; else if( r1 < l1 ) return false;
		return less_than( l2,l3,l4,l5,l6, r2,r3,r4,r5, r6 );
	}
	template <typename T1,typename T2,typename T3,typename T4,typename T5, typename T6>
	inline bool greater_than( 
		const T1& l1, const T2& l2, const T3& l3, const T4& l4, const T5& l5, const T6& l6,
		const T1& r1, const T2& r2, const T3& r3, const T4& r4, const T5& r5, const T6& r6 )
	{
		return less_than( r1,r2,r3,r4,r5,r6, l1,l2,l3,l4,l5,l6 );
	}
};
/// RAII
template <typename T>
struct vector_raii {
	T& v;
	vector_raii(T& vv, const typename T::value_type& x ) : v(vv) { v.push_back(x); }
	~vector_raii() { v.pop_back(); }
};

// added for conditional pushing/popping
template<class T> struct vector_raii_p {
	T &vec;
	uint32_t cnt;
	vector_raii_p(T& v) : vec(v), cnt(0) {}
	void push(const typename T::value_type &n) {
		vec.push_back(n);
		++cnt;
	}
	~vector_raii_p() {
		while (cnt--) vec.pop_back();
	}
private:
	vector_raii_p(vector_raii_p&) {}
	vector_raii_p& operator=(const vector_raii_p &) {}
};

// bitwise copy semantics
namespace bcs {

template <typename T>
inline bool equal( const T& l, const T& r ) 
	{ return !(memcmp( &l, &r, sizeof(T) )); }
template <typename T>
inline bool less( const T& l, const T& r ) 
	{ return (memcmp( &l, &r, sizeof(T) )<0); }

}
/// copies s into dest, replacing diacritics with an english character
int umlautsToAscii( std::string& dest, const char* s );
int stripDiacrictics( std::string& dest, const char* s ) { return umlautsToAscii(dest,s); }

/// returns true if s points at the beginning of a diacritic char 
bool is_diacritic( const char* s );

}

#endif
