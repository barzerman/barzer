#ifndef AY_CHAR_H
#define AY_CHAR_H

#include <cstring>
#include <cstdlib>
/// char string utilities 
namespace ay {

template <typename T>
struct char_compare { bool operator()( const T& l, const T& r ) const { return (l==r); } };

struct char_compare_nocase_ascii { 
	bool operator()( char l, char r ) const { return( toupper(l) == toupper(r) ); } 
};

/// tandard levenshtein edit distance algorithm wrapped in a reusable object
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
} // ay namespace ends 
#endif // AY_CHAR_H
