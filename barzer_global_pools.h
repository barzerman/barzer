#pragma once

#include <ay_string_pool.h>
#include <ay_snowball.h>
#include <ay_ngrams.h>
#include <barzer_el_rewriter.h>
#include <barzer_el_wildcard.h>
#include <barzer_el_trie.h>
#include <barzer_dtaindex.h>
#include <barzer_el_function.h>
#include <barzer_date_util.h>
#include <barzer_settings.h>
#include <barzer_config.h>
#include <barzer_bzspell.h>
#include <barzer_el_compwords.h>
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#include <boost/thread/thread.hpp>
#include <barzer_topics.h>
#include <barzer_locale.h>
#include <barzer_language.h>
#include <barzer_barz.h>
#include <barzer_global_pools.h>
namespace ay { struct CommandLineArgs; }
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

    typedef std::map< BELTrie::UniqueTrieId, const BELTrie* > UniqIdTrieMap;
    UniqIdTrieMap d_ownTrieMap;
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
        d_ownTrieMap.clear();
    }
    ~UniverseTrieCluster() { clearList(); }

    void appendTriePtr( BELTrie* trie, GrammarInfo* gi ) {

        d_trieList.push_back( TheGrammar(trie,gi) );
        d_ownTrieMap.insert( UniqIdTrieMap::value_type( trie->getUniqueTrieId(), trie));
    }
    const BELTrie* getTrieByUniqueId( const BELTrie::UniqueTrieId& tid ) const
    {
        UniqIdTrieMap::const_iterator i = d_ownTrieMap.find(tid);
        return ( i == d_ownTrieMap.end() ? i->second : 0 );
    }
    const BELTrie* getTrieByUniqueId( uint32_t tc, uint32_t tid ) const
        { return getTrieByUniqueId( BELTrie::UniqueTrieId(tc,tid) ); }
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
	GlobalPools(GlobalPools&);
	GlobalPools& operator=(GlobalPools&);
	typedef boost::unordered_set< uint32_t > DictionaryMap;
	DictionaryMap d_dictionary;
    stem_to_srcs_map d_stemSrcs;

	ay::UTF8TopicModelMgr *m_utf8langModelMgr;
	ay::ASCIITopicModelMgr *m_asciiLangModelMgr;

    EntityData entData; // canonic names and relevance
public:
    EntityData& getEntData() { return entData; }
    const EntityData& getEntData() const { return entData; }
    EntityData::EntProp*  setEntPropData( const StoredEntityUniqId& euid, const char* name, uint32_t rel, bool overrideName=false ) 
        { return entData.setEntPropData(euid, name,rel, overrideName); }
    const EntityData::EntProp* getEntPropData( const BarzerEntity& ent ) const 
        { return entData.getEntPropData(ent); }
    EntityData::EntProp* getEntPropData( const BarzerEntity& ent )
        { return entData.getEntPropData(ent); }

    uint32_t getEntityRelevance( const BarzerEntity& ent ) const
    {
        if( const EntityData::EntProp* eprop = getEntPropData(ent) )
            return eprop->relevance;

        return 0;
    }
    bool isGenericEntity( const BarzerEntity& ent ) const
    {
        return ( ent.getEntityClass() );
    }
	inline ay::UTF8TopicModelMgr* getUTF8LangMgr() const { return m_utf8langModelMgr; }
	inline ay::ASCIITopicModelMgr* getASCIILangMgr() const { return m_asciiLangModelMgr; }

    void addStemSrc ( uint32_t stemId, uint32_t srcId ) { d_stemSrcs[stemId].insert(srcId); }

	const strIds_set* getStemSrcs ( uint32_t stemId ) const { 
        stem_to_srcs_map::const_iterator i  = d_stemSrcs.find(stemId);
        return ( i == d_stemSrcs.end() ? 0 : &(i->second) );
    }
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

	uint32_t string_intern( const char* str, size_t s_len ) { return stringPool.internIt( str, s_len ); }
	uint32_t string_intern( const char* str ) { return stringPool.internIt( str ); }
	const char* string_resolve( uint32_t id ) const { return stringPool.resolveId( id ); }

	uint32_t internString_internal( const char* str, size_t s_len ) { return internalStringPool.internIt( str, s_len ); }
	uint32_t internString_internal( const char* str ) { return internalStringPool.internIt( str ); }
	const char* internalString_resolve( uint32_t id ) const { return internalStringPool.resolveId( id ); }

	const char* internalString_resolve_safe( uint32_t id ) const { 
        const char* s = internalStringPool.resolveId( id );
        return ( s ? s: "" );
    }

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

	//void createGenericEntities();
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
    /// when fullMode == false some initialization is omitted (this is a performance tweak for emitter etc)
	GlobalPools( bool fullMode = true);
	~GlobalPools();
    const UniverseMap& getUniverses() const { return d_uniMap; }
	const char* decodeStringById( uint32_t strId ) const
		{ return dtaIdx.resolveStringById( strId ); }
	/// never returns 0
	const char* decodeStringById_safe( uint32_t strId ) const
		{ const char* str = decodeStringById(strId); return (str ? str :"" ); }
	const BELTrie* getTrie( uint32_t cs, uint32_t ids ) const { return globalTriePool.getTrie( cs, ids ); }
	BELTrie* getTrie( uint32_t cs, uint32_t ids ) { return globalTriePool.getTrie( cs, ids ); }

	BELTrie* getTrie( const char* cs, const char* ids ) { return globalTriePool.getTrie( cs, ids ); }

	BELTrie* produceTrie( uint32_t trieClass, uint32_t trieId )
        { return globalTriePool.produceTrie( trieClass, trieId ) ; }

    const char* resolveLiteral( const BarzerLiteral& l ) const
        { return stringPool.resolveId(l.getId()); }

    void init_cmdline( ay::CommandLineArgs & );
	const GlobalTriePool& getTriePool() const { return  globalTriePool; }
};

class EntReverseLookup
{
	typedef boost::unordered_map<StoredEntityUniqId, std::vector<BarzelTranslationTraceInfo>> dict_t;
	dict_t m_hash;
public:
	void add(const StoredEntityUniqId& triple, const BarzelTranslationTraceInfo& info)
	{
		auto pos = m_hash.find(triple);
		if (pos == m_hash.end())
			pos = m_hash.insert(dict_t::value_type(triple, dict_t::mapped_type())).first;
		pos->second.push_back(info);
	}
	
	void lookup(const StoredEntityUniqId& triple, std::vector<BarzelTranslationTraceInfo>& out) const
	{
		auto pos = m_hash.find(triple);
		if (pos != m_hash.end())
			out = pos->second;
	}
};

} //namespace barzer
