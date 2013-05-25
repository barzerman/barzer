#include <algorithm>
#include <vector>
#include <map>
#include <string>
#include <sstream>
#include "ay_debug.h"
#include "ay_trie.h"

typedef ay::trie<int, int > Trie;
typedef ay::char_trie_funcs<uint32_t> CharTrie;
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
struct Uint32CharTrie_fullpath_print_cb {
	ay::trie_visitor_continue_t operator()( const CharTrie::Trie::Path& p)

	{
		for( CharTrie::Trie::Path::const_iterator i = p.begin(); i != p.end();++i ) {
			std::cerr << (*i)->first << ":" << (*i)->second.data() << "/" ;
		}
		std::cerr << std::endl;
		return ay::TRIE_VISITOR_CONTINUE;
	}
};

struct Uint32CharTrie_print_cb {
	ay::trie_visitor_continue_t operator()( const CharTrie::Trie::Path& p)

	{
		if( !p.size() || p.back()->second.data() == 0xffffffff ) 
			return ay::TRIE_VISITOR_CONTINUE;

		for( CharTrie::Trie::Path::const_iterator i = p.begin(); i != p.end();++i ) {
			std::cerr << (*i)->first ;
		}
		std::cerr << ":" << p.back()->second.data() << std::endl;
		return ay::TRIE_VISITOR_CONTINUE;
	}
};

int main( int argc, char* argv[] ) 
{
	//// 
	{
		Uint32CharTrie_print_cb cb;
		CharTrie::Trie ctrie;
		CharTrie::add( ctrie, "he", 333, 0xffffffff );
		CharTrie::add( ctrie, "hello", 150, 0xffffffff );
		CharTrie::add( ctrie, "hell", 250, 0xffffffff );
		ay::trie_visitor< CharTrie::Trie, Uint32CharTrie_print_cb > vis(cb);
		
		vis.visit(ctrie);
		char buf[1024];
		
		while( true ) {
			std::vector< int > intVec;
			int i4=0;
			std::cout << "Enter word:";
			
			std::cin.getline( buf, sizeof(buf) );
				
			typedef std::pair< const CharTrie::Trie*, const char*  > PathPair;
			
			PathPair pp = CharTrie::matchString( ctrie, buf );
			if( pp.first ) {
				std::cerr << "substring matched:" << pp.first->data() << "~"  << std::string( buf, pp.second- buf ) << std::endl;
				std::cerr << "\n";
			} else 
				std::cerr << "substring did not match!\n";
		}
	}

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
