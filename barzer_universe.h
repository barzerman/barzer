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
#include <barzer_global_pools.h>

struct sb_stemmer;

namespace ay { struct CommandLineArgs; }
namespace zurch { 
    class DocIndexAndLoader; 
}

namespace barzer {

class BZSpell;

class Ghettodb;
class MeaningsStorage;
class BarzerGeo;

class StoredUniverse {
	uint32_t d_userId;
    std::string d_userName;
public:
	GlobalPools& gp;

    boost::unordered_map< uint32_t, zurch::DocIndexAndLoader* > d_zurchIndexPool;
    
    zurch::DocIndexAndLoader* initZurchIndex( uint32_t idxId ); 
    zurch::DocIndexAndLoader* getZurchIndex( uint32_t idxId );
    const zurch::DocIndexAndLoader* getZurchIndex( uint32_t idxId ) const;
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
	BarzerGeo *m_geo;

	bool m_soundsLike;
	
	EntReverseLookup m_revEntLookup;
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
		UBIT_FEATURED_SPELLCORRECT, // when 1, feature extraction-based spell corrector will be active
		UBIT_NEED_CONFIDENCE, // when 1 output will contain confidence info(which needs to be separately computed)

        /// extra normalization includes single quotes, utf8 punctuation etc.
        UBIT_NO_EXTRA_NORMALIZATION, /// when set no extra normalization is performed
        /// add new bits above this line only 
        UBIT_MAX
    };
private:
    EntityData entData; // canonic names and relevance
    ay::bitflags<UBIT_MAX> d_biflags;
public:
    EntityData::EntProp*  setEntPropData( const StoredEntityUniqId& euid, const char* name, uint32_t rel, bool overrideName=false ) 
        { return entData.setEntPropData(euid, name,rel, overrideName); }

    const EntityData::EntProp* getEntPropData( const BarzerEntity& ent ) const 
    {
        if( const EntityData::EntProp* eprop = entData.getEntPropData(ent) ) 
            return eprop;
        else if( gp.isGenericEntity(ent) ) 
            return gp.getEntPropData(ent);
        else
            return 0;
    }
    EntityData::EntProp* getEntPropData( const BarzerEntity& ent ) 
        { 
            return const_cast<EntityData::EntProp*>( const_cast<const StoredUniverse*>(this)->getEntPropData(ent) );
        }
    uint32_t getEntityRelevance( const BarzerEntity& ent ) const
    {
        if( const EntityData::EntProp* eprop = entData.getEntPropData(ent) ) {
            return eprop->relevance;
        } else if( gp.isGenericEntity(ent) ) {
            return gp.getEntityRelevance(ent);
        } else 
            return 0;
    }
    const char* getEntIdString(const BarzerEntity& euid ) const { return getGlobalPools().internalString_resolve(euid.tokId); }

    const std::string& userName() const { return d_userName; }
    void setUserName(const char* n) { d_userName.assign(n); }

	EntReverseLookup& getEntRevLookup() { return m_revEntLookup; }
	const EntReverseLookup& getEntRevLookup() const { return m_revEntLookup; }

	      MeaningsStorage& meanings()       { return *m_meanings; }
	const MeaningsStorage& meanings() const { return *m_meanings; }
	
	bool soundsLikeEnabled() const { return m_soundsLike; }
	void setSoundsLike(bool set) { m_soundsLike = set; }

    void setUBits( const char* ) ;
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
    void clearMeanings();
    void clearGeo();
	/// result of spelling correction is in out
	/// it attempts to do first pass spelling correction (that is correction to a word known to the user)
	/// uses bzSpell
	// returns stringId of corrected word or 0xffffffff
	uint32_t spellCorrect( const char* word, bool doStemCorrect = true ) const
		{ return ( bzSpell ? bzSpell->getSpellCorrection( word, doStemCorrect, LANG_UNKNOWN ) : 0 ); }
	//  performs trivial practical stemming
	// returns stringId of corrected word or 0xffffffff
	uint32_t stemCorrect( std::string& out, const char* word) const
		{ return ( bzSpell ? bzSpell->getStemCorrection( out, word, LANG_UNKNOWN, BZSpell::CORRECTION_MODE_NORMAL) : 0 ); }
    bool stem( std::string& out, const char* word ) const
    {
        return ( bzSpell ? bzSpell->stem( out, word ) : false );
    }
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

	const BarzerGeo* getGeo() const { return m_geo; }
	BarzerGeo* getGeo() { return m_geo; }
    const char* resolveLiteral( const BarzerLiteral& l ) const
        { return gp.resolveLiteral(l);}
    const char* resolveStoredTok( const StoredToken& stok ) const
    {
        return gp.string_resolve( stok.getStringId() );
    }
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
