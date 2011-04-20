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

/// every time a match is detected in a trie 
struct BarzelMatchInfo {
	/// matching context - variable names (if named variables)
	/// or map of matched wildcard ids
	struct Context {
		Context() {}
		void clear() {}
	} ctxt;
	/// represents a deterministic pattern path in the trie resulting in a specific translation
	struct TrieMatchPath {
		TrieMatchPath() {}
		void clear() {}
		std::ostream& print( std::ostream& fp ) const {
			return fp;
		}
	} matchPath;


	BeadRange beadRng;
	int score; // 
	
	void clear() {
		ctxt.clear();
		matchPath.clear();
		beadRng = BeadRange();
	}
	BarzelMatchInfo() : score(0)  {}
	void setScore( int s ) { score = s; }
};
/// Barzel TrieMatcher (BTM)
struct BTMBestPaths {
	/// information of the best terminal paths matched 
	/// 
	BeadRange fullRange; // original range of the top level matcher 
	BTMBestPaths( const BeadRange& r ) : fullRange(r) {}
	
};

typedef std::pair< const BarzelTrieNode*, BeadList::iterator > NodeAndBead;
typedef std::vector< NodeAndBead > NodeAndBeadVec;

// object used by the matcher
class BTMIterator {
public:
	typedef std::vector<const BarzelTrieNode*> BTNVec;

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
		{ matchBeadChain( bestPaths.fullRange, trieNode ); }
	
	bool evalWildcard( wcKey, BeadList::iterator fromI, BeadList::iterator theI, uint8_t tokSkip ) const;
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
	typedef std::pair<BarzelMatchInfo,const BarzelTranslation*> RewriteUnit;
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
	int findWinningPath( RewriteUnit& rwrUnit, BTMBestPaths& bestPaths );

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
