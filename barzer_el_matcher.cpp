#include <barzer_el_chain.h>
#include <barzer_el_matcher.h>
#include <barzer_el_btnd.h>
#include <barzer_universe.h>
#include <barzer_barz.h>
#include <ay_logger.h>

namespace barzer {

int BarzelMatcher::matchInRange( RewriteUnit& rwrUnit, const BeadRange& fullRange ) 
{
	int  score = 0;
	const BarzelTrieNode* trieNode = &(universe.getBarzelTrie().getRoot());

	for( BeadList::iterator i = fullRange.first; i!=  fullRange.second; ++i ) {
		const BarzelBead & bead = *i;

		if( bead.isBlank() ) {
			AYLOG(WARNING) << trieNode << std::endl;
		}
		/// matching this bead to the children (trieNode)
		/// if matched 
		/// -- update trieNode
		/// -- push  trieNode into the matched path
		/// --- newTrieNode->isLeaf - record leaf path (maybe it will be naturaly there)
		/// else (not matched and we abort) 
		/// back out to the previous leaf path 

		// alternative: 
		/// take the whole sub tree and match wildcards forward , keeping track of the longest path  
	}
	return score;
}

namespace {
	inline BarzelMatcher::RangeWithScoreVec::iterator findBestRangeByScore( 
		BarzelMatcher::RangeWithScoreVec::iterator beg,
		BarzelMatcher::RangeWithScoreVec::iterator end 
	)
	{
		BarzelMatcher::RangeWithScoreVec::iterator result = end;
		int score = 0;
		for( BarzelMatcher::RangeWithScoreVec::iterator i = beg; i!= end; ++i ) {
			/// this is a left-biased system and the vector is ordered 
			/// by closeness to the left edge
			if( i->second > score ) {
				result = i;
				score = i->second;
			}
		}
		return result;
	}

}

bool BarzelMatcher::match( RewriteUnit& ru, BarzelBeadChain& beadChain )
{
	// trie always has at least one node (root)


	RangeWithScoreVec rwScoreVec;
	for( BeadList::iterator i = beadChain.getLstBegin(); beadChain.isIterNotEnd(i); ++i ) {
		/// full range - all beads starting from i
		BeadRange fullRange( i, beadChain.getLstEnd() );

		RewriteUnit rwrUnit;
		int score = matchInRange( rwrUnit, fullRange );

		if( score ) {
			rwScoreVec.push_back( 
				RangeWithScoreVec::value_type(
					rwrUnit, score
				)
			);

		}
	}
	if( !rwScoreVec.size() ) 
		return false;

	RangeWithScoreVec::iterator bestI = findBestRangeByScore( rwScoreVec.begin(), rwScoreVec.end() );
	if( bestI != rwScoreVec.end() ) {
		ru = bestI->first;
		/// it's better to keep the scores separte in case we decide to massage the scores later
		ru.first.setScore( bestI->second );
		return true;
	} else { // should never happen 
		AYLOG(ERROR) << "no valid rewrite unit for a valid match" << std::endl;
		return false;
	}
}

bool BarzelMatcher::evaluateRewriteUnit( const RewriteUnit&, const BarzelBeadChain& ) const
{
	// #warning BarzelMatcher::evaluateRewriteUnit
	return true;
}

/// 
int BarzelMatcher::rewriteUnit( const RewriteUnit&, BarzelBeadChain& )
{
	#warning BarzelMatcher::evaluateRewriteUnit
	return 0;
}
int BarzelMatcher::matchAndRewrite( Barz& barz )
{
	AYTRACE( "BarzelMatcher::matchAndRewrite unimplemented" );
	clear();

	BarzelBeadChain beads = barz.getBeads();

	int rewrCount = 0, matchCount = 0;


	for( ; ; ++ matchCount ) {
		if( matchCount> MAX_MATCH_COUNT ) {
			d_err.setCode( Error::ERR_CRCBRK_MATCH );
			AYLOG(ERROR) << "Match count circuit breaker hit .." << std::endl;
			break;
		}

		RewriteUnit rewrUnit;
		if( !match( rewrUnit, beads) ) {
			break; // nothing matched
		}
		/// here we can add some debugging where we would log rewrUnit.first (matching info)

		if( evaluateRewriteUnit( rewrUnit, beads ) ) {
			if( rewrCount > MAX_REWRITE_COUNT ) {
				d_err.setCode( Error::ERR_CRCBRK_MATCH );
				AYLOG(ERROR) << "Rewrite count circuit breaker hit .." << std::endl;
				break;
			}
			int rewrRc = rewriteUnit( rewrUnit, beads );
			if( rewrRc ) { // should never be the case
				d_err.setFlag( Error::EF_RWRFAILS );
				AYLOG(ERROR) << "rewrite failed"  << std::endl;
			} else
				++rewrCount;
		} else 
			d_exclList.addExclusion( rewrUnit.first );
		
	}
	return rewrCount;
}

} // namespace barzer ends 
