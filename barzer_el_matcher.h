/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#pragma once
#include <barzer_el_chain.h>
#include <barzer_el_variable.h>
#include <barzer_el_trie.h>
/// Barzel matching algorithm is here 
/// it takes an array of CTokens , converts it to BarzelBeadChain 
/// and then using BELTrie matches subsequences in the bead chain to terminal pattern paths in 
/// the trie and rewrites the bead chain subsequences using BarzelTranslation objects at 
/// the end of the terminal path in the trie;
/// the rewriting stops once no new sequences have been matched
/// 

namespace barzer {
class StoredUniverse;
struct BarzelWCKey;
class BarzelTranslation;
class BarzelTrieNode;
class BELTrie;
struct BTND_Rewrite_Variable;

// reference information about the matched sequence 
// pair of iterators pointing to the first and the last element to be rewritten
typedef BarzelBeadChain::Range BeadRange;

/// NodeAndBead represent one node in the trie + the range of beads used in matching 
/// for this node
typedef std::pair< const BarzelTrieNode*, BarzelBeadChain::Range > NodeAndBead;
typedef std::vector< NodeAndBead > NodeAndBeadVec;
typedef std::vector< const NodeAndBead* > NodeAndBeadPtrVec; 
class BTMIterator;

struct MatcherCallback {
    virtual void operator()( const BTMIterator& nb ) = 0;
    virtual ~MatcherCallback() {}
};

template <typename CB>
struct MatcherCallbackGeneric : public MatcherCallback {
    CB& d_cb;
    MatcherCallbackGeneric( CB& cb ) : d_cb(cb) {}

    void operator()( const BTMIterator& btm ) { d_cb( btm ); }
};

/// state type (contained and used only by BarzelMatchInfo 
/// ambiguity
struct BarzelMatchAmbiguities {
    BarzerEntityList                          entList;
    std::vector< BarzelTranslationTraceInfo > traceInfoVec;
    bool                                      isAmbiguos;
    bool                                      d_makeUnmatchable;

    void makeUnmatchable() { if( !d_makeUnmatchable ) d_makeUnmatchable= true; }
    bool isUnmatchable() const { return d_makeUnmatchable; }

    const BarzerEntityList& getEntList() const { return entList; }
    BarzerEntityList& getEntList() { return entList; }
    BarzelMatchAmbiguities() : isAmbiguos(false),d_makeUnmatchable(false){}
    void clear()
    {
        isAmbiguos= false;
        d_makeUnmatchable= false;
        traceInfoVec.clear();
        entList.clear();
    }
    bool ambiguous() const { return isAmbiguos; }
    
    void addEntity( const BarzerEntity& e ) 
    {
        if( !isAmbiguos ) isAmbiguos= true;
        entList.addEntity(e);
    }

    void addEntity( const BarzerEntityList& e ) 
    {
        if( !isAmbiguos ) isAmbiguos= true;
        entList.appendEList(e.getList());
    }

    void addEntity( const BarzerEntityList::EList& e ) 
    {
        if( !isAmbiguos ) isAmbiguos= true;
        entList.appendEList(e);
    }

    void addEntity( const BELTrie& trie, const BarzelTranslation& tran );

    void addTrace( const BarzelTranslationTraceInfo& t ) { traceInfoVec.push_back(t); }
};

/// BarzelMatchInfo represents one left side of one substitution 
/// 
class BarzelMatchInfo {
	BeadRange d_beadRng; // full range of beads the match is done for

	// range of beads we will be actually substituting
	// substitution range is computed from the path in setPath
	BeadRange d_substitutionBeadRange; 
	int d_score; // 
	///  for each trie node stores the node + the range of beads that matched the node
	NodeAndBeadVec    d_thePath;
	std::vector< size_t > d_wcVec; // $n would get translated to wcVec[n] (stores offsets in thePath)
    
    /// careful! this is a state object 
    BarzelMatchAmbiguities d_ambiguities; 
    const Barz*            d_barz;
	// varid is the key to BarzelVariableIndex::d_pathInterner
	bool getDataByVarId( BeadRange& r, uint32_t varId, const BELTrie& trie ) const;

public:
    void setBarz( const Barz* b ) { d_barz=b; }
    const Barz* getBarz() const { return d_barz; }
    const BarzelBeadAtomic*  getClosestAtomicFromLeft( BarzelBeadAtomic_type_t typ, bool& found ) const;
    const BarzelMatchAmbiguities& ambiguities() const   { return d_ambiguities; }
          BarzelMatchAmbiguities& ambiguities()         { return d_ambiguities; }

    bool isAmbiguous() const { return d_ambiguities.ambiguous(); }

    const std::vector< BarzelTranslationTraceInfo >&    ambiguousTraceInfo() const { return d_ambiguities.traceInfoVec; }
    std::vector< BarzelTranslationTraceInfo >&          ambiguousTraceInfo() { return d_ambiguities.traceInfoVec; }

    const BELSingleVarPath* getVarPathByVarId( uint32_t varId, const BELTrie& trie ) const;

	bool isFullBeadRangeEmpty() const { return d_beadRng.first == d_beadRng.second ; }
	bool isSubstitutionBeadRangeEmpty() const
		{return d_substitutionBeadRange.first == d_substitutionBeadRange.second;}
	

	BarzelMatchInfo() : d_score(0), d_barz(0)  {}
	BarzelMatchInfo(const Barz* b ) : d_score(0), d_barz(b)  {}
	void setScore( int s ) 	{ d_score = s; }
	int getScore() const 	{ return d_score; }
	
	bool  iteratorIsEnd( BeadList::iterator i ) const
		{ return (i == d_beadRng.second); }
	void  expandRangeByOne( BeadRange& r ) const
	{
		if( r.second != d_beadRng.second ) ++r.second;
	}

    std::pair<int,int> getConfidenceRange() const 
    {
        std::pair<int,int> confBoost(0,0);
        bool gotSome = false;
        for( BeadList::const_iterator i = d_substitutionBeadRange.first; i!= d_substitutionBeadRange.second; ++i ) 
        {
            int tmpBoost = i->getConfidenceBoost();
            if( !gotSome ) {
                confBoost.first= confBoost.second = i->getConfidenceBoost();
                gotSome= true;
            } else if( tmpBoost< confBoost.first  ) {
                confBoost.first= tmpBoost;
            } else if( tmpBoost> confBoost.second ) {
                confBoost.second= tmpBoost;
            }
        }
        return confBoost;
    }
	const BeadRange& getSubstitutionBeadRange( ) const { return  d_substitutionBeadRange; }
	const BeadRange& getFullBeadRange( ) const { return  d_beadRng; }
	void setFullBeadRange( const BeadRange& br ) { d_beadRng= br; }

	const NodeAndBead* getDataByElementNumber(size_t n ) const 
		{ return ( n>0 && n<= d_thePath.size() ? &(d_thePath[n-1]) : 0 ); }

	typedef std::pair< NodeAndBeadVec::const_iterator, NodeAndBeadVec::const_iterator > PathRange;
	const NodeAndBead* getDataByWildcardNumber(size_t n ) const 
		{ 
			if( n>0 && n<= d_wcVec.size() ) {
				size_t i = d_wcVec[n-1];
				return ( i< d_thePath.size() ? &(d_thePath[i]) : 0 );
			} else
				return 0;
		}
	/// forms the range between the end of n-1th and n-th wildcards if n=1 then the range is
	/// between beginning of the match and the start of the wildcard
	bool getGapRange( BeadRange& r, size_t n ) const;
	bool getGapRange( BeadRange& r, const BTND_Rewrite_Variable& n ) const;

	/// the trie node is needed in case there's a potential variable name 
	void setPath( const NodeAndBeadVec& p );

	void clear() {
		d_thePath.clear();
		d_wcVec.clear();
        d_ambiguities.clear();
		d_beadRng = BeadRange();
	}

	inline const NodeAndBead* getDataByVar_trivial( const BTND_Rewrite_Variable& var ) const;

	// r will contain the bead range matching the contents of var
	// var can be pn - pattern number, w - wildcard umber and gn - gap number
	// gn[i] is everything between (exclusively) w[i-1] and w[i]
	// when resolution fails return false
	bool getDataByVar( BeadRange& r, const BTND_Rewrite_Variable& var, const BELTrie& trie );
	const BarzelTrieNode* getTheLeafNode() const
		{ return (d_thePath.size() ? d_thePath.back().first : 0 ); }
	uint32_t getTranslationId() const;
};
typedef std::pair<BarzelMatchInfo,const BarzelTranslation*> RewriteUnit;

/// Barzel TrieMatcher (BTM)
struct BTMBestPaths {
	// ghetto - technically evaluation may be required for right side backout
	// so the best path with right sides that guarantee no backout
	// such paths are called INFALLIBLE
    Barz&           d_barz;
    RewriteUnit&    d_rwrUnit;
	NodeAndBeadVec  d_bestInfalliblePath;
	int             d_bestInfallibleScore;
        
	// fallible paths (the ones with potential backout) potentially better than 
	// the best infallible
	// this will be implemented LATER (?) 
	typedef std::pair< NodeAndBeadVec, int > NABVScore;
	const BELTrie& d_trie;

	/// information of the best terminal paths matched 
	/// 
	BeadRange d_fullRange; // original range of the top level matcher 
	const StoredUniverse& universe;
    
	BTMBestPaths( Barz& barz,RewriteUnit&  ru, const BeadRange& r, const StoredUniverse& u, const BELTrie& trie ) : 
        d_barz(barz),
        d_rwrUnit(ru),
		d_bestInfallibleScore(0),
		d_trie(trie),
		d_fullRange(r),
		universe(u) 
	{}
    void clear() 
    {
        d_bestInfallibleScore= 0;
    }

	const BeadList::iterator getAbsoluteEnd() const { return d_fullRange.second; }
	const BeadRange& getFullRange() const { return d_fullRange; }
	void setFullRange(const BeadRange& br ) { d_fullRange = br; }
	
	/// bool indicates fallibility (true - infallible)
	/// int is the score in case path doesnt fail
	std::pair< bool, int > scorePath( const NodeAndBeadVec& nb ) const;

	void addPath( const NodeAndBeadVec& nb, const NodeAndBead& lastNB );
	/// returns the score of the winning match
	int setRewriteUnit( );

	int getBestScore() const
		{ return d_bestInfallibleScore; }
	const NodeAndBeadVec&  getBestPath() const { return  d_bestInfalliblePath ; }
	const BarzelTrieNode*  getTrieNode() const { 
		const NodeAndBeadVec& p = getBestPath();
		return( p.size() ? p.rbegin()->first : 0 ); 
	}

    bool isAmbiguous() const { return d_rwrUnit.first.isAmbiguous(); }
    void disambiguate() { return d_rwrUnit.first.ambiguities().clear(); }
    BarzelMatchAmbiguities& ambiguities() { return d_rwrUnit.first.ambiguities(); }
    const BarzelMatchAmbiguities& ambiguities() const { return d_rwrUnit.first.ambiguities(); }
private:
    const BarzelTranslation* getTranslation( const NodeAndBeadVec& nb ) const 
    {
        const BarzelTrieNode* node = getTrieNode();
        return ( node ? node->getTranslation(d_trie): 0 );
    }
    const BarzelTranslation* getTranslation( const NodeAndBead& nb ) const 
    {
        return( nb.first ? nb.first->getTranslation(d_trie) : 0 );
    }
};

struct QSemanticParser_AutocParms;
// object used by the matcher
class BTMIterator {
public:
	typedef std::vector<const BarzelTrieNode*> BTNVec;
	NodeAndBeadVec d_matchPath;
    enum {
        ITMODE_NORMAL, // default
        ITMODE_AUTOCOMPLETE,
    };
private:
	/// current path - d_tn is one level below. 
	BTNVec d_path;

	const BELTrie& d_trie;
    
    // one of the ITMODE_XXX values 
    int d_mode; 

	/// pushes curPath and the accompanying bead range 
	int addTerminalPath( const NodeAndBead& nb );
    const QSemanticParser_AutocParms* d_autocParm;
public:
	int addTerminalPath_Autocomplete( MatcherCallback& cb, const NodeAndBead& nb );
	int addTerminalPath_Autocomplete( MatcherCallback& cb );

	// puts all matching children of tn int mtChild
	// uses rng[0-...] for matching
	bool findMatchingChildren( NodeAndBeadVec& mtChild, const BeadRange& rng, const BarzelTrieNode* tn );
	BTMBestPaths bestPaths;
	// recursive function
	void matchBeadChain( const BeadRange& rng, const BarzelTrieNode* trieNode );
    
    /// returns the number of callbacks. 0 if match failed completely
	size_t matchBeadChain_Autocomplete( MatcherCallback& cb, const BeadRange& rng, const BarzelTrieNode* trieNode );

	const StoredUniverse& universe;
	const BELTrie&  getTrie() const { return d_trie; }
    bool mode_is_Autocomplete() const { return (d_mode==ITMODE_AUTOCOMPLETE); }
    bool mode_is_Normal() const { return (d_mode==ITMODE_NORMAL); }
    
    void mode_set_Normal() { d_mode= ITMODE_NORMAL; }
    const QSemanticParser_AutocParms* getAutocParm() const { return d_autocParm; }
    void mode_set_Autocomplete( const QSemanticParser_AutocParms* autocParm) { 
        d_mode= ITMODE_AUTOCOMPLETE; 
        if( autocParm ) 
            d_autocParm= autocParm;
    }

	BTMIterator( Barz& barz, RewriteUnit& ru, const BeadRange& rng, const StoredUniverse& u, const BELTrie& trie ) : 
		d_trie(trie),
        d_mode(ITMODE_NORMAL),
        d_autocParm(0),
		bestPaths(barz,ru,rng,u,trie),
		universe(u)
	{ }
    void autocParm_set( QSemanticParser_AutocParms* acp ) { d_autocParm= acp; }

	void findPaths( const BarzelTrieNode* trieNode)
		{ matchBeadChain( bestPaths.getFullRange(), trieNode ); }
	
	bool evalWildcard( const BarzelWCKey& wcKey, BeadList::iterator fromI, BeadList::iterator theI, uint8_t tokSkip ) const;
    const NodeAndBeadVec& getMatchPath() const { return d_matchPath; }
};



/// BarzelMatcher objects need to be unique per thread
class BarzelMatcher {
protected:
	const StoredUniverse& universe;
	const BELTrie& d_trie;

	/// Rewrite unit has BarzelTranslation (see barzer_el_trie.h)
	/// - which contains the actual rewriting instructions
	/// as well as the BarzelMatchInfo, which stores:
	///   - matching context (variable names and such) 
	///   - match path - determinitsic path in the trie that was matched for this unit
public:
	typedef std::pair< RewriteUnit, int> RangeWithScore;
	typedef std::vector< RangeWithScore > RangeWithScoreVec;
protected:
	/// returns the score of the match beginning wht the bead pointed 
	/// to by fullRange.first and ending before or at fullRange.second
	/// score of 0 means no match was found
	int matchInRange( Barz& barz, RewriteUnit& , const BeadRange& fullRange );


	virtual bool match( Barz& barz, RewriteUnit&, BarzelBeadChain& );

	int rewriteUnit( RewriteUnit&, Barz& );	 	

	class PathExclusionList {
		public:
			void clear() {}
			void addExclusion( const BarzelMatchInfo& ) {}
	};
	enum { MAX_REWRITE_COUNT = 1024*4, MAX_MATCH_COUNT = 1024*16 } ;
	//// error object
	struct Error {
		typedef enum { 
			ERR_OK, // no error
			ERR_CRCBRK_MATCH, // match count circuitbreaker hit (MAX_MATCH_COUNT)
			ERR_CRCBRK_REWRITE // rewrite count circuitbreaker hit (MAX_MATCH_COUNT)
		} Code_t;
		typedef enum {
			EF_RWRFAILS,
			EF_MAX
		} Flag_t;
		ay::bitflags<EF_MAX> failFlag;
		Code_t err;
	/// please only access the class using these methods ... unless you really have to
	/// go directly to the data members

		Error() : err(ERR_OK) {}
		void setFlag( Flag_t f ) { failFlag.set( f ); }
		void setCode( Code_t c ) { err = c; }

		void clear() { err = ERR_OK; failFlag.clear(); }
	};
	Error d_err;
public:

	BarzelMatcher( const StoredUniverse& u, const BELTrie& trie ) : universe(u), d_trie(trie) {}
	virtual ~BarzelMatcher(  ) {}

	const Error& getError() const { return d_err; }

	/// initializes beadChain with ctokens from barz, runs the matching loop 
	/// and modifies the beads
	// returns the number of successful rewrites
	virtual int matchAndRewrite( Barz& );

    int imperativeRewrite(Barz& barz, const BELTrieImperative& imperative );

    int runImperatives( Barz&, bool isPre );
	int runImperatives_Pre( Barz& b ) { return runImperatives(b,true); }
	int runImperatives_Post( Barz& b ) { return runImperatives(b,false); }

    size_t match_Autocomplete( MatcherCallback& cb, Barz&, const QSemanticParser_AutocParms& autocParm );

	virtual void clear() 
	{
		d_err.clear();
	}

	
    size_t get_match_greedy( MatcherCallback& cb, BeadList::iterator fromI, BeadList::iterator toI, bool precededByBlank ) const;

    // starting with each bea tries to match the longest path 
    // for each matched path invokes callback( const NodeAndBead& ) - NodeAndBead will contain a (matched range,node) pair
    template <typename CB>
    int pureMatchGreedy( CB& callback, const BeadRange& rng ) const
    {
        int numEmits = 0;
        bool precededByBlank = false;
        for(BeadList::iterator b= rng.first; b != rng.second; ++b ) {
            const BarzelBeadAtomic* bead;
            bead = b->getAtomic();
            if( bead ) {
                if( !bead->isBlankLiteral() ) {
                    MatcherCallbackGeneric<CB> cb(callback);

                    numEmits = get_match_greedy(cb, b, rng.second, precededByBlank);
                } else if( !precededByBlank ) 
                    precededByBlank = true;
            }
        }
        return numEmits;
    }
};

} // namespace barzer
