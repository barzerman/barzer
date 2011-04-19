#include <barzer_el_chain.h>
#include <barzer_el_matcher.h>
#include <barzer_el_btnd.h>
#include <barzer_universe.h>
#include <barzer_barz.h>
#include <ay_logger.h>

namespace barzer {

int BTMIterator::addTerminalPath( const NodeAndBead& nb )
{
	#warning BTMIterator::addTerminalPath
	return 0;
}

namespace {

struct BarzelWCLookupKey_form : public boost::static_visitor<> {
	BarzelWCLookupKey& key; // std::pair<BarzelTrieFirmChildKey, BarzelWCKey>
	const uint8_t& maxSpan;
	const bool& followsBlank;

	BarzelWCLookupKey_form( BarzelWCLookupKey& k, uint8_t skip, bool fb ) :
		key(k), maxSpan(skip), followsBlank(fb) {}

	template <typename T>
	void operator() ( const T& dta ) {
		key.first.setNull();
		key.second.set( maxSpan, followsBlank );
	}

};
template <> 
inline void BarzelWCLookupKey_form::operator()<BarzerLiteral> ( const BarzerLiteral& dta ) 
{
	key.first.set( dta, false );
	key.second.set( maxSpan, followsBlank );
}

struct findMatchingChildren_visitor : public boost::static_visitor<bool> {
	BTMIterator& btmi;
	NodeAndBeadVec& mtChild;
	BeadRange rng;
	const BarzelTrieNode* tn;

	bool followsBlank;
	bool tryMore;
	
	findMatchingChildren_visitor( 
		BTMIterator& bi, NodeAndBeadVec& mC, const BeadRange& r, const BarzelTrieNode* t ):
		btmi(bi), mtChild(mC), rng(r), tn(t), followsBlank(false), tryMore(true)
	{}
	
	bool partialWCKeyProcess( const BarzelWCLookupKey& key, BeadList::iterator iter, uint8_t tokSkip )
	{
		//  for each child matching partial wcKey
		//     if( wildcardMatch( child, rng.first, i, skip ) ) 
		// 		   mtChild.push_back( child, i )
		//  end foreach
		return true;
	}

	bool operator()( const BarzerLiteral& dta ) 
	{
		BarzelTrieFirmChildKey key; 

		// forming firm key
		if( key.set(dta,followsBlank).isBlankLiteral() ) {
			++rng.first;
			return ( followsBlank = tryMore= true );
		} else if( followsBlank ) 
			followsBlank = false;

		const BarzelTrieNode* ch = tn->getFirmChild( key ); 
		if( ch ) {
			mtChild.push_back( NodeAndBeadVec::value_type(ch,rng.first) );
		}
		// ch is 0 here 

		if( tn->hasValidWcLookup() ) 
			return false;

		const BarzelWCLookup* wcLookup = btmi.universe.getWCLookup( tn->getWCLookupId() );
		if( !wcLookup ) { // this should never happen
			AYLOG(ERROR) << "null lookup" << std::endl;
		}
		
		uint8_t tokSkip = 1;
		BeadList::iterator i = rng.first;
		for( ++i; i!= rng.second; ++i ) {
			BarzelWCLookupKey key;
			BarzelWCLookupKey_form key_form( key, tokSkip, followsBlank );
			const BarzelBeadAtomic* bead = i->getAtomic();
			if( bead )  {
				boost::apply_visitor( key_form, bead->dta );
			} else {
				BarzelBeadData blankBead;
				boost::apply_visitor( key_form, blankBead );
			}	
			partialWCKeyProcess( key, i,tokSkip );
			if( !key.first.isNull() ) {
				BarzelWCLookupKey nullkey;
				BarzelWCLookupKey_form nullKey_form ( nullkey, tokSkip, followsBlank );
				BarzelBeadData blankBead;
				boost::apply_visitor( nullKey_form, blankBead );

				partialWCKeyProcess( nullkey,i,tokSkip );
			}
			// key is formed here
			++tokSkip;
		}
		return false;
	}

	bool operator()( const BarzerString& dta ) 
	{
		return false;
	}
	bool operator()( const BarzerNumber& dta ) 
	{
		return false;
	}
	bool operator()( const BarzerDate& dta ) 
	{
		return false;
	}
	bool operator()( const BarzerTimeOfDay& dta ) {
		return false;
	}
	bool operator()( const BarzerRange& dta ) {
		return false;
	}
	bool operator()( const BarzerEntityList& dta ) {
		return false;
	}
	bool operator()( const BarzelEntityRangeCombo& dta )
	{
		return false;
	}

	inline bool keepTrying() const 
	{
		if( rng.first == rng.second ) 
			return false;
		if( tryMore ) 
			return true;	
		
		return false;
	}
};

}

bool BTMIterator::findMatchingChildren( NodeAndBeadVec& mtChild, const BeadRange& rng, const BarzelTrieNode* tn )
{
	if( rng.first == rng.second ) 
		return false; // this should never happen bu just in case

	const BarzelBead& b = *(rng.first);
	
	const BarzelBeadAtomic* bead = b.getAtomic();
	if( !bead )  /// expressions and blanks wont be matched 
		return false; 
	
	findMatchingChildren_visitor vis( *this, mtChild, rng, tn );

	size_t numMatched = 0;
	while( vis.keepTrying() ) {
		if( boost::apply_visitor( vis, bead->dta ) )
			++numMatched; 
	}	
	return mtChild.size();
}

void BTMIterator::matchBeadChain( const BeadRange& rng, const BarzelTrieNode* trieNode )
{
	if( rng.first == rng.second ) {
		return; // range is empty
	}
	NodeAndBeadVec mtChild;

	if( !findMatchingChildren( mtChild, rng, trieNode ) ) 
		return; // no matching children found

	for( NodeAndBeadVec::const_iterator ch = mtChild.begin(); ch != mtChild.end(); ++ ch ) {
		const BarzelTrieNode* tn = ch->first;
		BeadList::iterator nextBead = ch->second;

		if( tn->isLeaf() ) {
			addTerminalPath( *ch );
		}

		if( nextBead != rng.second ) {  // recursion
			matchBeadChain( BeadRange(nextBead,rng.second), tn );
		}
	}
}

int BarzelMatcher::matchInRange( RewriteUnit& rwrUnit, const BeadRange& fullRange ) 
{
	int  score = 0;
	const BarzelTrieNode* trieRoot = &(universe.getBarzelTrie().getRoot());

	BTMIterator btmi(fullRange,universe);	
	btmi.findPaths(trieRoot);
	
	score = findWinningPath( rwrUnit, btmi.bestPaths );

	return score;
}

int BarzelMatcher::findWinningPath( RewriteUnit& rwrUnit, BTMBestPaths& bestPaths )
{
	#warning BarzelMatcher::findWinningPath unimplemented 
	return 0;
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

/// 
int BarzelMatcher::rewriteUnit( const RewriteUnit&, BarzelBeadChain& )
{
	#warning BarzelMatcher::rewriteUnit
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
	}
	return rewrCount;
}

} // namespace barzer ends 
