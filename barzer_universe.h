#ifndef BARZER_UNIVERSE_H
#define BARZER_UNIVERSE_H

#include <ay_string_pool.h>
#include <barzer_el_rewriter.h>
#include <barzer_el_wildcard.h>
#include <barzer_el_trie.h>
#include <barzer_dtaindex.h>

namespace barzer {

/// a single data universe encompassing all stored 
/// data for 
/// Barzel, DataIndex and everything else 

class StoredUniverse {
	ay::UniqueCharPool stringPool; /// all strings in the universe 

	DtaIndex dtaIdx;
	BarzelRewriterPool barzelRewritePool; // all rewrite structures for barzel
	BarzelWildcardPool barzelWildcardPool;  // all wildcard structures for barzel

	BELTrie  barzelTrie;
public:
	StoredUniverse();
	const DtaIndex& getDtaIdx() const { return dtaIdx; }
		  DtaIndex& getDtaIdx() 	  { return dtaIdx; }

	const BELTrie& getBarzelTrie() const { return barzelTrie; }
		  BELTrie& getBarzelTrie() 	  { return barzelTrie; }
	
	// purges everything 
	void clear();

	const char* printableStringById( uint32_t id )  const
		{ return stringPool.printableStr(id); }
	
	std::ostream& printBarzelTrie( std::ostream& fp, const BELPrintFormat& fmt ) const;
	std::ostream& printBarzelTrie( std::ostream& fp ) const;
}; 

}
#endif // BARZER_UNIVERSE_H
