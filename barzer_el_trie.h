#pragma once
#include <barzer_elementary_types.h>
#include <barzer_el_rewriter.h>
#include <barzer_el_variable.h>
#include <barzer_el_entcol.h>
#include <barzer_el_proc.h>
#include <barzer_topics.h>
#include <barzer_meaning.h>
#include <ay_bitflags.h>
#include <ay_util.h>
#include <ay_pool_with_id.h>
#include <map>
#include <set>
#include <boost/thread/shared_mutex.hpp>
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#include <boost/thread/locks.hpp>
#include <barzer_el_translation.h>


/// data structures representing the Barzer Expression Language BarzEL term pattern trie
///
namespace barzer {
class BELTrie;
class BarzelRewriterPool;
class BarzelWildcardPool;

struct BELPrintContext;
struct BELTrieContext;
struct BELPrintFormat;

/// this type is used as a key by firmchild lookup (BarzelFCLookup)
struct BarzelTrieFirmChildKey {
	uint32_t id;
	uint8_t  type;
	// when noLeftBlanks is !=0
	// the firm child will only match if it immediately follows the prior token
	// in other words spaces wont be skipped (!)
	// tis is needed for patterns defining dates and such
	uint8_t  noLeftBlanks;
	uint8_t  d_matchMode;

	BarzelTrieFirmChildKey( uint8_t t, uint32_t i, bool nlb=true) :
        id(i),
        type(t),
        noLeftBlanks(nlb?0:1),
        d_matchMode(BTND_Pattern_Base::MATCH_MODE_NORMAL)
    {}

	BarzelTrieFirmChildKey() :
        id(0xffffffff),
        type(BTND_Pattern_None_TYPE),
        noLeftBlanks(0),
        d_matchMode(BTND_Pattern_Base::MATCH_MODE_NORMAL)  {}

	BarzelTrieFirmChildKey(const BTND_Pattern_Meaning& x) :
        id(x.meaningId),
        type((uint8_t)BTND_Pattern_Meaning_TYPE),
        noLeftBlanks(0),
        d_matchMode(x.d_matchMode)
        {}
	BarzelTrieFirmChildKey(const BTND_Pattern_Token& x) :
        id(x.stringId),
        type((uint8_t)BTND_Pattern_Token_TYPE),
        noLeftBlanks(0),
        d_matchMode(x.d_matchMode)
        {}
	BarzelTrieFirmChildKey(const BTND_Pattern_Punct& x) :
        id(x.theChar),
        type((uint8_t)BTND_Pattern_Punct_TYPE),
        noLeftBlanks(0),
        d_matchMode(x.d_matchMode)
    {}

	BarzelTrieFirmChildKey(const BTND_Pattern_CompoundedWord& x) :
		id(x.compWordId),
        type((uint8_t)BTND_Pattern_CompoundedWord_TYPE), noLeftBlanks(0),
        d_matchMode(x.d_matchMode)
	{}

    bool isNormalKey() const { return d_matchMode== BTND_Pattern_Base::MATCH_MODE_NORMAL; }
    bool isNegativeKey() const { return d_matchMode== BTND_Pattern_Base::MATCH_MODE_NEGATIVE; }
    void mkNegativeKey() {
        d_matchMode = BTND_Pattern_Base::MATCH_MODE_NEGATIVE;
        id = 0;
        type=0;
        noLeftBlanks=0;
    }
	// default followsBlank is false
	inline BarzelTrieFirmChildKey& setMeaning( uint32_t meaningId, bool followsBlank )
    {
        return( 
		    noLeftBlanks = ( followsBlank ? 0:1 ),
		    id =   meaningId,
            type=((uint8_t) BTND_Pattern_Meaning_TYPE),
            *this
        );
    }
	inline BarzelTrieFirmChildKey& set( const BarzerLiteral& dta, bool followsBlank )
	{
		noLeftBlanks = ( followsBlank ? 0:1 );
		id =   dta.getId();
		switch(dta.getType()) {
		case BarzerLiteral::T_STRING:   type=(uint8_t) BTND_Pattern_Token_TYPE; break;
		case BarzerLiteral::T_COMPOUND: type=(uint8_t) BTND_Pattern_CompoundedWord_TYPE; break;
		case BarzerLiteral::T_STOP:     type=(uint8_t) BTND_Pattern_StopToken_TYPE; break;
		case BarzerLiteral::T_PUNCT:    type=(uint8_t) BTND_Pattern_Punct_TYPE; break;
		case BarzerLiteral::T_MEANING:    type=(uint8_t) BTND_Pattern_Meaning_TYPE; break;
		case BarzerLiteral::T_BLANK:
			type = (uint8_t) BTND_Pattern_Token_TYPE;
			id =   0xffffffff;
			break;
		}
		return *this;
	}
	bool isLiteralKey() const
	{
		int it = (int)type;
		return(
			it == BTND_Pattern_Token_TYPE ||
			it == BTND_Pattern_StopToken_TYPE ||
			it == BTND_Pattern_Punct_TYPE ||
			it == BTND_Pattern_Meaning_TYPE ||
			it == BTND_Pattern_CompoundedWord_TYPE
		);
	}
	uint32_t getTokenStringId() const
		{ return ( type == BTND_Pattern_Token_TYPE ? id : 0xffffffff ); }
	void setNull( ) { type = BTND_Pattern_None_TYPE; id = 0xffffffff; noLeftBlanks=0;}

	bool isNull() const { return (type == BTND_Pattern_None_TYPE); }

	bool isBlankLiteral() const { return (BTND_Pattern_Token_TYPE ==type && id==0xffffffff); }
	bool isStopToken() const { return( BTND_Pattern_StopToken_TYPE == type ); }

	bool isBlankMeaning() const { return (BTND_Pattern_Meaning_TYPE ==type && id==0xffffffff); }
	bool isMeaning() const { return (BTND_Pattern_Meaning_TYPE ==type ); }

	bool isNoLeftBlanks() const { return noLeftBlanks; }
	std::ostream& print( std::ostream& , const BELPrintContext& ctxt ) const;

	std::ostream& print( std::ostream& ) const;
};

inline std::ostream& operator <<( std::ostream& fp, const BarzelTrieFirmChildKey& key )
{ return key.print(fp); }

struct BarzelTrieFirmChildKey_comp_less {
	inline bool operator() ( const BarzelTrieFirmChildKey& l, const BarzelTrieFirmChildKey& r ) const
		{
			return ay::range_comp().less_than(
				l.d_matchMode, l.type, r.id, // l.noLeftBlanks,
				r.d_matchMode, r.type, l.id// , r.noLeftBlanks
			);
			// return( l.id < r.id ? true : ( r.id < l.id ? false : (l.type < r.type)));
		}
};
inline bool operator <( const BarzelTrieFirmChildKey& l, const BarzelTrieFirmChildKey& r )
{ return BarzelTrieFirmChildKey_comp_less() ( l, r ); }

struct BarzelTrieFirmChildKey_comp_eq {
	inline bool operator() ( const BarzelTrieFirmChildKey& l, const BarzelTrieFirmChildKey& r ) const
		{ return ( l.id == r.id && l.type == r.type );}
};
inline bool operator ==( const BarzelTrieFirmChildKey& l, const BarzelTrieFirmChildKey& r )
{ return BarzelTrieFirmChildKey_comp_eq() ( l, r ); }

/// wildcard child key
struct BarzelWCKey {
	uint32_t wcId; // wildcard id unique for the type in a pool
	uint8_t wcType; // one of BTND_Pattern_XXXX_TYPE enums
	// longest span of the wildcard
	// for example if number wildcard has maxSpan of 1
	// only 1 bead long sequences will be considered candidates
	// for matching
	uint8_t maxSpan;
	uint8_t noLeftBlanks; // one of BTND_Pattern_XXXX_TYPE enums

	void clear()
		{ wcType = BTND_Pattern_None_TYPE; wcId= 0xffffffff; }
	bool isBlank() const
		{ return( wcType == BTND_Pattern_None_TYPE ); }
	BarzelWCKey() :
		wcId(0xffffffff),
		wcType(BTND_Pattern_None_TYPE),
		maxSpan(1),
		noLeftBlanks(0)
	{}
	void set( uint8_t span, bool followsBlank )
	{
		maxSpan= span;
		noLeftBlanks = ( followsBlank? 0: 1 );
	}

	// non trivial constructor - this may add wildcard to the pool
	// when fails wcType will be set to BTND_Pattern_None_TYPE
	// BarzelWCKey( BELTrie& trie, const BTND_PatternData& pat );

	inline bool lessThan( const BarzelWCKey& r ) const
	{
		if(
			ay::range_comp().less_than(
				maxSpan, 	wcType,
				r.maxSpan, 	r.wcType
			)
		) {
			return true;
		} else if(
			ay::range_comp().less_than(
				r.maxSpan, 	r.wcType,
				maxSpan, 	wcType
			)
		) {
			return false;
		} else
			return ( ay::range_comp().less_than(
				r.wcId, r.noLeftBlanks,
				  wcId,   noLeftBlanks
			) );
	}

	std::ostream& print( std::ostream& fp, const BarzelWildcardPool* ) const;
};


inline bool operator <( const BarzelWCKey& l, const BarzelWCKey& r )
{
	return l.lessThan( r );
}


/// barzel wildcard child lookup object
/// stored in barzel trie nodes

typedef std::pair<BarzelTrieFirmChildKey, BarzelWCKey> BarzelWCLookupKey;
inline bool operator< (const BarzelWCLookupKey& l, const BarzelWCLookupKey& r )
{
	return ay::range_comp().less_than(
		l.first.id, l.first.type, l.second, l.first.noLeftBlanks,
		r.first.id, r.first.type, r.second, r.first.noLeftBlanks
	);
}

class BarzelTrieNode;
typedef std::map<BarzelTrieFirmChildKey, BarzelTrieNode > BarzelFCMap;
typedef ay::PoolWithId< BarzelFCMap > BarzelFirmChildPool;
typedef ay::PoolWithId< BarzelTranslation > BarzelTranslationPool;


class BarzelTrieNode {
	const BarzelTrieNode* d_parent;
	uint32_t d_firmMapId; // when valid (not 0xffffffff) can it's an id of a firm lookup object  (BarzelFCMap)

	uint32_t d_wcLookupId; // when valid (not 0xffffffff) can it's an id of a wildcard lookup object
	uint32_t d_translationId;

	enum {
		B_WCCHILD, // it's a wildcard child of its parent

		// new bits strictly above this line
		B_MAX
	};
	ay::bitflags<B_MAX> d_flags;
	
	/// methods
	void clearFirmMap();
	void clearWCMap();

	// given the nature of the trie - it's extremely leaf heavy - most nodes will actually have
	// translation
	//BarzelTranslation d_translation;
public:
	/// these functions MAY return 0 if the node has no firm children
	uint32_t getFirmMapId() const { return d_firmMapId; }
	uint32_t getTranslationId() const { return d_translationId; }

	BarzelTranslation* getTranslation(BELTrie& trie);
	const BarzelTranslation* getTranslation(const BELTrie& trie) const;

	BarzelTrieNode():
		d_parent(0),
		d_firmMapId(0xffffffff),
		d_wcLookupId(0xffffffff) ,
		d_translationId(0xffffffff)
	{}
	BarzelTrieNode(const BarzelTrieNode* p ):
		d_parent(p),
		d_firmMapId(0xffffffff) ,
		d_wcLookupId(0xffffffff) ,
		d_translationId(0xffffffff)
	{}

	const BarzelTrieNode* getParent() const { return d_parent; }
	uint32_t getWCLookupId() const { return d_wcLookupId; }

	void clear();
	std::ostream& print_firmChildren( std::ostream& fp, BELPrintContext& ) const;
	std::ostream& print_wcChildren( std::ostream& fp, BELPrintContext& ) const;
	std::ostream& print_translation( std::ostream& fp, const BELPrintContext& ) const;

	bool hasFirmChildren() const { return (d_firmMapId!=0xffffffff); }

	bool hasWildcardChildren() const { return (d_wcLookupId != 0xffffffff ) ; }
	bool hasChildren() const
		{ return (hasWildcardChildren() || hasFirmChildren() ); }

	bool hasValidTranslation() const { return (d_translationId != 0xffffffff); }
	bool isLeaf() const { return hasValidTranslation(); }

	/// makes node leaf and sets translation
	void setTranslation(uint32_t tranId ) { d_translationId = tranId; }
	void clearTranslation() { setTranslation(0xffffffff); }

	// locates a child node or creates a new one and returns a reference to it. non-leaf by default
	// if pattern data cant be translated into a valid key the same node is returned
	/// so typically  when we want to add a chain of patterns as a path to the trie
	/// we will iterate over the chain calling node = node->addPattern(p) for each pattern
	/// and in the end we will call node->setTranslation()

	// if p is a non-firm kind (a wildcard) this is a no-op
	BarzelTrieNode* addFirmPattern( BELTrie& trie, const BarzelTrieFirmChildKey& key );

	// if p is firm (not a wildcard) this is a no-op
	BarzelTrieNode* addWildcardPattern( BELTrie& trie, const BTND_PatternData& p, const BarzelTrieFirmChildKey& fk  );


	bool hasValidWcLookup() const
		{ return (d_wcLookupId != 0xffffffff); }

	void setFlag_isWcChild() { d_flags.set( B_WCCHILD ); }
	bool isWcChild() const { return d_flags[ B_WCCHILD ]; }

	const BarzelTrieNode* getFirmChild( const BarzelTrieFirmChildKey& key, const BarzelFCMap& fcmap ) const
	{
		BarzelFCMap::const_iterator  i = fcmap.find( key );
		return ( i == fcmap.end() ? 0 : &(i->second) );
	}

	std::ostream& print( std::ostream& , BELPrintContext& ) const;
};

typedef std::map< BarzelWCLookupKey, BarzelTrieNode > BarzelWCLookup;
struct BELStatementParsed;
class GlobalPools;

/// one of these is stored for each unique strId in a trie
struct TrieWordInfo {
	uint32_t wordCount;     // number of time the word occurs as itself
    uint32_t stemWordCount; // number of times the word occurs as a stem

	TrieWordInfo() : wordCount(0), stemWordCount(0) {}
	void incrementCount(bool stemmed ) {
        if( stemmed )
            ++stemWordCount;
        else
            ++wordCount;
    }
    uint32_t getWordCount() const { return wordCount; }
    uint32_t getStemWordCount() const { return stemWordCount; }
};
typedef boost::unordered_map< uint32_t, TrieWordInfo > strid_to_triewordinfo_map;
typedef std::set< uint32_t > strIds_set;
typedef boost::unordered_map< uint32_t, strIds_set> stem_to_srcs_map;

struct BELTrieImperative {
    BarzelTranslation          translation;
    BarzelTranslationTraceInfo trace;
};

typedef std::vector< BELTrieImperative > BELTrieImperativeVec;

/// stores links for ambiguous translations
class AmbiguousTranslationReference {
public:
    typedef std::pair<BarzelTranslationTraceInfo, AmbiguousTraceInfo> DataUnit;
    typedef std::vector<DataUnit> Data;
private:
    std::map< uint32_t, Data > d_dataMap;
public:
    /// links (translation traceinfo, ambiguity id) pair to translation
    void link( uint32_t tranId, const BarzelTranslationTraceInfo&, const AmbiguousTraceId& );
    /// returns the number of ambiguous rules still linked to this translation after the removal
    size_t unlink( uint32_t tranId, const BarzelTranslationTraceInfo&, BELTrie& trie );
    size_t unlink( uint32_t tranId, uint32_t source, uint32_t statementId, BELTrie& trie ) 
        { return unlink( tranId, BarzelTranslationTraceInfo(source,statementId), trie); }
    void clear() { d_dataMap.clear(); }
    const AmbiguousTranslationReference::Data* data(uint32_t tranId) const
        { auto i = d_dataMap.find( tranId ); return( i == d_dataMap.end() ? 0 : &(i->second) ); }

    // iterates over all matching traceinfos and invokes the CB
    template <typename CB>
    void getTraceInfos( const CB& cb, uint32_t tranId ) const
    {
        auto i = d_dataMap.find( tranId );
        if( i != d_dataMap.end() ) for( const auto& x : i->second ) cb( x );
    }
};

class BELTrie {
	GlobalPools& globalPools;
	/// trie shouldnt be copyable
	BarzelRewriterPool* d_rewrPool;
	BarzelWildcardPool* d_wcPool;
	BarzelFirmChildPool* d_fcPool;
	BarzelTranslationPool *d_tranPool;
	BarzelVariableIndex   d_varIndex;
	EntityCollection      d_entCollection;

    TopicEntLinkage       d_topicEnt;
    
    typedef std::pair< const BarzelTrieNode*, uint32_t > TrieNodeStringIdPair;
    /// if pair (trie node, uint32_t id) is in the set this means that the id cant be matched from the trie node 
    /// in case the id is
    boost::unordered_set<TrieNodeStringIdPair> d_nostemSet;

	/// this trie's global id in the global trie pool
	uint32_t d_globalTriePoolId;
	// tokens participating in this trie will have this priority in the localized spelling corrector
	uint8_t d_spellPriority;

	strid_to_triewordinfo_map d_wordInfoMap;
	stem_to_srcs_map d_stemSrcs;

    AmbiguousTranslationReference d_ambTranRef;
    
	BELTrie( const BELTrie& a );
public:
    void storeNodeAsNoStem( const BarzelTrieNode* n, uint32_t id ) { 
        d_nostemSet.insert( TrieNodeStringIdPair{ n,id} ); 
    }
    bool isNodeNoStem( const BarzelTrieNode* n, uint32_t id ) const { 
        auto x = d_nostemSet.find( TrieNodeStringIdPair{ n, id } );
        return( x != d_nostemSet.end() ); 
    }

    AmbiguousTranslationReference& getAmbiguousTranslationReference() { return d_ambTranRef; }
    const AmbiguousTranslationReference& getAmbiguousTranslationReference() const { return d_ambTranRef; }

    void linkTraceInfoNodes( uint32_t tranId, const BarzelTranslationTraceInfo& trInfo, const BarzelTranslationTraceInfo& v, const std::vector<BarzelEvalNode::NodeID_t>& );
    void linkTraceInfoEnt( uint32_t tranId, const BarzelTranslationTraceInfo& trInfo, const BarzelTranslationTraceInfo& v, uint32_t );

    bool getLinkedTraceInfo( BarzelTranslationTraceInfo::Vec&, uint32_t tranId ) const;
    const TopicEntLinkage& getTopicEntLinkage() const { return d_topicEnt; }
    const TopicEntLinkage::BarzerEntitySet* getTopicEntities( const BarzerEntity& t ) const
        { return d_topicEnt.getTopicEntities( t ); }
    void linkEntToTopic( const BarzerEntity& topicEnt, const BarzerEntity&ent, uint32_t strength ) { d_topicEnt.link( topicEnt, ent, strength ); }

	// must be called from BELParser::internString
	void addWordInfo( uint32_t strId, bool stemmed );

	void addStemSrc ( uint32_t stemId, uint32_t srcId );
	const strIds_set* getStemSrcs ( uint32_t stemId ) const { 
        stem_to_srcs_map::const_iterator i  = d_stemSrcs.find(stemId);
        return ( i == d_stemSrcs.end() ? 0 : &(i->second) );
    }

	const strid_to_triewordinfo_map& getWordInfoMap() const { return d_wordInfoMap; }

	uint32_t getGlobalTriePoolId( ) const { return d_globalTriePoolId; }
	void 	 setGlobalTriePoolId( uint32_t i ){ d_globalTriePoolId = i; }

	uint8_t getSpellPriority( ) const { return d_spellPriority; }
	void setSpellPriority( uint8_t p ) { d_spellPriority= p; }

	typedef std::set< uint32_t > UseridSet;
	typedef boost::shared_mutex Lock;
	typedef boost::unique_lock< boost::shared_mutex > WriteLock;
	typedef boost::shared_lock< boost::shared_mutex >  ReadLock;
private:
	UseridSet d_userIdSet; // set of unique userids using this trie
	mutable Lock d_threadLock;
    // std::string d_trieClass, d_trieId;

    uint32_t d_trieClass_strId, d_trieId_strId;
public:
    UniqueTrieId getUniqueTrieId() const
        { return UniqueTrieId( d_trieClass_strId, d_trieId_strId ); }

    WordMeaningBufPtr getMeanings( const BarzerLiteral& l ) const;

    uint32_t getTrieClass_strId() const { return d_trieClass_strId; }
    uint32_t getTrieId_strId() const { return d_trieId_strId; }

    const char* getTrieClass() const;
    const char* getTrieId() const ;

    void setTrieClassAndId( uint32_t cn, uint32_t id ) { d_trieClass_strId= cn; d_trieId_strId= id; }

    void setTrieClassAndId( const char* c, const char* i ) ;
	Lock& getThreadLock() const { return d_threadLock; }

	// call in critical section only
	void registerUser( uint32_t uid ) { d_userIdSet.insert(uid); }
	void deRegisterUser( uint32_t uid ) { d_userIdSet.erase(uid); }

	const UseridSet& getUserIdSet() const { return d_userIdSet; }
	size_t getNumUsers() const { return d_userIdSet.size(); }

	BarzelWildcardPool*  getWCPoolPtr() const { return d_wcPool; }
	GlobalPools& getGlobalPools() { return globalPools; }
	const GlobalPools& getGlobalPools() const { return globalPools; }
	BarzelTrieNode root;

    //  imperatives . Imperative statement is a translation which is executed 
    //  unconditionally before or after the rewriting 
    //  its rvec always contains the entire bead list, as iw it has matched all beads 
    BELTrieImperativeVec d_imperativePre; // array of imperatives executed in order BEFORE rewriting 
    BELTrieImperativeVec d_imperativePost; // array of imperatives executed in order AFTER rewriting

	// allocates the pools
	void initPools();

	~BELTrie();
	BELTrie( GlobalPools& gp );
	BarzelMacros macros;
	BarzelProcs procs;

	const BarzelProcs& getProcs() const { return  procs; }
	BarzelProcs& getProcs() { return  procs; }
	const BarzelMacros& getMacros() const { return  macros; }
	BarzelMacros& getMacros() { return  macros; }

	const EntityGroup* getEntGroupById( uint32_t id ) const
	{ return d_entCollection.getEntGroup(id); }
	const EntityCollection& getEntityCollection() const { return       d_entCollection; }
	EntityCollection& getEntityCollection() { return       d_entCollection; }

		  BarzelRewriterPool& getRewriterPool() { return  * d_rewrPool; }
	const BarzelRewriterPool& getRewriterPool() const { return  * d_rewrPool; }

		  BarzelWildcardPool& getWildcardPool()  { return * d_wcPool; }
	const BarzelWildcardPool& getWildcardPool() const  { return * d_wcPool; }

		  BarzelFirmChildPool& getFirmChildPool() { return * d_fcPool; }
	const BarzelFirmChildPool& getFirmChildPool() const { return * d_fcPool; }

		  BarzelTranslationPool& getTranslationPool() { return *d_tranPool; }
	const BarzelTranslationPool& getTranslationPool() const { return *d_tranPool; }

	      BarzelVariableIndex& getVarIndex()       { return d_varIndex; }
	const BarzelVariableIndex& getVarIndex() const { return d_varIndex; }

	std::ostream& printVariableName( std::ostream& fp, uint32_t varId ) const;

	BarzelTranslation*  makeNewBarzelTranslation( uint32_t& id )
		{ 
            BarzelTranslation* tran =  d_tranPool->addObj( id ); 
            tran->tranId = id;
            return tran;
        }
	const BarzelTranslation* getBarzelTranslation( uint32_t tranId ) const { return d_tranPool->getObjById(tranId); }
	const BarzelTranslation* getBarzelTranslation( const BarzelTrieNode& node ) const { return d_tranPool->getObjById(node.getTranslationId()); }
		  BarzelTranslation* getBarzelTranslation( const BarzelTrieNode& node ) 	   { return d_tranPool->getObjById(node.getTranslationId()); }

    // for future use if we ever have "fallible" translations - the ones that we know may fail
    bool isTranslationFallible( const BarzelTranslation& tran ) const
        { return false; }
	/// tries to add another translation to the existing one.
	/// initially this will only work for entity lists
	bool tryAddingTranslation( BarzelTrieNode* n, uint32_t tranId, const BELStatementParsed& stmt, uint32_t emitterSeqNo, StoredUniverse* );

    /// called to remove an ellement of an ambiguous translation
    /// returns true if successful
	bool removeFromAmbiguousTranslation( const BarzelTranslation& tran, const AmbiguousTraceId& traceId );

	BarzelFCMap*  makeNewBarzelFCMap( uint32_t& id )
		{ return d_fcPool->addObj( id ); }
	const BarzelFCMap* getBarzelFCMap( const BarzelTrieNode& node ) const { return d_fcPool->getObjById(node.getFirmMapId()); }
		  BarzelFCMap* getBarzelFCMap( const BarzelTrieNode& node ) 	  { return d_fcPool->getObjById(node.getFirmMapId()); }

	/// stores wildcard data n a form later usable by the Trie
	/// this ends up calling d_wcPool->produceWCKey()
	void produceWCKey( BarzelWCKey&, const BTND_PatternData&   );

    void addImperative( const BELStatementParsed& stmt, bool pre );

	/// adds a new path to the  trie
	const BarzelTrieNode* addPath( const BELStatementParsed& stmt,
			const BTND_PatternDataVec& path, uint32_t tranId, const BELVarInfo& varInfo, uint32_t emitterSeqNo, StoredUniverse* );
	void setTanslationTraceInfo( BarzelTranslation& tran, const BELStatementParsed& stmt, uint32_t emitterSeqNo );
	std::ostream& printTanslationTraceInfo( std::ostream& , const BarzelTranslationTraceInfo& traceInfo ) const;
	BarzelWildcardPool&  getWCPool() { return *d_wcPool; }
	const BarzelWildcardPool&  getWCPool() const { return *d_wcPool; }

	BarzelTrieNode& getRoot() { return root; }
	const BarzelTrieNode& getRoot() const { return root; }

	/// print methods
	std::ostream& print( std::ostream&, BELPrintContext& ctxt ) const;

	void clear();
    void removeNode( BarzelTrieNode* );
};

inline BarzelTranslation* BarzelTrieNode::getTranslation(BELTrie& trie)
    { return trie.getBarzelTranslation( *this ); }
inline const BarzelTranslation* BarzelTrieNode::getTranslation(const BELTrie& trie) const
    { return trie.getBarzelTranslation( *this ); }

/// object necessary for meaningful printing
struct BELPrintFormat {
	enum {
		PFB_NODESCEND, // doesnt recursively print the children

		/// add new flags only above this line
		PFB_MAX
	};
	ay::bitflags<PFB_MAX> flags; // only prints children at the current level

	/// PFB_NODESCEND flag
	bool doDescend() const 	{ return (!flags[PFB_NODESCEND]); }
	void setNodescend() 	{ flags.set( PFB_NODESCEND ); }

};
struct BELTrieContext {
	BELTrie& trie;
	const ay::UniqueCharPool& strPool;

	BELTrieContext(
		BELTrie& t,
		const ay::UniqueCharPool& sp
	) :
		trie(t),
		strPool(sp)
	{}
};

struct BELPrintContext {
	const BELTrie& trie;
	const ay::UniqueCharPool& strPool;
	const BELPrintFormat& format;

	std::string prefix;
	int depth;
	enum { PREFIX_INDENT_SZ = 4 };

	void descend() { ++depth; prefix.resize( prefix.size() + PREFIX_INDENT_SZ, ' ' ); }
	void ascend() {
		if( depth > 0 )
			--depth;
		if( prefix.size() >= PREFIX_INDENT_SZ )
			prefix.resize( prefix.size() - PREFIX_INDENT_SZ );
	}
	BELPrintContext(
		const BELTrie& t,
		const ay::UniqueCharPool& sp ,
		const BELPrintFormat& f
	) :
		trie(t),
		strPool(sp),
		format(f),
		depth(0)
	{}


	const char* printableString( uint32_t id )  const
	{ return strPool.printableStr(id); }

	std::ostream& printVariableName( std::ostream& fp, uint32_t varId ) const
		{ return trie.printVariableName(fp,varId); }
	const BarzelWCLookup*  getWildcardLookup( uint32_t id ) const;

	bool needDescend() const
	{
		return( !depth || format.doDescend() );
	}

	std::ostream& printBarzelWCLookupKey( std::ostream& fp, const BarzelWCLookupKey& key ) const;

	std::ostream& printRewriterByteCode( std::ostream& fp, const BarzelTranslation& ) const;
    const GlobalPools& gp() const { return trie.getGlobalPools(); }
};

} // namespace barzer
