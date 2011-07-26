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
	GlobalPools& gp;
	StoredUniverse& universe;
	BELTrieWalker trieWalker;
	BELTrie* d_trie;

	Barz barz;
	QParser parser;

	DtaIndex* obtainDtaIdx() { return &(universe.getDtaIdx()); }
	BELTrie& getTrie() { return *d_trie; }
	const BELTrie& getTrie() const { return *d_trie; }
	BarzerShellContext(StoredUniverse& u, BELTrie& trie) : 
		gp(u.getGlobalPools()),
		universe(u),
		trieWalker(trie),
		d_trie(&trie),
		parser( u )
	{}
	BELTrie* getCurrentTriePtr() { return d_trie; }
	void setTrie( BELTrie* t ) 
	{
		if( t ) 
			d_trie = t;
		else {
			std::cerr << "trying to set null trie in barzer shell context\n";
		}
	}
	GlobalPools& getGLobalPools() { return gp; }
	const GlobalPools& getGLobalPools() const { return gp; }

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
