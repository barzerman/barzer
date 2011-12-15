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
#include <barzer_topics.h>


namespace barzer {

class StoredUniverse;
/// holds all tries in the system keyed by 2 strings
/// TrieClass and  TrieId
class GlobalTriePool {
	typedef std::map< uint32_t , BELTrie* > ClassTrieMap;
	typedef std::map< uint32_t , ClassTrieMap > TrieMap; 
	
	TrieMap d_trieMap;

	// every time new trie is added to d_trieMap its also added to 
	// d_triePool
	
	std::vector< BELTrie* > d_triePool;

	GlobalPools& d_gp;
	
public:
	const BELTrie* getTrie_byGlobalId( uint32_t n ) const { return( n< d_triePool.size() ? (d_triePool[n]) : 0 ); } 

	ClassTrieMap& produceTrieMap( uint32_t trieClass );

	ClassTrieMap* getTrieMap( uint32_t trieClass ) ;
	const ClassTrieMap* getTrieMap( uint32_t trieClass ) const;

	const BELTrie* getTrie( uint32_t trieClass, const uint32_t trieId ) const;
	const BELTrie* getTrie( const char* trieClass, const char* trieId ) const;

	BELTrie* getTrie( const char* trieClass, const char* trieId ) ;
	BELTrie* getTrie( uint32_t trieClass, uint32_t trieId ) ;

	BELTrie* produceTrie( uint32_t trieClass, uint32_t trieId ) ;
	BELTrie* mkNewTrie() ;
	BELTrie& init() 
		{ return *produceTrie( 0xffffffff, 0xffffffff ); }

    BELTrie* getDefaultTrie() { return getTrie(0xffffffff,0xffffffff); }

	GlobalTriePool( GlobalPools& gp) : d_gp(gp) { init(); }
	~GlobalTriePool();
};

typedef std::list< TheGrammar > TheGrammarList;

/// in general there are 2 clusters per customer installation 
/// the regular trie cluster and the topic trie one 
class UniverseTrieCluster {
	GlobalTriePool& d_triePool;
	StoredUniverse& d_universe;

public:
    
	// typedef std::list< BELTrie* > TheGrammarList; BELTrieList
private:
	TheGrammarList d_trieList;
	friend class UniverseTrieClusterIterator;

public: 
	const TheGrammarList& getTrieList() const { return d_trieList; }

	UniverseTrieCluster( GlobalTriePool& triePool, StoredUniverse& u ) ;

	BELTrie& appendTrie( const char* , const char* , GrammarInfo* gi );
	BELTrie& appendTrie( uint32_t trieClass, uint32_t trieId, GrammarInfo* gi );
	BELTrie*  getFirstTrie() { 
        return ( d_trieList.empty() ?  0 : d_trieList.begin()->triePtr() ); 
    }
	const BELTrie*  getFirstTrie() const { return ( d_trieList.empty() ?  0 : d_trieList.begin()->triePtr()) ; }

    void clearList() { 
        for( TheGrammarList::iterator i = d_trieList.begin(); i!= d_trieList.end(); ++i ) 
            delete i->grammarInfo();

        d_trieList.clear(); 
    }
    ~UniverseTrieCluster() { clearList(); }

    void appendTriePtr( BELTrie* trie, GrammarInfo* gi ) { 

        d_trieList.push_back( TheGrammar(trie,gi) ); 
    }
    void clearTries();
};


class UniverseTrieClusterIterator {
	const UniverseTrieCluster& d_cluster;
	TheGrammarList::const_iterator d_i;
public:
	UniverseTrieClusterIterator( const UniverseTrieCluster& cluster ) : 
		d_cluster( cluster),
		d_i( cluster.getTrieList().begin() )
	{}
	
	void reset() { d_i = d_cluster.getTrieList().begin(); }
	const BELTrie& getCurrentTrie() const
		{ return (d_i->trie()); }
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


	uint32_t internalString_getId( const char* str ) const { return internalStringPool.getId( str ); }
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
	BELTrie* getTrie( uint32_t cs, uint32_t ids ) { return globalTriePool.getTrie( cs, ids ); }

	BELTrie* getTrie( const char* cs, const char* ids ) { return globalTriePool.getTrie( cs, ids ); }

	BELTrie* produceTrie( uint32_t trieClass, uint32_t trieId ) 
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
    TopicEntLinkage d_topicEntLinkage;
    EntitySegregatorData d_entSeg;

	void addWordsFromTriesToBZSpell();
public:
    const std::set< BarzerEntity >* getTopicEntities( const BarzerEntity& t ) const 
        { return d_topicEntLinkage.getTopicEntities( t ); }

    const TopicEntLinkage& getTopicEntLinkage() const { return  d_topicEntLinkage; }
    // doesnt destroy the actual tries
    void clearTrieList() { 
        trieCluster.clearList();
        d_topicEntLinkage.clear();
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
	bool isWordValidInUniverse( uint32_t word ) const
		{ return ( bzSpell ? bzSpell->isWordValidInUniverse( word ) : true ); }
	bool isWordValidInUniverse( const char* word ) const
		{ return ( bzSpell ? bzSpell->isWordValidInUniverse( word ) : true ); }

	const TheGrammarList& getTrieList() const { return trieCluster.getTrieList(); }

	/// adds uer specific strings from extra file 
	/// 
	size_t   internString( const char*, bool asUserSpecific, uint8_t frequency );
	bool 	 isStringUserSpecific( uint32_t id ) const
        { return( id == 0xffffffff ? false : (userSpecificStringSet.find(id) != userSpecificStringSet.end()) ); }
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
	// UniverseTrieCluster& getTrieCluster() { return trieCluster; }

	const UniverseTrieCluster& getTopicTrieCluster() const { return topicTrieCluster; }
    bool hasTopics() const { return !(topicTrieCluster.getTrieList().empty()); }

	const UniverseTrieCluster& getTrieCluster() const { return trieCluster; }
	BarzerHunspell& getHunspell() { return hunspell; }
	const BarzerHunspell& getHunspell() const { return hunspell; }

	size_t getMaxAnalyticalModeMaxSeqLength() const { return gp.getMaxAnalyticalModeMaxSeqLength(); }
	bool isAnalyticalMode() const { return gp.isAnalyticalMode(); }
	EntPropCompatibility& getEntPropIndex() { return gp.entCompatibility; }
	const EntPropCompatibility& getEntPropIndex() const { return gp.entCompatibility; }
	BELTrie& produceTrie( uint32_t trieClass, uint32_t trieId ) 
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
	// void clearTries();
	// void clearTopicTries();
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

    void appendTriePtr( BELTrie* trie, GrammarInfo* gi ) { 
        trieCluster.appendTriePtr(trie,gi); 
        d_topicEntLinkage.append( trie->getTopicEntLinkage() );
    }
	BELTrie& appendTopicTrie( uint32_t trieClass, uint32_t trieId, GrammarInfo* gi )
        { return topicTrieCluster.appendTrie( trieClass, trieId, gi ); }
	BELTrie& appendTopicTrie( const char* trieClass, const char* trieId, GrammarInfo* gi )
        { return topicTrieCluster.appendTrie( trieClass, trieId, gi ); }

	BELTrie& appendTrie( uint32_t trieClass, uint32_t trieId, GrammarInfo* gi )
    {
	    BELTrie& t = trieCluster.appendTrie( trieClass, trieId, gi );
        d_topicEntLinkage.append( t.getTopicEntLinkage() );
        return t;
    }
	BELTrie& appendTrie( const char* trieClass, const char* trieId, GrammarInfo* gi )
    {
	    BELTrie& t = trieCluster.appendTrie( trieClass, trieId, gi );
        d_topicEntLinkage.append( t.getTopicEntLinkage() );
        return t;
    }
    
    // entity segregation
    const EntitySegregatorData& geEntSeg() const { return d_entSeg; }
    bool needEntitySegregation() const { return !(d_entSeg.empty()); }
    void addEntClassToSegregate( const StoredEntityClass& ec ) { d_entSeg.add(ec); }
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
