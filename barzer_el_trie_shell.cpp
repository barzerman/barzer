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

#define DEF_TFUN(n) static int trie_shell_##n( BarzerShell* shell, std::istream& in )

DEF_TFUN(load) {
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
		AYLOG(DEBUG) << "Loading " << fname;
		int numsts = reader.loadFromFile(fname);
		std::cout << numsts << " statements read. " << std::endl;
		context->trieWalker.loadFC();
		context->trieWalker.loadWC();
	} else {
		AYLOG(ERROR) << "no filename";
	}
	return 0;
}

DEF_TFUN(print) {
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

/*
DEF_TFUN(show) {
	BarzerShellContext *context = shell->getBarzerContext();
	StoredUniverse &uni = context->universe;
	BELTrieWalker &walker = context->trieWalker;

	std::vector<BarzelTrieFirmChildKey> fcvec = walker.getFCvec();
	BarzelTrieNode &node = walker.getCurrentNode();

	ay::UniqueCharPool &stringPool = uni.getStringPool();

	std::cout << "Current node got: " << std::endl;
	std::cout << fcvec.size() << " firm children" << std::endl;

	for (size_t i = 0; i < fcvec.size(); i++) {
		BarzelTrieFirmChildKey &key = fcvec[i];
		std::cout << "[" << i << "] ";
		key.print(std::cout) << ":";

		switch(key.type) {
		case BTND_Pattern_Token_TYPE: {
				const char *str = stringPool.resolveId(key.id);
				std::cout << (str ? str : "Illegal string ID");
			}
			break;
		case BTND_Pattern_Punct_TYPE:
			std::cout << (char) key.id;
			break;
		case BTND_Pattern_CompoundedWord_TYPE: // no idea what to do here
			break;
		}
		std::cout << std::endl;
	}


	std::cout << "the wildcard lookup ID is: " << node.getWCLookupId() << std::endl;

	AYLOG(DEBUG) << "Stack size is " << walker.getNodeStack().size();

	return 0;
}

DEF_TFUN(moveback) {
	BELTrieWalker &w = shell->getBarzerContext()->trieWalker;
	w.moveBack();
	return 0;
}

DEF_TFUN(moveto) {
	BELTrieWalker &w = shell->getBarzerContext()->trieWalker;
	std::string s;
	if (in >> s) {
		const char *cstr = s.c_str();
		size_t num = atoi(cstr);
		std::cout << "Moving to child " << num << std::endl;
		if (w.moveTo(num)) {
			AYLOG(ERROR) << "Couldn't load child";
		}
		std::cout << "Stack size is " << w.getNodeStack().size() << std::endl;
	} else {
		std::cout << "trie moveto <#Child>" << std::endl;
	}
	return 0;

} //*/
DEF_TFUN(test) {
	BELTrieWalker w  = shell->getBarzerContext()->trieWalker;
    std::cout << w.getNodeStack().size() << std::endl;
    return 0;
}


DEF_TFUN(help) {
	AYLOG(DEBUG) << "trie_shell_help  called";
	std::cout << "trie [load|print|help|list|show|moveto|moveback]" << std::endl;
	return 0;
}


#undef DEF_TFUN

// but i really want some static directory service for storing this kind of shit

#define TSHF(n) (#n, trie_shell_##n)
static std::map<std::string,TrieShellFun> triefunmap =
		boost::assign::map_list_of TSHF(load)
								   TSHF(print)
								   TSHF(test)
								   TSHF(help);
/*TSHF(show)
TSHF(moveback)
TSHF(moveto)
*/

#undef TSHF


int bshtrie_process( BarzerShell* shell, std::istream& in ) {
	AYLOG(DEBUG) << "bshtrie_process called";
	std::string fname;
	in >> fname;
	AYLOG(DEBUG) << "fname=" << fname;
	TrieShellFun fun = triefunmap[fname];
	if (fun) return fun(shell, in);
	return 0;
}


}
