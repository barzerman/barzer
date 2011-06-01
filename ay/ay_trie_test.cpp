#include <algorithm>
#include <vector>
#include <map>
#include <string>
#include <sstream>
#include "ay_debug.h"
#include "ay_trie.h"

typedef ay::trie<int, int > Trie;
struct CB_crap {
	ay::trie_visitor_continue_t operator()( const Trie::Path& p)

	{
		for( Trie::Path::const_iterator i = p.begin(); i != p.end();++i ) {
			std::cerr << (*i)->first << ":" << (*i)->second.data() << "/" ;
		}
		std::cerr << std::endl;
		return ay::TRIE_VISITOR_CONTINUE;
	}
};

int main( int argc, char* argv[] ) 
{
	Trie t;
	
	t.add(10,2).add(11,3);
	t.add(10,2).add(25,3).add(100,18);
	t.add(10,2).add(25,3);
	t.add(10,2).add(25,3);
	t.add(10,2).add(25,3);
	t.add(10,2).add(25,3);
	t.add(10,2).add(25,3);
	t.add(10,2).add(25,3);
	t.add(10,2).add(25,3);
	CB_crap cb;
	ay::trie_visitor< Trie, CB_crap > vis(cb);
	
	vis.visit(t);
	
	char buf[1024];
	while( true ) {
		std::vector< int > intVec;
		int i4=0;
		std::cout << "Enter path:";
		
		std::cin.getline( buf, sizeof(buf) );
		std::istringstream sstr(buf);
		while( sstr >> i4 ) {
			std::cerr << "adding " << i4 << "\n";
			intVec.push_back( i4 );
		}
		
		typedef std::pair< bool, std::vector< int >::const_iterator > PathPair;
		
		PathPair pp = t.getLongestPath( intVec.begin(), intVec.end() );
		if( pp.first ) {
			std::cerr << "path matched:" ;
			for( std::vector< int >::const_iterator i = intVec.begin(); i!= pp.second; ++i )
			{
				std::cerr << *i << "/";
			}
			std::cerr << "\n";
		} else 
			std::cerr << "Path did not match!\n";
	}
}
