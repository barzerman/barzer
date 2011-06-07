#include <hunspell/hunspell.hxx>
#include <hunspell/hunspell.h>
#include <iostream>
#include <math.h>

namespace {

template <typename T>
struct char_compare { bool operator()( const T& l, const T& r ) const { return (l==r); } };

struct char_compare_nocase_ascii { 
	bool operator()( char l, char r ) const { return( toupper(l) == toupper(r) ); } 
};

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

};
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


}


int main( int argc, char* argv[] ) 
{
	printf( "Hello World\n" );

	char buf[ 1024] ; 
	Hunspell hunspell( "/Users/yanis/Downloads/en_US/en_US.aff", "/Users/yanis/Downloads/en_US/en_US.dic" );

	const char* extrawords = "extrawords.txt";
	FILE* fp = fopen( extrawords, "r" );
	if( !fp ) {
		fp= stdin;
		std::cerr <<" reading extra words from stdin\n";
	} else {
		std::cerr <<" reading extra words from " << extrawords << std::endl;
	}
	time_t t = time(0);
	int wordCount = 0;
	LevenshteinEditDistance editDist;
	while( fgets( buf, sizeof(buf), fp ) ) {
		buf[ strlen(buf)-1 ] = 0;
		if( !*buf ) 
			break;
		int rc = hunspell.add(  buf ) ;
		++wordCount;
		//std::cerr << "rc=" << rc << std::endl;
	}
	if( fp != stdin ) 
		fclose(fp);
	std::cerr << "DONE ADDING " << wordCount << " EXTRA WORDS in " << time(0) -t << " seconds\n";
	while( fgets( buf, sizeof(buf), stdin ) ) {
		buf[ strlen(buf)-1 ] = 0;
		char ** result = 0;
		int n = hunspell.spell( buf );
		std::cerr << n << " returnned from hunspell.spell( buf ) " << std::endl;
		if( !n ) {
			n = hunspell.suggest( &result, buf );
			for( int i = 0; i< n; ++i ) {
				std::cerr << "SUGGESTION #" << i << ":" << result[i] << 
				"~" << editDist.ascii_no_case( buf, result[i] ) << std::endl;
			}
			hunspell.free_list( &result, n );
			result = 0;
		} else {
			std::cerr << buf << " is ok\n";
		}
		/// analysis 
		{
			// n = hunspell.analyze( &result, buf );
			n = hunspell.stem( &result, buf );
			if( n<=0 ) {
				std::cerr << "analysis failed\n";
			}
			for( int i = 0; i< n; ++i ) {
				std::cerr << "ANALYZE #" << i << ":" << result[i] << std::endl;
			}
			hunspell.free_list( &result, n );
		}
	}
}
