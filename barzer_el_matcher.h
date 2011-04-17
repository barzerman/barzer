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
	// reference information about the matched sequence 
	// pair of iterators pointing to the first and the last element to be rewritten
	typedef BarzelBeadChain::Range BeadRange;
	BeadRange beadRng;
	
	void clear() {
		ctxt.clear();
		matchPath.clear();
		beadRng = BeadRange();
	}
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
	typedef std::pair<BarzelMatchInfo,const BarzelTranslation*> RewriteUnit;

	virtual bool match( RewriteUnit&, BarzelBeadChain& );

	/// evaluation methods are called within match 
	/// returns true if this unit is rewritable. if one unit in a set is not rewritable
	/// mostly this will be useful for entity search patterns 
	bool evaluateRewriteUnit( const RewriteUnit&, const BarzelBeadChain& ) const;

	/// 
	int rewriteUnit( const RewriteUnit&, BarzelBeadChain& );	 	

	class PathExclusionList {
		public:
			void clear() {}
			void addExclusion( const BarzelMatchInfo& ) {}
	};
	PathExclusionList d_exclList;
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

	BarzelMatcher( const StoredUniverse& u ) : universe(u) {}
	virtual ~BarzelMatcher(  ) {}

	const Error& getError() const { return d_err; }

	/// initializes beadChain with ctokens from barz, runs the matching loop 
	/// and modifies the beads
	// returns the number of successful rewrites
	virtual int matchAndRewrite( Barz& );
	virtual void clear() 
	{
		d_exclList.clear();
		d_err.clear();
	}

};

}
#endif // BARZER_EL_MATCHER_H
