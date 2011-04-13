#ifndef BARZER_SHELL_H
#define BARZER_SHELL_H

#include <ay/ay_headers.h>
#include <ay/ay_shell.h>
#include <barzer_dtaindex.h>
#include <barzer_parse.h>
#include <barzer_universe.h>
#include "barzer_el_trie_walker.h"

#include <iostream>
namespace barzer {

//struct BarzerShellContext;

struct BarzerShellContext : public ay::ShellContext {
	DtaIndex* dtaIdx;

	Barz barz;
	QParser parser;

	StoredUniverse universe;
	BELTrieWalker trieWalker;

	DtaIndex* obtainDtaIdx()
	{ return &(universe.getDtaIdx()); }

	BarzerShellContext() : trieWalker(universe.getBarzelTrie()) {}
};
struct BarzerShell : public ay::Shell {
virtual int init();
virtual ay::ShellContext* mkContext();

BarzerShellContext* getBarzerContext() ;
	
};

}

#endif
