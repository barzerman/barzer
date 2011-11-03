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
#include <barzer_bzspell.h>
#include <barzer_el_compwords.h>
#include <boost/unordered_map.hpp> 
#include <boost/unordered_set.hpp> 


namespace barzer {

class StoredUniverse;
/// holds all tries in the system keyed by 2 strings
/// TrieClass and  TrieId
class GlobalTriePool {
	typedef std::map< std::string, BELTrie* > ClassTrieMap;
	typedef std::map< std::string, ClassTrieMap > TrieMap; 
	
	TrieMap d_trieMap;

	// every time new trie is added to d_trieMap its also added to 
	// d_triePool
	
	std::vector< BELTrie* > d_triePool;

	GlobalPools& d_gp;
	
public:
	const BELTrie* getTrie_byGlobalId( uint32_t n ) const { return( n< d_triePool.size() ? (d_triePool[n]) : 0 ); } 

	ClassTrieMap& produceTrieMap( const std::string& trieClass );

	ClassTrieMap* getTrieMap( const std::string& trieClass ) ;
	const ClassTrieMap* getTrieMap( const std::string& trieClass ) const;
	const BELTrie* getTrie( const std::string& trieClass, const std::string& trieId ) const;

	BELTrie* getTrie( const std::string& trieClass, const std::string& trieId ) ;
	BELTrie* produceTrie( const std::string& trieClass, const std::string& trieId ) ;
	BELTrie* mkNewTrie() ;
	BELTrie& init() 
		{ return *produceTrie( std::string(), std::string() ); }

    BELTrie* getDefaultTrie() { return getTrie(std::string(), std::string()); }

	GlobalTriePool( GlobalPools& gp) : d_gp(gp) { init(); }
	~GlobalTriePool();
};

/// in general there are 2 clusters per customer installation 
/// the regular trie cluster and the topic trie one 
class UniverseTrieCluster {
	GlobalTriePool& d_triePool;
	StoredUniverse& d_universe;

public:
	typedef std::list< BELTrie* > BELTrieList;
private:
	BELTrieList d_trieList;
	friend class UniverseTrieClusterIterator;

public: 
	BELTrieList& getTrieList() { return d_trieList; }
	const BELTrieList& getTrieList() const { return d_trieList; }
	UniverseTrieCluster( GlobalTriePool& triePool, StoredUniverse& u ) ;
	BELTrie& appendTrie( const std::string& trieClass, const std::string& trieId );
	BELTrie& prependTrie( const std::string& trieClass, const std::string& trieId );
	BELTrie*  getFirstTrie() { return ( d_trieList.empty() ?  0 : (*(d_trieList.begin())) ); }
	const BELTrie*  getFirstTrie() const { return ( d_trieList.empty() ?  0 : (*(d_trieList.begin())) ); }

    void clearList() { d_trieList.clear(); }

    void appendTriePtr( BELTrie* trie ) { d_trieList.push_back(trie); }
};


class UniverseTrieClusterIterator {
	const UniverseTrieCluster& d_cluster;
	UniverseTrieCluster::BELTrieList::const_iterator d_i;
public:
	UniverseTrieClusterIterator( const UniverseTrieCluster& cluster ) : 
		d_cluster( cluster),
		d_i( cluster.getTrieList().begin() )
	{}
	
	void reset() { d_i = d_cluster.getTrieList().begin(); }
	const BELTrie& getCurrentTrie() const
		{ return *(*d_i); }
	BELTrie& getCurrentTrie()
		{ return *(*d_i); }
	bool isAtEnd() const 
		{ return (d_i == d_cluster.getTrieList().end()); }
	bool advance( ) 
		{ return ( d_i == d_cluster.getTrieList().end() ? false : (++d_i, d_i != d_cluster.getTrieList().end()) ); }
};
/// a single data universe encompassing all stored 
/// data for 
/// Barzel, DataIndex and everything else 

class GlobalPools {
	GlobalPools(GlobalPools&); // {}
	GlobalPools& operator=(GlobalPools&); // {}
	typedef boost::unordered_set< uint32_t > DictionaryMap;
	DictionaryMap d_dictionary;
public:
	void addWordToDictionary( uint32_t w ) { d_dictionary.insert(w); }
	bool isWordInDictionary( uint32_t w ) const { return (d_dictionary.find(w) != d_dictionary.end()); }
	size_t readDictionaryFile( const char* name ); 

	// 0 should never be used 
	enum { DEFAULT_UNIVERSE_ID = 0 }; 

	ay::UniqueCharPool stringPool; /// all dictionary strings in the universe 

	/// strings for internal use only - file names, trie names, user info stuff etc 
	/// this pool should be very small compared to stringPool
	ay::UniqueCharPool internalStringPool; 

	/// every client's domain has this 
	DtaIndex dtaIdx; // entity-token links

	BarzelCompWordPool compWordPool; /// compounded words pool
	BELFunctionStorage funSt;
	DateLookup dateLookup;

	GlobalTriePool globalTriePool;

	/// maps userId to universe pointer
	typedef boost::unordered_map< uint32_t, StoredUniverse* > UniverseMap;

	UniverseMap d_uniMap;
    const UniverseMap& getUniverseMap() const { return d_uniMap; }

	BarzerSettings settings;

	bool d_isAnalyticalMode;
	size_t d_maxAnalyticalModeMaxSeqLength;


	uint32_t string_getId( const char* str ) const { return stringPool.getId( str ); }
	uint32_t string_intern( const char* str ) { return stringPool.internIt( str ); }
	const char* string_resolve( uint32_t id ) const { return stringPool.resolveId( id ); }

	uint32_t internString_internal( const char* str ) { return internalStringPool.internIt( str ); }
	const char* internalString_resolve( uint32_t id ) const { return internalStringPool.resolveId( id ); }
	
	size_t getMaxAnalyticalModeMaxSeqLength() const { return d_maxAnalyticalModeMaxSeqLength; }

	EntPropCompatibility entCompatibility;

	bool isAnalyticalMode() const { return d_isAnalyticalMode; }
	void setAnalyticalMode() { d_isAnalyticalMode= true; }

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
	BELTrie* getTrie( const std::string& cs, const std::string& ids ) 
		{ return globalTriePool.getTrie( cs, ids ); }

	BELTrie* produceTrie( const std::string& trieClass, const std::string& trieId ) 
        { return globalTriePool.produceTrie( trieClass, trieId ) ; }
};

class BZSpell;

class StoredUniverse {
	uint32_t d_userId;
	GlobalPools& gp;

	UniverseTrieCluster          trieCluster; 
	UniverseTrieCluster          topicTrieCluster; 

	BarzerHunspell      hunspell;

	BZSpell*             bzSpell; 
	
	typedef boost::unordered_map< uint32_t, bool > StringIdSet;

	/// 
	StringIdSet userSpecificStringSet;
	// bool  d_stemByDefault; 

	friend class QSemanticParser;

	void addWordsFromTriesToBZSpell();
public:
    // doesnt destroy the actual tries
    void clearTrieList() { 
        trieCluster.clearList();
    }
    
    typedef boost::shared_mutex Mutex;
    mutable Mutex d_theMutex;

    const Mutex& getMutex() const { return d_theMutex; }
    Mutex& getMutex() { return d_theMutex; }

    typedef boost::unique_lock< Mutex > WriteLock;
    typedef boost::shared_lock< Mutex >  ReadLock;

	const BZSpell* getBZSpell() const { return bzSpell; }
	BZSpell* getBZSpell() { return bzSpell; }
	BZSpell* initBZSpell( const StoredUniverse* secondaryUniverse = 0);
    // clears the dictionary
    void clearSpelling();
	/// result of spelling correction is in out
	/// it attempts to do first pass spelling correction (that is correction to a word known to the user)
	/// uses bzSpell
	// returns stringId of corrected word or 0xffffffff 
	uint32_t spellCorrect( const char* word ) const
		{ return ( bzSpell ? bzSpell->getSpellCorrection( word ) : 0 ); }
	//  performs trivial practical stemming
	// returns stringId of corrected word or 0xffffffff 
	uint32_t stem( std::string& out, const char* word ) const
		{ return ( bzSpell ? bzSpell->getStemCorrection( out, word ) : 0 ); }
	bool isWordValidInUniverse( const char* word ) const
		{ return ( bzSpell ? bzSpell->isWordValidInUniverse( word ) : true ); }

	const UniverseTrieCluster::BELTrieList& getTrieList() const { return trieCluster.getTrieList(); }
	UniverseTrieCluster::BELTrieList& getTrieList() { return trieCluster.getTrieList(); }

	/// adds uer specific strings from extra file 
	/// 
	size_t   internString( const char*, bool asUserSpecific, uint8_t frequency );
	bool 	 isStringUserSpecific( const char* s ) const
		{ 
			uint32_t id = gp.string_getId( s );
			if( id == 0xffffffff ) return false;
			return userSpecificStringSet.find(id) != userSpecificStringSet.end(); 
		}

    enum { GENERIC_CLASS_RANGE_MAX= 100 };

	uint32_t getUserId() const { return d_userId; }
    uint32_t getEntClass() const { return ( GENERIC_CLASS_RANGE_MAX+ d_userId); }

	bool stemByDefault() const { return gp.parseSettings().stemByDefault(); }
	UniverseTrieCluster& getTrieCluster() { return trieCluster; }

	UniverseTrieCluster& getTopicTrieCluster() { return topicTrieCluster; }
	const UniverseTrieCluster& getTopicTrieCluster() const { return topicTrieCluster; }
    bool hasTopics() const { return !(topicTrieCluster.getTrieList().empty()); }

	const UniverseTrieCluster& getTrieCluster() const { return trieCluster; }
	BarzerHunspell& getHunspell() { return hunspell; }
	const BarzerHunspell& getHunspell() const { return hunspell; }

	size_t getMaxAnalyticalModeMaxSeqLength() const { return gp.getMaxAnalyticalModeMaxSeqLength(); }
	bool isAnalyticalMode() const { return gp.isAnalyticalMode(); }
	EntPropCompatibility& getEntPropIndex() { return gp.entCompatibility; }
	const EntPropCompatibility& getEntPropIndex() const { return gp.entCompatibility; }
	BELTrie& produceTrie( const std::string& trieClass, const std::string& trieId ) 
		{ return *(gp.globalTriePool.produceTrie( trieClass, trieId )); }

	GlobalPools& getGlobalPools() { return gp; }
	const GlobalPools& getGlobalPools() const { return gp; }

	explicit StoredUniverse(GlobalPools& gp, uint32_t id );
	const DtaIndex& getDtaIdx() const { return gp.dtaIdx; }
		  DtaIndex& getDtaIdx() 	  { return gp.dtaIdx; }

	size_t getEclassEntCount( const StoredEntityClass& eclass ) const { return getDtaIdx().getEclassEntCount( eclass ); }
	BELTrie& getSomeTrie() { return *(trieCluster.getFirstTrie()); }
	const BELTrie& getSomeTrie() const { return *(trieCluster.getFirstTrie()); }
	const BELTrie& getBarzelTrie( const UniverseTrieClusterIterator& trieClusterIter ) const { return trieClusterIter.getCurrentTrie(); }
		  BELTrie& getBarzelTrie( UniverseTrieClusterIterator& trieClusterIter )  { return trieClusterIter.getCurrentTrie(); }

	      BarzelCompWordPool& getCompWordPool()       { return gp.compWordPool; }
	const BarzelCompWordPool& getCompWordPool() const { return gp.compWordPool; }

	const BarzelRewriterPool& getRewriterPool( const BELTrie& trie ) const { return trie.getRewriterPool(); }
		  BarzelRewriterPool& getRewriterPool( BELTrie& trie ) { return trie.getRewriterPool(); }
	const BarzelWildcardPool& getWildcardPool( const BELTrie& trie) const { return trie.getWildcardPool(); } 
		  BarzelWildcardPool& getWildcardPool( BELTrie& trie ) { return trie.getWildcardPool(); } 
	const BarzelFirmChildPool& getFirmChildPool( const BELTrie& trie ) const { return trie.getFirmChildPool(); }
		  BarzelFirmChildPool& getFirmChildPool( BELTrie& trie ) { return trie.getFirmChildPool(); }
	const BarzelTranslationPool& getTranslationPool( const BELTrie& trie ) const { return trie.getTranslationPool(); }
		BarzelTranslationPool& getTranslationPool( BELTrie& trie ) { return trie.getTranslationPool(); }

	const BarzelWCLookup* getWCLookup( const BELTrie& trie, uint32_t id ) const
	{
		return getWildcardPool(trie).getWCLookup( id );
	}

	// had to add it in order to be able to create BELPrintContext
    // for custom trie printing
	ay::UniqueCharPool& getStringPool() {
		return gp.stringPool;
	}

	const ay::UniqueCharPool& getStringPool() const {
		return gp.stringPool;
	}


	// clears all tries 
	void clearTries();
	void clearTopicTries();
	void clearSpell();
	// purges everything 
	void clear();

	const char* printableStringById( uint32_t id )  const
		{ return gp.stringPool.printableStr(id); }
	
	bool getBarzelRewriter( const BELTrie& trie, BarzelRewriterPool::BufAndSize& bas, const BarzelTranslation& tran ) const
	{
		if( tran.isRewriter() ) 
			return getRewriterPool(trie).resolveTranslation( bas, tran );
		else 
			return ( bas = BarzelRewriterPool::BufAndSize(), false );
	}

	bool isBarzelTranslationFallible( const BELTrie& trie, const BarzelTranslation& tran ) const {
		return tran.isFallible( getRewriterPool(trie) );
	}

	const BELFunctionStorage& getFunctionStorage() const { return gp.funSt; }
	const DateLookup& getDateLookup() const { return gp.dateLookup; }
	
	const char* getGenericSubclassName( uint16_t subcl ) const;

	const char* decodeStringById( uint32_t strId ) const 
		{ return getDtaIdx().resolveStringById( strId ); }

    void appendTriePtr( BELTrie* trie ) { trieCluster.appendTriePtr(trie); }
}; 

inline StoredUniverse& GlobalPools::produceUniverse( uint32_t id )
{
	StoredUniverse * p = getUniverse( id );
	if( !p ) { 
		p = new StoredUniverse( *this, id );
		d_uniMap[ id ] = p;	
	}
	return *p;
}

}
#endif // BARZER_UNIVERSE_H
