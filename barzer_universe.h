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
#include <barzer_spell.h>
#include <barzer_el_compwords.h>
#include <boost/unordered_map.hpp> 


namespace barzer {

class StoredUniverse;
/// holds all tries in the system keyed by 2 strings
/// TrieClass and  TrieId
class GlobalTriePool {
	typedef std::map< std::string, BELTrie > ClassTrieMap;
	typedef std::map< std::string, ClassTrieMap > TrieMap; 

	TrieMap d_trieMap;

	GlobalPools& d_gp;
	BarzelRewriterPool* d_rewrPool;
	BarzelWildcardPool* d_wcPool;
	BarzelFirmChildPool* d_fcPool;
	BarzelTranslationPool* d_tranPool;

public:
	ClassTrieMap& produceTrieMap( const std::string& trieClass );

	const ClassTrieMap* getTrieMap( const std::string& trieClass ) const
	{
		TrieMap::const_iterator i = d_trieMap.find( trieClass );
		if( i == d_trieMap.end() ) 
			return 0;
		else
			return &(i->second);
	}

	const BELTrie* getTrie( const std::string& trieClass, const std::string& trieId ) const
	{
		const ClassTrieMap* ctm = getTrieMap( trieClass );
		if( !ctm )
			return 0;
		else {
			ClassTrieMap::const_iterator j = ctm->find( trieId );
			if( j == ctm->end() ) 
				return 0;
			else
				return &(j->second);
		}
	}

	BELTrie& produceTrie( const std::string& trieClass, const std::string& trieId ) 
	{
		ClassTrieMap&  ctm = produceTrieMap( trieClass );
		ClassTrieMap::iterator i = ctm.find( trieId );

		if( i == ctm.end() ) 
			i = ctm.insert( ClassTrieMap::value_type(trieId, 
				BELTrie(
					d_gp,
					d_rewrPool,
					d_wcPool,
					d_fcPool,
					d_tranPool
				)) ).first;
		
		return i->second;
	}
	BELTrie* mkNewTrie() 
	{
		return new BELTrie(
			d_gp,
			d_rewrPool,
			d_wcPool,
			d_fcPool,
			d_tranPool
		);
	}
	
	BELTrie& init() 
		{ return produceTrie( std::string(), std::string() ); }

	GlobalTriePool( 
			GlobalPools& gp,
			BarzelRewriterPool* rewrPool,
			BarzelWildcardPool* wcPool,
			BarzelFirmChildPool* fcPool,
			BarzelTranslationPool* tranPool
	) :
			d_gp(gp),
			d_rewrPool( rewrPool ),
			d_wcPool( wcPool ),
			d_fcPool( fcPool ),
			d_tranPool( tranPool )
	{
		init();
	}
};

/// in general there is one cluster per customer installation 
class UniverseTrieCluster {
	GlobalTriePool& d_triePool;

public:
	typedef std::list< BELTrie* > BELTrieList;
private:
	BELTrieList d_trieList;
	friend class UniverseTrieClusterIterator;

	BELTrieList& getTrieList() { return d_trieList; }
public: 
	UniverseTrieCluster( GlobalTriePool& triePool ) : d_triePool( triePool ) 
		{
			d_trieList.push_back( &(d_triePool.init()) ) ;
		}

	BELTrie& appendTrie( const std::string& trieClass, const std::string& trieId )
	{
		BELTrie& tr = d_triePool.produceTrie(trieClass,trieId);
		d_trieList.push_back( &tr );
		return tr;
	}
	BELTrie& prependTrie( const std::string& trieClass, const std::string& trieId )
	{
		BELTrie& tr = d_triePool.produceTrie(trieClass,trieId);
		d_trieList.remove(&tr);
		d_trieList.push_front( &tr );
		return tr;
	}
};

class UniverseTrieClusterIterator {
	UniverseTrieCluster& d_cluster;
	UniverseTrieCluster::BELTrieList::iterator d_i;
public:
	UniverseTrieClusterIterator( UniverseTrieCluster& cluster ) : 
		d_cluster( cluster),
		d_i( cluster.getTrieList().begin() )
	{}
	
	void reset() { d_i = d_cluster.getTrieList().begin(); }
	const BELTrie& getCurrentTrie() const
		{ return *(*d_i); }
	BELTrie& getCurrentTrie()
		{ return *(*d_i); }
	bool advance( ) 
		{ return ( d_i == d_cluster.getTrieList().end() ? false : (++d_i, d_i != d_cluster.getTrieList().end()) ); }
};
/// a single data universe encompassing all stored 
/// data for 
/// Barzel, DataIndex and everything else 

class GlobalPools {
	GlobalPools(GlobalPools&); // {}
	GlobalPools& operator=(GlobalPools&); // {}
	// there can be only one
	static GlobalPools* g_instance;
public:
	static GlobalPools& getInstance() { return *g_instance; }
	
	// 0 should never be used 
	enum { DEFAULT_UNIVERSE_ID = 0 }; 

	ay::UniqueCharPool stringPool; /// all dictionary strings in the universe 

	/// strings for internal use only - file names, trie names, user info stuff etc 
	/// this pool should be very small compared to stringPool
	ay::UniqueCharPool internalStringPool; 

	/// every client's domain has this 
	DtaIndex dtaIdx; // entity-token links

	BarzelRewriterPool barzelRewritePool; // all rewrite structures for barzel
	BarzelWildcardPool barzelWildcardPool;  // all wildcard structures for barzel
	BarzelFirmChildPool barzelFirmChildPool; // all firm child lookups for barzel
	BarzelTranslationPool barzelTranslationPool;
	BarzelCompWordPool compWordPool; /// compounded words pool
	BELFunctionStorage funSt;
	DateLookup dateLookup;

	GlobalTriePool globalTriePool;

	typedef boost::unordered_map< uint32_t, StoredUniverse* > UniverseMap;

	UniverseMap d_uniMap;

	BarzerSettings settings;

	bool d_isAnalyticalMode;
	size_t d_maxAnalyticalModeMaxSeqLength;

	uint32_t internString_internal( const char* str ) { return internalStringPool.internIt( str ); }
	const char* internalString_resolve( uint32_t id ) const { return internalStringPool.resolveId( id ); }
	
	size_t getMaxAnalyticalModeMaxSeqLength() const { return d_maxAnalyticalModeMaxSeqLength; }

	EntPropCompatibility entCompatibility;

	bool isAnalyticalMode() const { return d_isAnalyticalMode; }
	void setAnalyticalMode() { d_isAnalyticalMode= true; }

	/// this will create the "wellknown" entities 
	StoredUniverse& produceUniverse( uint32_t id );

	BELTrie* mkNewTrie() { return globalTriePool.mkNewTrie(); }
	const StoredUniverse* getUniverse( uint32_t id )  const 
	{
		UniverseMap::const_iterator i = d_uniMap.find( id );
		return ( i == d_uniMap.end() ? 0 : i->second );
	}
	ParseSettings& parseSettings() { return settings.parseSettings(); }
	const ParseSettings& parseSettings() const { return settings.parseSettings(); }

	StoredUniverse* getUniverse( uint32_t id )
		{ return const_cast<StoredUniverse*>( ((const GlobalPools*)this)->getUniverse(id) ); }

	void createGenericEntities();
	      StoredUniverse* getDefaultUniverse()       { return getUniverse( DEFAULT_UNIVERSE_ID ) ; }
	const StoredUniverse* getDefaultUniverse() const { return getUniverse( DEFAULT_UNIVERSE_ID ) ; }

	BarzelCompWordPool& getCompWordPool() { return compWordPool; }
	const BarzelCompWordPool& getCompWordPool() const { return compWordPool; }

	DtaIndex& getDtaIdx() { return dtaIdx; }
	const DtaIndex& getDtaIdx() const { return dtaIdx; }

	const BarzerSettings& getSettings() const { return settings; }
	BarzerSettings& getSettings() { return settings; }

	const DtaIndex& getDtaIndex() const { return dtaIdx; }
	DtaIndex& getDtaIndex() { return dtaIdx; }

	ay::UniqueCharPool& getStringPool() { return stringPool; }
	const ay::UniqueCharPool& getStringPool() const { return stringPool; }

	std::ostream& printTanslationTraceInfo( std::ostream& , const BarzelTranslationTraceInfo& traceInfo ) const;
	GlobalPools();
	~GlobalPools();

	const char* decodeStringById( uint32_t strId ) const 
		{ return dtaIdx.resolveStringById( strId ); }
	/// never returns 0
	const char* decodeStringById_safe( uint32_t strId ) const
		{ const char* str = decodeStringById(strId); return (str ? str :"" ); }
};

class StoredUniverse {
	GlobalPools& gp;

	UniverseTrieCluster          trieCluster; 
	mutable UniverseTrieClusterIterator trieClusterIter;
	BarzerHunspell      hunspell;

	//BarzerSettings settings;
	friend class QSemanticParser;


	void  getToFirstTrie() const { trieClusterIter.reset(); }
	bool  getToNextTrie() const { return trieClusterIter.advance(); }

public:
	bool stemByDefault() const { return gp.parseSettings().stemByDefault(); }
	UniverseTrieCluster& getTrieCluster() { return trieCluster; }
	BarzerHunspell& getHunspell() { return hunspell; }
	const BarzerHunspell& getHunspell() const { return hunspell; }

	size_t getMaxAnalyticalModeMaxSeqLength() const { return gp.getMaxAnalyticalModeMaxSeqLength(); }
	bool isAnalyticalMode() const { return gp.isAnalyticalMode(); }
	EntPropCompatibility& getEntPropIndex() { return gp.entCompatibility; }
	const EntPropCompatibility& getEntPropIndex() const { return gp.entCompatibility; }
	BELTrie& produceTrie( const std::string& trieClass, const std::string& trieId ) 
	{
		return gp.globalTriePool.produceTrie( trieClass, trieId );
	}

	GlobalPools& getGlobalPools() { return gp; }
	const GlobalPools& getGlobalPools() const { return gp; }

	StoredUniverse(GlobalPools& gp );
	const DtaIndex& getDtaIdx() const { return gp.dtaIdx; }
		  DtaIndex& getDtaIdx() 	  { return gp.dtaIdx; }

	size_t getEclassEntCount( const StoredEntityClass& eclass ) const { return getDtaIdx().getEclassEntCount( eclass ); }
	const BELTrie& getBarzelTrie() const { return trieClusterIter.getCurrentTrie(); }
		  BELTrie& getBarzelTrie() 	  { return trieClusterIter.getCurrentTrie(); }

	      BarzelCompWordPool& getCompWordPool()       { return gp.compWordPool; }
	const BarzelCompWordPool& getCompWordPool() const { return gp.compWordPool; }

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
		return getWildcardPool().getWCLookup( id );
	}

	// had to add it in order to be able to create BELPrintContext
    // for custom trie printing
	ay::UniqueCharPool& getStringPool() {
		return gp.stringPool;
	}

	const ay::UniqueCharPool& getStringPool() const {
		return gp.stringPool;
	}


	// purges everything 
	void clear();

	const char* printableStringById( uint32_t id )  const
		{ return gp.stringPool.printableStr(id); }
	
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

	const BELFunctionStorage& getFunctionStorage() const { return gp.funSt; }
	const DateLookup& getDateLookup() const { return gp.dateLookup; }
	
	const char* getGenericSubclassName( uint16_t subcl ) const;

	const char* decodeStringById( uint32_t strId ) const 
		{ return getDtaIdx().resolveStringById( strId ); }
}; 

inline StoredUniverse& GlobalPools::produceUniverse( uint32_t id )
{
	StoredUniverse * p = getUniverse( id );
	if( !p ) { 
		p = new StoredUniverse( *this );
		d_uniMap[ id ] = p;	
	}
	return *p;
}

}
#endif // BARZER_UNIVERSE_H
