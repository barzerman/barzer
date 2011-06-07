#include <hunspell/hunspell.hxx>
#include <hunspell/hunspell.h>
#include <iostream>


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
				std::cerr << "SUGGESTION #" << i << ":" << result[i] << std::endl;
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
