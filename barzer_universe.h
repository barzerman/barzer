#ifndef BARZER_UNIVERSE_H
#define BARZER_UNIVERSE_H

#include <ay_string_pool.h>
#include <barzer_el_rewriter.h>
#include <barzer_el_wildcard.h>
#include <barzer_el_trie.h>
#include <barzer_dtaindex.h>
#include <barzer_el_function.h>

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

	BELFunctionStorage funSt;

public:
	StoredUniverse();
	const DtaIndex& getDtaIdx() const { return dtaIdx; }
		  DtaIndex& getDtaIdx() 	  { return dtaIdx; }

	const BELTrie& getBarzelTrie() const { return barzelTrie; }
		  BELTrie& getBarzelTrie() 	  { return barzelTrie; }
	const BarzelWCLookup* getWCLookup( uint32_t id ) const
	{
		return barzelWildcardPool.getWCLookup( id );
	}
	const BarzelWildcardPool& getWildcardPool() const {return barzelWildcardPool;}

	// had to add it in order to be able to create BELPrintContext
    // for custom trie printing
	ay::UniqueCharPool& getStringPool() {
		return stringPool;
	}

	// purges everything 
	void clear();

	const char* printableStringById( uint32_t id )  const
		{ return stringPool.printableStr(id); }
	
	std::ostream& printBarzelTrie( std::ostream& fp, const BELPrintFormat& fmt ) const;
	std::ostream& printBarzelTrie( std::ostream& fp ) const;

	bool getBarzelRewriter( BarzelRewriterPool::BufAndSize& bas, const BarzelTranslation& tran ) const
	{
		if( tran.isRewriter() ) 
			return barzelRewritePool.resolveTranslation( bas, tran );
		else 
			return ( bas = BarzelRewriterPool::BufAndSize(), false );
	}
	bool isBarzelTranslationFallible( const BarzelTranslation& tran ) const {
		return tran.isFallible( barzelRewritePool );
	}

	const BELFunctionStorage& getFunctionStorage() const { return funSt; }

}; 

}
#endif // BARZER_UNIVERSE_H
