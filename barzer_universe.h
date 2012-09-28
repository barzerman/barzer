#ifndef BARZER_UNIVERSE_H
#define BARZER_UNIVERSE_H

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

struct sb_stemmer;

namespace ay
{
	struct CommandLineArgs;
}

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
	ay::StemThreadPool m_stemPool;

	ay::UTF8TopicModelMgr *m_utf8langModelMgr;
	ay::ASCIITopicModelMgr *m_asciiLangModelMgr;
public:
	inline ay::UTF8TopicModelMgr* getUTF8LangMgr() const { return m_utf8langModelMgr; }
	inline ay::ASCIITopicModelMgr* getASCIILangMgr() const { return m_asciiLangModelMgr; }

	inline ay::StemThreadPool& getStemPool() { return m_stemPool; }

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
    EntityData entData; // canonic names and relevance

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

    void init_cmdline( ay::CommandLineArgs & );
	const GlobalTriePool& getTriePool() const { return  globalTriePool; }
};

class BZSpell;

class Ghettodb;
class MeaningsStorage;

class StoredUniverse {
	uint32_t d_userId;
    std::string d_userName;
public:
	GlobalPools& gp;
private:
	UniverseTrieCluster          trieCluster;
	UniverseTrieCluster          topicTrieCluster;

	BZSpell*             bzSpell;
    Ghettodb*            d_ghettoDb;

	BarzHints m_hints;

	typedef boost::unordered_map< uint32_t, bool > StringIdSet;

	///
	StringIdSet userSpecificStringSet;
	// bool  d_stemByDefault;

	friend class QSemanticParser;
    TopicEntLinkage d_topicEntLinkage;
    EntitySegregatorData d_entSeg;

	BarzerLocale_ptr m_defLocale;
	std::vector<BarzerLocale_ptr> m_otherLocales;
    LangInfoArray    d_langInfo;
    TokenizerStrategy d_tokenizerStrat;

	void addWordsFromTriesToBZSpell();
	
	MeaningsStorage *m_meanings;

	bool m_soundsLike;
public:
    /// much fancier interner than the overloaded one - this function will try to work with the trie 
    StoredToken& internString( int lang, const char* t, BELTrie* triePtr, const char* unstemmed);
    uint32_t stemAndIntern( int& lang, const char* s, size_t lem, BELTrie* triePtr );

    TokenizerStrategy& tokenizerStrategy() { return d_tokenizerStrat; }
    const TokenizerStrategy& getTokenizerStrategy() const { return d_tokenizerStrat; }
    enum {
        UBIT_NOSTRIP_DIACTITICS, // when set diacritics wont be stripped and utf8 is going to be processed as is 
        UBIT_TWOSTAGE_TOKENIZER, // when 1 tries to tokenize in 2 stages - first just by spaces then by everything
        UBIT_NO_ENTRELEVANCE_SORT, // when 1 wont sort entities by relevance on output
        UBIT_CORRECT_FROM_DICTIONARY, // when 1 will correct words away from generic dictionary otherwise leaves those words intact
		UBIT_LEX_STEMPUNCT, // when 1, space_default token classifier will consider <term>'s (and other similar) as <term> if <term> is valid

        /// add new bits above this line only 
        UBIT_MAX
    };
private:
    ay::bitflags<UBIT_MAX> d_biflags;
public:
    uint32_t getEntityRelevance( const BarzerEntity& ent ) const
    {
        EntityData::EntProp* eprop = gp.entData.getEntPropData(ent);
        return( eprop ? eprop->relevance : 0 );
    }
    const std::string& userName() const { return d_userName; }
    void setUserName(const char* n) { d_userName.assign(n); }

	      MeaningsStorage& meanings()       { return *m_meanings; }
	const MeaningsStorage& meanings() const { return *m_meanings; }
	
	bool soundsLikeEnabled() const { return m_soundsLike; }
	void setSoundsLike(bool set) { m_soundsLike = set; }

    void setBit(size_t bit) { d_biflags.set(bit); }
    bool checkBit(size_t b ) const { return d_biflags.checkBit(b); }
    const BZSpell::char_cp_to_strid_map* getValidWordMapPtr() const 
        { return bzSpell->getValidWordMapPtr(); }
        
    const LangInfoArray& getLangInfo() const { return d_langInfo; }
    LangInfoArray& getLangInfo() { return d_langInfo; }

    uint32_t recordLangWord( int16_t lang );
    const TopicEntLinkage::BarzerEntitySet*  getTopicEntities( const BarzerEntity& t ) const
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

	void addLocale(BarzerLocale_ptr locale, bool isDefault);
	BarzerLocale_ptr getDefaultLocale() const { return m_defLocale; }
	const std::vector<BarzerLocale_ptr>& getOtherLocales() const { return m_otherLocales; }

	const BZSpell* getBZSpell() const { return bzSpell; }
	BZSpell* getBZSpell() { return bzSpell; }
	BZSpell* initBZSpell( const StoredUniverse* secondaryUniverse = 0);
    // clears the dictionary
    void clearSpelling();
	/// result of spelling correction is in out
	/// it attempts to do first pass spelling correction (that is correction to a word known to the user)
	/// uses bzSpell
	// returns stringId of corrected word or 0xffffffff
	uint32_t spellCorrect( const char* word, bool doStemCorrect = true ) const
		{ return ( bzSpell ? bzSpell->getSpellCorrection( word, doStemCorrect ) : 0 ); }
	//  performs trivial practical stemming
	// returns stringId of corrected word or 0xffffffff
	uint32_t stem( std::string& out, const char* word) const
		{ return ( bzSpell ? bzSpell->getStemCorrection( out, word) : 0 ); }
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

	size_t getMaxAnalyticalModeMaxSeqLength() const { return gp.getMaxAnalyticalModeMaxSeqLength(); }
	bool isAnalyticalMode() const { return gp.isAnalyticalMode(); }
	EntPropCompatibility& getEntPropIndex() { return gp.entCompatibility; }
	const EntPropCompatibility& getEntPropIndex() const { return gp.entCompatibility; }
	BELTrie& produceTrie( uint32_t trieClass, uint32_t trieId )
		{ return *(gp.globalTriePool.produceTrie( trieClass, trieId )); }

	GlobalPools& getGlobalPools() { return gp; }
	const GlobalPools& getGlobalPools() const { return gp; }

	explicit StoredUniverse(GlobalPools& gp, uint32_t id );
    ~StoredUniverse();

	const DtaIndex& getDtaIdx() const { return gp.dtaIdx; }
		  DtaIndex& getDtaIdx() 	  { return gp.dtaIdx; }

	const BarzHints& getBarzHints() const { return m_hints; }
	BarzHints& getBarzHints() { return m_hints; }

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

    const StoredToken* getStoredToken( const char* s ) const
        { return getDtaIdx().getStoredToken(s); }
    const char* printableTokenByid( uint32_t tokId ) const
    {
        const StoredToken *tok = gp.dtaIdx.tokPool.getTokByIdSafe(tokId);
        return( tok ? printableTokenByid(tok->stringId): "" );
    }
    const char* printableEntityId( const BarzerEntity& euid ) const
        { return printableTokenByid(euid.getTokId()); }

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

	//const char* getGenericSubclassName( uint16_t subcl ) const;

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

    const BELTrie* getRuleTrieByUniqueId( const BELTrie::UniqueTrieId& tid ) const
        { return trieCluster.getTrieByUniqueId(tid); }
    const BELTrie* getTopicTrieByUniqueId( const BELTrie::UniqueTrieId& tid ) const
        { return topicTrieCluster.getTrieByUniqueId(tid); }

    const BELTrie* getTrieByUniqueId( const BELTrie::UniqueTrieId& tid, bool topic = false )
        { return( topic ? getTopicTrieByUniqueId(tid) : getRuleTrieByUniqueId(tid) ); }
    const BELTrie* getTrieByUniqueId( uint32_t tc, uint32_t tid ) const
        { return getRuleTrieByUniqueId( BELTrie::UniqueTrieId(tc, tid)); }
    
    Ghettodb&       getGhettodb()       { return *d_ghettoDb; }
    const Ghettodb& getGhettodb() const { return *d_ghettoDb; }
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
