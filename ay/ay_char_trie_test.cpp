#include <algorithm>
#include <vector>
#include <map>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include "ay_debug.h"
#include "ay_trie.h"

typedef ay::char_trie<uint32_t> CharTrie;
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

/// end of spelling affix trie 
int main( int argc, char* argv[] ) 
{
	//// 
	{
		Uint32CharTrie_print_cb cb;
		CharTrie::Trie ctrie;
		
		
		char buf[1024];
		int id= 0;
		
		std::ifstream inFile;
		if( argc > 1 ) {
			inFile.open( argv[1] );
			if( !inFile.is_open() ) 
				std::cerr << "cant open " << argv[1] << std::cerr;
			
		}
		while( inFile.getline( buf, sizeof(buf) ) ) {
			/*
			if( !strlen(buf) ) 
				break;
			*/
			CharTrie::add( ctrie, buf, id++, 0xffffffff );
		}

		CharTrie::add( ctrie, "he", id++, 0xffffffff );
		CharTrie::add( ctrie, "hello", id++, 0xffffffff );
		CharTrie::add( ctrie, "hell", id++, 0xffffffff );

		if( id < 100000 ) {
			ay::trie_visitor< CharTrie::Trie, Uint32CharTrie_print_cb > vis(cb);
			vis.visit(ctrie);
		}
		
		
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

}
