#ifndef BARZER_UNIVERSE_H
#define BARZER_UNIVERSE_H

#include <ay_string_pool.h>
#include <barzer_el_rewriter.h>
#include <barzer_el_wildcard.h>
#include <barzer_el_trie.h>
#include <barzer_dtaindex.h>
#include <barzer_el_function.h>
#include <barzer_date_util.h>
#include <barzer_settings.h>
#include <barzer_config.h>
#include <barzer_dict.h>

namespace barzer {

/// a single data universe encompassing all stored 
/// data for 
/// Barzel, DataIndex and everything else 

class StoredUniverse {
	ay::UniqueCharPool stringPool; /// all strings in the universe 

	DtaIndex dtaIdx;
	BarzelRewriterPool barzelRewritePool; // all rewrite structures for barzel
	BarzelWildcardPool barzelWildcardPool;  // all wildcard structures for barzel
	BarzelFirmChildPool barzelFirmChildPool; // all firm child lookups for barzel
	BarzelTranslationPool barzelTranslationPool;

	BELTrie  barzelTrie;

	BELFunctionStorage funSt;
	DateLookup dateLookup;
	
	BarzerSettings settings;

	BarzerDict dict;

	/// this will create the "wellknown" entities 
	void createGenericEntities();
public:
	StoredUniverse();
	const DtaIndex& getDtaIdx() const { return dtaIdx; }
		  DtaIndex& getDtaIdx() 	  { return dtaIdx; }

	const BELTrie& getBarzelTrie() const { return barzelTrie; }
		  BELTrie& getBarzelTrie() 	  { return barzelTrie; }

	const BarzelRewriterPool& getRewriterPool() const { return getBarzelTrie().getRewriterPool(); }
	BarzelRewriterPool& getRewriterPool() { return getBarzelTrie().getRewriterPool(); }
	const BarzelWildcardPool& getWildcardPool() const { return getBarzelTrie().getWildcardPool(); } 
	BarzelWildcardPool& getWildcardPool() { return getBarzelTrie().getWildcardPool(); } 
	const BarzelFirmChildPool& getFirmChildPool() const { return getBarzelTrie().getFirmChildPool(); }
	BarzelFirmChildPool& getFirmChildPool() { return getBarzelTrie().getFirmChildPool(); }
	const BarzelTranslationPool& getTranslationPool() const { return getBarzelTrie().getTranslationPool(); }
	BarzelTranslationPool& getTranslationPool() { return getBarzelTrie().getTranslationPool(); }

	const BarzelWCLookup* getWCLookup( uint32_t id ) const
	{
		return barzelWildcardPool.getWCLookup( id );
	}

	// had to add it in order to be able to create BELPrintContext
    // for custom trie printing
	ay::UniqueCharPool& getStringPool() {
		return stringPool;
	}

	const ay::UniqueCharPool& getStringPool() const {
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
			return getRewriterPool().resolveTranslation( bas, tran );
		else 
			return ( bas = BarzelRewriterPool::BufAndSize(), false );
	}
	bool isBarzelTranslationFallible( const BarzelTranslation& tran ) const {
		return tran.isFallible( getRewriterPool() );
	}

	const BELFunctionStorage& getFunctionStorage() const { return funSt; }
	const DateLookup& getDateLookup() const { return dateLookup; }
	const BarzerSettings& getSettings() const { return settings; }
	BarzerSettings& getSettings() { return settings; }
	const BarzerDict& getDict() const { return dict; }
	
	const char* getGenericSubclassName( uint16_t subcl ) const;
}; 

}
#endif // BARZER_UNIVERSE_H
