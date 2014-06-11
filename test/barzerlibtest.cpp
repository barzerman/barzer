#include <barzer_lib.h>
#include <iostream>

int main( int argc, char* argv[]) 
{
    barzer::barzerlib::Instance barzerInstance;
    
    std::vector< std::string > bararg;
    for( int i =1; i< argc; ++i ) 
        bararg.push_back( argv[i] );
    barzerInstance.init( bararg );
    
    std::string buf;
    while( std::getline(std::cin, buf) ) {
        barzerInstance.processUri( std::cout, buf.c_str() );
    }

    return 0;
}

