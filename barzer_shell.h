#ifndef BARZER_SHELL_H
#define BARZER_SHELL_H

#include <ay/ay_headers.h>
#include <ay/ay_shell.h>
#include <barzer_dtaindex.h>
#include <barzer_parse.h>
#include <barzer_barz.h>
#include <barzer_universe.h>
#include <barzer_el_trie_walker.h>
#include <barzer_el_wildcard.h>

#include <iostream>
namespace barzer {

//struct BarzerShellContext;

struct BarzerShellContext : public ay::ShellContext {
	// DtaIndex* dtaIdx;

	StoredUniverse& universe;
	BELTrieWalker trieWalker;

	Barz barz;
	QParser parser;

	DtaIndex* obtainDtaIdx()
	{ return &(universe.getDtaIdx()); }

	BarzerShellContext(StoredUniverse& u) : 
		universe(u),
		trieWalker(u.getBarzelTrie()) ,
		parser( u )
	{}
};
struct BarzerShell : public ay::Shell {
	StoredUniverse* d_universe;

	void setUniverse( StoredUniverse* u ) { d_universe = u; }
	BarzerShell( StoredUniverse* u=0 ) : d_universe(u) {}
	virtual int init();
	virtual ay::ShellContext* mkContext();
	virtual ay::ShellContext* cloneContext();
	virtual BarzerShell* cloneShell() 
	{
		BarzerShell* newShell =  new BarzerShell( *this );
		ay::ShellContext* newCtxt = this->cloneContext();
		newShell->setContext( newCtxt );
		return newShell;
	}

	BarzerShellContext* getBarzerContext() ;
	
};

}

#endif
