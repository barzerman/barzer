#pragma once
#include <cstring>
#include <cstdlib>
#include <stdlib.h>
#include <vector>
#include <ay_utf8.h>
#include <ay_char.h>
/// char string utilities 
namespace ay {

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
			d_buf = (int*)realloc( d_buf, newSz );
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

    /// _sz is size in 2 byte glyphs. make sure you pass 2 chains with even number of characters
	int twoByte(const char *s,size_t s_sz, const char*t, size_t t_sz);

	template <typename char_type, typename T>
	int generic(const char_type *s, size_t s_sz, const char_type*t, size_t t_sz, const T& compare);
    
	int utf8(const StrUTF8& s,const StrUTF8& t);
};
// levenshtein for generic character type

inline int LevenshteinEditDistance::utf8(const StrUTF8& s,const StrUTF8& t )
{
  int n=s.length();
  int m=t.length();
  if(n!=0&&m!=0)
  {
    ++m;
    ++n;
    int * d= setBuf(m,n);
    //Step 2	
    for(int k=0;k<n;++k) d[k]=k;
    for(int k=0;k<m;++k) d[k*n]=k;
    //Step 3 and 4	
    CharUTF8 tchar, schar;
    for(int i=1;i<n;++i) {
	  schar = s[i-1]; 
      for(int j=1;j<m;++j) {
        //Step 5
		tchar = t[j-1];
		int cost = ( (schar!=tchar) ? 0: 1 );
        //Step 6			 
        d[j*n+i]=min3(d[(j-1)*n+i]+1,d[j*n+i-1]+1,d[(j-1)*n+i-1]+cost);
      }
	}
    int distance=d[n*m-1];
    return distance;
  } else
    return ( !n ? m: n ); 
}

inline int LevenshteinEditDistance::twoByte(const char *s, size_t s_sz, const char*t, size_t t_sz )
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
	  const char * schar = (s+2*(i-1));
      for(int j=1;j<m;++j) {
        //Step 5
		const char * tchar = t+2*(j-1);
		int cost = ( (schar[0]==tchar[0]&&schar[1]==tchar[1]) ? 0: 1 );
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
  if( n == m ) { // checking if the distance is simple 1 char or swap
    if( n == 1 ) 
        return 0;
    
    enum { MAX_NUM_DIFF = 2 };
    size_t diff[ MAX_NUM_DIFF ];
    size_t numDiff = 0;
    for( int i = 0; i< n; ++i ) {
        if( s[i] != t[i] ) {
            if( numDiff < MAX_NUM_DIFF ) diff[ numDiff ] = i;
            ++numDiff;
        }
    }
    switch( numDiff ) {
    case 0: return 0; 
    case 1: return 1; 
    case 2: 
        { 
            if( s[diff[0]] == t[diff[1]] 
                && 
                s[diff[1]] == t[diff[0]]
                && 
                ( diff[1] +1 == diff[0] || diff[0] +1 == diff[1])
            )
                return 1;
            else
                return 2;
        } 
        break;
    }
  }
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
} // ay namespace ends 
