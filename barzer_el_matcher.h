#ifndef BARZER_EL_MATCHER_H
#define BARZER_EL_MATCHER_H

#include <barzer_el_chain.h>
#include <barzer_el_trie.h>
#include <barzer_parse_types.h>

/// Barzel matching algorithm is here 
/// it takes an array of CTokens , converts it to BarzelBeadChain 
/// and then using BELTrie matches subsequences in the bead chain to terminal pattern paths in 
/// the trie and rewrites the bead chain subsequences using BarzelTranslation objects at 
/// the end of the terminal path in the trie;
/// the rewriting stops once no new sequences have been matched
/// 

namespace barzer {
class StoredUniverse;

// reference information about the matched sequence 
// pair of iterators pointing to the first and the last element to be rewritten
typedef BarzelBeadChain::Range BeadRange;

typedef std::pair< const BarzelTrieNode*, BarzelBeadChain::Range > NodeAndBead;
typedef std::vector< NodeAndBead > NodeAndBeadVec;
typedef std::vector< const NodeAndBead* > NodeAndBeadPtrVec; 
/// every time a match is detected in a trie 
class BarzelMatchInfo {
	BeadRange d_beadRng; // full range of beads the match is done for
	int d_score; // 
	NodeAndBeadVec    d_thePath;
	NodeAndBeadPtrVec d_wcVec; // $n would get translated to wcVec[n] (points into d_thePath)
	/// uint32_t is the stringId from the pool
	std::map< uint32_t, size_t > d_varNameMap; 
public:
	BarzelMatchInfo() : d_score(0)  {}
	void setScore( int s ) 	{ d_score = s; }
	int getScore() const 	{ return d_score; }
	
	const BeadRange& getBeadRange( ) const { return  d_beadRng; }
	void setBeadRange( const BeadRange& br ) { d_beadRng= br; }

	const NodeAndBead* getDataByNumber(size_t n ) const 
		{ return ( n< d_wcVec.size() ? d_wcVec[n] : 0 ); }
	/// the trie node is needed in case there's a potential variable name 
	void setPath( const NodeAndBeadVec& p )
	{
		d_thePath = p;
		d_wcVec.clear();
		d_wcVec.reserve( d_thePath.size() );
		d_varNameMap.clear();
		for( NodeAndBeadVec::const_iterator i = d_thePath.begin(); i!= d_thePath.end(); ++i ) {
			if( i->first->isWcChild() ) 
				d_wcVec.push_back( &(*i) );
			/// if we want to add variable names we can have trie node hold var name 
			/// so right in this loop we will just update varNameMap 
		}
	}

	void clear() {
		d_thePath.clear();
		d_wcVec.clear();
		d_beadRng = BeadRange();
	}

	const NodeAndBead* getDataByNameId( uint32_t id ) const 
	{
		std::map< uint32_t, size_t >::const_iterator i = d_varNameMap.find( id );
		return( i == d_varNameMap.end() ? 0 : getDataByNumber( i->second ) );
	}
};
typedef std::pair<BarzelMatchInfo,const BarzelTranslation*> RewriteUnit;
//struct RewriteUnit;

/// Barzel TrieMatcher (BTM)
struct BTMBestPaths {
	// ghetto - technically evaluation may be required for right side backout
	// so the best path with right sides that guarantee no backout
	// such paths are called INFALLIBLE
	NodeAndBeadVec d_bestInfalliblePath;
	int d_bestInfallibleScore;
	

	// fallible paths (the ones with potential backout) potentially better than 
	// the best infallible
	// this will be implemented LATER (SHIT)
	typedef std::pair< NodeAndBeadVec, int > NABVScore;
	std::list<NABVScore> d_falliblePaths;

	/// information of the best terminal paths matched 
	/// 
	BeadRange d_fullRange; // original range of the top level matcher 

	BTMBestPaths( const BeadRange& r ) : 
		d_bestInfallibleScore(0),
		d_fullRange(r) {}
	const BeadRange& getFullRange() const { return d_fullRange; }
	void setFullRange(const BeadRange& br ) { d_fullRange = br; }
	
	/// bool indicates fallibility (true - infallible)
	/// int is the score in case path doesnt fail
	std::pair< bool, int > scorePath( const NodeAndBeadVec& nb ) const;

	void addPath( const NodeAndBeadVec& nb );
	/// returns the score of the winning match
	int setRewriteUnit( RewriteUnit& ru );

	int getBestScore() const
		{ return d_bestInfallibleScore; }
	const NodeAndBeadVec&  getBestPath() const { return  d_bestInfalliblePath ; }
	const BarzelTranslation* const getTranslation() { 
		const NodeAndBeadVec& p = getBestPath();
		return( p.size() ? &(p.rbegin()->first->translation) : 0 ); 
	}
};


// object used by the matcher
class BTMIterator {
public:
	typedef std::vector<const BarzelTrieNode*> BTNVec;
	NodeAndBeadVec d_matchPath;
private:
	/// current path - d_tn is one level below. 
	BTNVec d_path;
	/// pushes curPath and the accompanying bead range 
	int addTerminalPath( const NodeAndBead& nb );

	// puts all matching children of tn int mtChild
	// uses rng[0-...] for matching
	bool findMatchingChildren( NodeAndBeadVec& mtChild, const BeadRange& rng, const BarzelTrieNode* tn );
public:
	BTMBestPaths bestPaths;
private:
	// recursive function
	void matchBeadChain( const BeadRange& rng, const BarzelTrieNode* trieNode );

public:
	const StoredUniverse& universe;

	BTMIterator( const BeadRange& rng, const StoredUniverse& u ) : 
		bestPaths(rng),
		universe(u)
	{ }
	void findPaths( const BarzelTrieNode* trieNode)
		{ matchBeadChain( bestPaths.getFullRange(), trieNode ); }
	
	bool evalWildcard( const BarzelWCKey& wcKey, BeadList::iterator fromI, BeadList::iterator theI, uint8_t tokSkip ) const;
};


/// BarzelMatcher objects need to be unique per thread
class BarzelMatcher {
protected:
	const StoredUniverse& universe;

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
	int matchInRange( RewriteUnit& , const BeadRange& fullRange );


	virtual bool match( RewriteUnit&, BarzelBeadChain& );

	/// 
	int rewriteUnit( const RewriteUnit&, BarzelBeadChain& );	 	

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
	
	/// evaluates highest scoring paths and returns the first suitable one
	/// fills out rwrUnit
	//int findWinningPath( RewriteUnit& rwrUnit, BTMBestPaths& bestPaths )
	//{ return bestPaths.setRewriteUnit( rwrUnit ); }	

public:

	BarzelMatcher( const StoredUniverse& u ) : universe(u) {}
	virtual ~BarzelMatcher(  ) {}

	const Error& getError() const { return d_err; }

	/// initializes beadChain with ctokens from barz, runs the matching loop 
	/// and modifies the beads
	// returns the number of successful rewrites
	virtual int matchAndRewrite( Barz& );
	virtual void clear() 
	{
		d_err.clear();
	}

};

}
#endif // BARZER_EL_MATCHER_H
