#ifndef AY_CHAR_H
#define AY_CHAR_H

#include <cstring>
#include <cstdlib>
#include <vector>
/// char string utilities 
namespace ay {

template <typename T>
struct char_compare { bool operator()( const T& l, const T& r ) const { return (l==r); } };

struct char_compare_nocase_ascii { 
	bool operator()( char l, char r ) const { return( toupper(l) == toupper(r) ); } 
};

/// standard levenshtein edit distance algorithm wrapped in a reusable object
/// it takes care of memory allocations and will only allocate a new chunk if its buffer is smaller 
/// than required 
/// best way to use it:
/// char comparison method is overridable. the algorithm also works for characters of wider types than char
/// there are specialized ascii versions - case sensitive and in-sensitive
/// LevenshteinEditDistance editDist;
///     perform many edit distance calculations using it
/// 
class LevenshteinEditDistance {
	size_t d_curBufSz;
	int    *d_buf;

	int* setBuf( size_t m, size_t n )
	{
		size_t newSz = m*n*sizeof(int);
		if( newSz> d_curBufSz ) {
			if( d_buf ) 
				free(d_buf);
			d_buf = (int*)malloc( newSz );
			d_curBufSz = newSz;
		}
		return d_buf;
	}
	/// this is needed to compute 
	inline int min3(int a,int b,int c)
	{
  		if( a< b ) 
			return( c< a ? c : a );
  		else 
			return( c< b ? c : b );
	}
	enum { DEFAULT_BUF_SZ = 256 };
public:
	LevenshteinEditDistance( size_t sz = DEFAULT_BUF_SZ ) : 
		d_curBufSz(sz),
		d_buf( (int*) malloc( sz ))
	{ }
	~LevenshteinEditDistance() { if( d_buf ) free(d_buf); }

	int ascii_with_case(const char *s,const char*t);
	int ascii_no_case(const char *s,const char*t);
	template <typename T>
	int ascii(const char *s,const char*t, const T& compare);

	template <typename char_type, typename T>
	int generic(const char_type *s, size_t s_sz, const char_type*t, size_t t_sz, const T& compare);
};
// levenshtein for generic character type
template <typename char_type, typename T>
inline int LevenshteinEditDistance::generic(const char_type *s, size_t s_sz, const char_type*t, size_t t_sz, const T& compare)
{
  //Step 1
  int n=s_sz;
  int m=t_sz;
  if(n!=0&&m!=0)
  {
    ++m;
    ++n;
    int * d= setBuf(m,n);
    //Step 2	
    for(int k=0;k<n;++k) d[k]=k;
    for(int k=0;k<m;++k) d[k*n]=k;
    //Step 3 and 4	
    for(int i=1;i<n;++i) {
	  const char_type schar = s[i-1]; 
      for(int j=1;j<m;++j) {
        //Step 5
		const char_type tchar = t[j-1];
		int cost = ( compare(schar,tchar) ? 0: 1 );
        //Step 6			 
        d[j*n+i]=min3(d[(j-1)*n+i]+1,d[j*n+i-1]+1,d[(j-1)*n+i-1]+cost);
      }
	}
    int distance=d[n*m-1];
    return distance;
  }
  else 
    return ( !n ? m: n ); 
}
// levenshtein template specialization for ascii char
template <typename T>
inline int LevenshteinEditDistance::ascii(const char *s,const char*t, const T& compare )
{
  //Step 1
  int n=strlen(s); 
  int m=strlen(t);
  typedef char char_type;
  if(n!=0&&m!=0)
  {
    ++m;
    ++n;
    int * d= setBuf(m,n);
    //Step 2	
    for(int k=0;k<n;++k) d[k]=k;
    for(int k=0;k<m;++k) d[k*n]=k;
    //Step 3 and 4	
    for(int i=1;i<n;++i) {
	  const char_type schar = s[i-1]; 
      for(int j=1;j<m;++j) {
        //Step 5
		const char_type tchar = t[j-1];
		int cost = ( compare(schar,tchar) ? 0: 1 );
        //Step 6			 
        d[j*n+i]=min3(d[(j-1)*n+i]+1,d[j*n+i-1]+1,d[(j-1)*n+i-1]+cost);
      }
	}
    int distance=d[n*m-1];
    return distance;
  }
  else 
    return ( !n ? m: n ); 
}
inline int LevenshteinEditDistance::ascii_with_case(const char *s,const char*t)
{
	return ascii( s, t, char_compare<char>() );
}
inline int LevenshteinEditDistance::ascii_no_case(const char *s,const char*t)
{
	return ascii( s, t, char_compare_nocase_ascii() );
}
// 2 byte char 
struct Char2B {
    char b[2];

    Char2B( ) { b[0] = b[1] = 0; }
    Char2B( const char* s ) { b[0] = *s; b[1] = s[1]; }
    
    inline static void mkCharvec( std::vector<char>& out, const std::vector< Char2B >& cv )
    {
        out.clear();
        out.reserve( 2*cv.size() + 1 );
        for( std::vector< Char2B >::const_iterator i = cv.begin(); i!= cv.end(); ++i ) {
            out.push_back( i->b[0] );
            out.push_back( i->b[1] );
        }
        out.push_back( 0 );
    }
    inline static void mkCharvec( 
        std::vector<char>& out, 
        std::vector< Char2B >::const_iterator& beg ,
        std::vector< Char2B >::const_iterator& end
        )
    {
        out.clear();
        out.reserve( 2*(end-beg) + 1 );
        for( std::vector< Char2B >::const_iterator i = beg; i!= end; ++i ) {
            out.push_back( i->b[0] );
            out.push_back( i->b[1] );
        }
        out.push_back( 0 );
    }
};

inline bool operator==( const Char2B& l, const Char2B& r ) { return( l.b[0] == r.b[0] && l.b[1] == r.b[1]); }
inline bool operator!=( const Char2B& l, const Char2B& r ) { return !(l == r ); }

inline bool operator<( const Char2B& l, const Char2B& r ) 
    { return( l.b[0] < r.b[0] || ( l.b[0] == r.b[0] && (l.b[1] < r.b[1]) ) ); }

struct Char2B_iterator {
    const char* d_s;
    Char2B_iterator( ): d_s(0) {}
    Char2B_iterator( const char* s ): d_s(s) {}
    Char2B_iterator& operator++() { d_s += 2; return *this; }
    Char2B_iterator& operator--() { d_s -= 2; return *this; }
    Char2B_iterator& operator+( int i) { d_s += (2*i); return *this; }
    Char2B_iterator& operator-( int i) { d_s -= (2*i); return *this; }

    Char2B operator *() { return Char2B(d_s); }

    const char* operator[]( size_t i ) const { return ( d_s + (i*2) ); }
};
inline int operator -( const Char2B_iterator& l, const Char2B_iterator& r ) 
    { return ( (l.d_s - r.d_s)/2 ); }

inline int operator != (const Char2B_iterator& l, const Char2B_iterator& r ) { return ( l.d_s != r.d_s); }
inline int operator == (const Char2B_iterator& l, const Char2B_iterator& r ) { return ( l.d_s == r.d_s); }

/// prefix matcher  - pfx should be an array of const char* terminated by 0
/// 
inline bool c_str_match_prefix( const char* r, const char* s )
{
       const char* x = r;
       for( ; *x && *s; ++x, ++s ) 
           if( *x != *s ) return false;
       
       return (*x || !*s);
}
inline bool c_str_match_any_prefix( const char* s, const char* pfx[] ) 
{
    for( ; *pfx; ++pfx ) { if( c_str_match_prefix(s,*pfx) ) return true; }

    return false;
}
/// Char2B_accessor str("фанат");
/// str("ф") is true
/// str(1,"a") is true

struct Char2B_accessor {
    const char* d_s;
    
    Char2B_accessor( const char* s) : d_s(s) {}

    bool isChar( const char* c2b ) const { return ( *d_s == c2b[0] && d_s[1] == c2b[1] ); }

    bool isChar( int i, const char* c2b ) const 
    {
        const char* io = ( d_s +  (2*i) );
        return ( *io == c2b[0] && io[1] == c2b[1] ); 
    }
    
    bool prefixMatch( const char* s ) const 
        { 
            const char* x = d_s;
            for( ; *x && *s; ++x, ++s ) 
                { if( *x != *s ) return false; } 
            return (*x || !*s); 
        }
    /// this does prefix match 
    bool operator()( const char* s ) const { return c_str_match_prefix(d_s,s); }
    /// gets array of strings terminated by 0 string
    /// returns true as soon as the first prefix match returns true
    /// const char * a[] = { "a", "b"}
    bool operator()( const char* s[] ) const { return c_str_match_any_prefix(d_s,s); }

    bool operator==( const char* s ) const { return !strcmp(d_s,s); }

    const char* c_str() const { return d_s; }

    Char2B_accessor offset( int i ) const { return Char2B_accessor( d_s + (2*i) ); }
    Char2B_accessor prev( ) const { return Char2B_accessor(d_s-2); }
    Char2B_accessor next( ) const { return Char2B_accessor(d_s+2); }
    
    char operator *( ) const { return *d_s; }
}; // Char2B_accessor


//// end of 2 byte char 


} // ay namespace ends 
#endif // AY_CHAR_H
