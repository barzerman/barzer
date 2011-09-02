/*
 * barzer_el_trie_shell.cpp
 *
 *  Created on: Apr 13, 2011
 *      Author: polter
 */


#include <map>
#include <boost/assign.hpp>

#include <barzer_el_trie_shell.h>
#include <ay_logger.h>
#include <ay_util_time.h>


namespace barzer {

#define DEF_TFUN(n) static int trie_shell_##n( BarzerShell* shell, std::istream& in )

DEF_TFUN(load) {
	//AYLOG(DEBUG) << "trie_shell_load  called";

	BarzerShellContext *context = shell->getBarzerContext();
	StoredUniverse &uni = context->getUniverse();

	BELTrie &trie = context->getTrie();

	BELReader reader(&trie, uni.getGlobalPools() );
	reader.initParser(BELReader::INPUT_FMT_XML);

	ay::stopwatch totalTimer;

	std::string sin;
	if (in >> sin) {
		const char *fname = sin.c_str();
		//AYLOG(DEBUG) << "Loading " << fname;
		int numsts = reader.loadFromFile(fname);
		std::cout << numsts << " statements read. " << std::endl;
		if (numsts)	context->trieWalker.loadChildren();
		std::cerr << "Loaded in " << totalTimer.calcTime() << " seconds\n";
	} else {
		//AYLOG(ERROR) << "no filename";
	}
	return 0;
}

DEF_TFUN(print) {
	//AYLOG(DEBUG) << "trie_shell_print  called";
	BarzerShellContext *context = shell->getBarzerContext();
	StoredUniverse &uni = context->getUniverse();
	BELTrie &trie = context->getTrie();

	BELTrieWalker &walker = context->trieWalker;

	BELPrintFormat fmt;
	ay::UniqueCharPool &stringPool = uni.getStringPool();
	BELPrintContext ctxt( trie, stringPool, fmt );
	//trie.print(std::cout, ctxt);
	walker.getCurrentNode().print(std::cout, ctxt);

	return 0;
}

DEF_TFUN(test) {
	BELTrieWalker w  = shell->getBarzerContext()->trieWalker;
    std::cout << w.getNodeStack().size() << std::endl;
    return 0;
}


DEF_TFUN(help) {
	//AYLOG(DEBUG) << "trie_shell_help  called";
	std::cout << "trie [load|print|help|list|show|moveto|moveback]" << std::endl;
	return 0;
}


#undef DEF_TFUN

// but i really want some static directory service for storing this kind of shit


typedef std::map<std::string,TrieShellFun> FunMap;

int bshtrie_process( BarzerShell* shell, std::istream& in ) {
#define TSHF(n) (#n, trie_shell_##n)
	static FunMap funmap =
		boost::assign::map_list_of TSHF(load)
								   TSHF(print)
								   TSHF(test)
								   TSHF(help);


#undef TSHF
	//AYLOG(DEBUG) << "bshtrie_process called";
	std::string fname;
	in >> fname;
	//AYLOG(DEBUG) << "fname=" << fname;
	//TrieShellFun fun = triefunmap[fname];
	FunMap::iterator i = funmap.find(fname);
	if (i == funmap.end()) return 0;
	return i->second(shell, in);
	//return 0;
}


}
