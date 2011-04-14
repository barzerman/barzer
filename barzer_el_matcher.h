#ifndef BARZER_EL_MATCHER_H
#define BARZER_EL_MATCHER_H

#include <barzer_el_chain.h>
#include <barzer_parse_types.h>
#include <barzer_universe.h>

/// Barzel matching algorithm is here 
/// it takes an array of CTokens , converts it to BarzelBeadChain 
/// and then using BELTrie matches subsequences in the bead chain to terminal pattern paths in 
/// the trie and rewrites the bead chain subsequences using BarzelTranslation objects at 
/// the end of the terminal path in the trie;
/// the rewriting stops once no new sequences have been matched
/// 

namespace barzer {

/// there is one BarzelMatcher per thread
class BarzelMatcher {
	const StoredUniverse& universe;

	BarzelBeadChain beadChain;	
public:
	BarzelMatcher( const StoredUniverse& u ) : universe(u) {}

	/// initializes beadChain with ctokens from barz, runs the matching loop 
	/// and then pushes out PUnits 
	int matchAndRewrite( Barz& );
};

}
#endif // BARZER_EL_MATCHER_H
