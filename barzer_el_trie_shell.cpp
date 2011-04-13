/*
 * barzer_el_trie_shell.cpp
 *
 *  Created on: Apr 13, 2011
 *      Author: polter
 */


#include <map>
#include <boost/assign.hpp>

#include "barzer_el_trie_shell.h"
#include <ay/ay_logger.h>

namespace barzer {

#define TSHF(n) (#n, trie_shell_##n)

static int trie_shell_load( BarzerShell* shell, char_cp cmd, std::istream& in ) {
	AYLOG(DEBUG) << "trie_shell_load  called";

	BarzerShellContext *context = shell->getBarzerContext();
	StoredUniverse &uni = context->universe;
	BELTrie &trie = uni.getBarzelTrie();
	ay::UniqueCharPool &stringPool = uni.getStringPool();

	BELReader reader(&trie, &stringPool);
	reader.initParser(BELReader::INPUT_FMT_XML);

	std::string sin;
	if (in >> sin) {
		const char *fname = sin.c_str();
		int numsts = reader.loadFromFile(fname);
		std::cout << numsts << " statements read. ";
	}
	return 0;
}

static int trie_shell_print( BarzerShell* shell, char_cp cmd, std::istream& in ) {
	AYLOG(DEBUG) << "trie_shell_print  called";
	BarzerShellContext *context = shell->getBarzerContext();
	StoredUniverse &uni = context->universe;
	BELTrie &trie = uni.getBarzelTrie();

	//BELTrieWalker &walker = context->trieWalker;
	//TrieNodeStack &nstack = walker.getNodeStack();

	BELPrintFormat fmt;
	ay::UniqueCharPool &stringPool = uni.getStringPool();
	BELPrintContext ctxt( trie, stringPool, fmt );
	trie.print(std::cout, ctxt);
	return 0;
}


static int trie_shell_help( BarzerShell* shell, char_cp cmd, std::istream& in ) {
	AYLOG(DEBUG) << "trie_shell_help  called";
	std::cout << "trie [load|print|help|list|moveto|moveback]" << std::endl;
	return 0;
}


// ye ye, I know i'm bad bad bad for creating this static map
// but i really want some static directory service for storing this kind of shit

static std::map<std::string,TrieShellFun> triefunmap =
		boost::assign::map_list_of TSHF(load)
								   TSHF(print)
								   TSHF(help);


int bshtrie_process( BarzerShell* shell, char_cp cmd, std::istream& in ) {
	AYLOG(DEBUG) << "bshtrie_process called";
	std::string fname;
	in >> fname;
	AYLOG(DEBUG) << "fname=" << fname;
	TrieShellFun fun = triefunmap[fname];
	if (fun) return fun(shell, cmd, in);
	return 0;
}


}
