#include <barzer_el_chain.h>
#include <barzer_el_matcher.h>
#include <barzer_el_btnd.h>
#include <barzer_universe.h>
#include <barzer_barz.h>
#include <ay_logger.h>

namespace barzer {

int BTMIterator::addTerminalPath( const NodeAndBead& nb )
{
	ay::vector_raii<NodeAndBeadVec>( d_matchPath, nb );
	bestPaths.addPath( d_matchPath );
	return 0;
}

namespace {

struct BarzelWCLookupKey_form : public boost::static_visitor<> {
	BarzelWCLookupKey& key; // std::pair<BarzelTrieFirmChildKey, BarzelWCKey>
	const uint8_t& maxSpan;

	BarzelWCLookupKey_form( BarzelWCLookupKey& k, uint8_t skip ) :
		key(k), maxSpan(skip) {}

	template <typename T>
	void operator() ( const T& dta ) {
		key.first.setNull();
		key.second.set( maxSpan, true );
		key.second.noLeftBlanks = 0;
	}

};
template <> 
inline void BarzelWCLookupKey_form::operator()<BarzerLiteral> ( const BarzerLiteral& dta ) 
{
	key.first.set( dta, true );
	key.second.set( maxSpan, true );
}

struct findMatchingChildren_visitor : public boost::static_visitor<bool> {
	BTMIterator& 			d_btmi;
	NodeAndBeadVec& 		d_mtChild;
	BeadRange 				d_rng;
	// initialy d_rng.first this may change if the range starts with blanks (see skipBlanks function)
	BeadList::iterator      d_rangeStart; 
	const BarzelTrieNode* 	d_tn;

	bool d_followsBlank;
	
	findMatchingChildren_visitor( BTMIterator& bi, NodeAndBeadVec& mC, const BeadRange& r, const BarzelTrieNode* t ):
		d_btmi(bi), 
		d_mtChild(mC), 
		d_rng(r), 
		d_rangeStart(r.first), 
		d_tn(t), 
		d_followsBlank(false)
	{}
	
	/// partial key comparison . called from partialWCKeyProcess
	inline bool partialWCKeyProcess_key_eq( const BarzelWCLookupKey& l, const BarzelWCLookupKey& r ) const
	{
		return(
			l.first.id == l.first.id &&
			l.first.type == l.first.type 
		);
	}

	void tryWildcardMatch( BarzelWCLookup::const_iterator wci, BeadList::iterator iter, uint8_t tokSkip )
	{
		const BarzelTrieNode* ch = &(wci->second);
		const BarzelWCKey& wcKey = wci->first.second;

		/// do the matching 
		if( d_btmi.evalWildcard( wcKey, d_rangeStart, iter, tokSkip ) )
			d_mtChild.push_back( NodeAndBeadVec::value_type(ch,iter) );
	}
	/// iterates over all matching subordinate wildcards
	inline void partialWCKeyProcess( const BarzelWCLookup& wcLookup, const BarzelWCLookupKey& key, BeadList::iterator iter, uint8_t tokSkip )
	{

		if( key.first.isNull() ) {
			/// in this case we should try to consume max span of every wildcard
			#warning partialWCKeyProcess wc not followed by literal
		} else {
			BarzelWCLookup::const_iterator wci = wcLookup.lower_bound(key);

			for( ; wci!= wcLookup.end() && partialWCKeyProcess_key_eq( wci->first,key) ; ++wci ) {
				if( wci->first.second.maxSpan< tokSkip ) 
					break;
	
				/// if match is successful tryWildcardMatch will push into d_mtChild
				tryWildcardMatch( wci, iter, tokSkip );
			}
		}
	}

	void doWildcards( const BarzelWCLookup& wcLookup ) 
	{
		uint8_t tokSkip = 0;
		BeadList::iterator i = d_rng.first;
		for( ++i; i!= d_rng.second; ++i ) {
			BarzelWCLookupKey key;
			BarzelWCLookupKey_form key_form( key, tokSkip );
			const BarzelBeadAtomic* bead = i->getAtomic();
			if( bead )  {
				if( !bead->isBlankLiteral() ) 
					++tokSkip;
				boost::apply_visitor( key_form, bead->dta );
			} else {
				BarzelBeadData blankBead;
				boost::apply_visitor( key_form, blankBead );
				++tokSkip;
			}	

			partialWCKeyProcess( wcLookup, key, i,tokSkip );
			if( !key.first.isNull() ) {
				BarzelWCLookupKey nullkey;
				BarzelWCLookupKey_form nullKey_form ( nullkey, tokSkip );
				BarzelBeadData blankBead;
				boost::apply_visitor( nullKey_form, blankBead );

				partialWCKeyProcess( wcLookup, nullkey,i,tokSkip );
			}
		}
	}
	void skipBlanks() 
	{
		for( ;d_rng.first!= d_rng.second; ++d_rng.first ) {
			const BarzelBead& b = *(d_rng.first);
			const BarzelBeadAtomic* bead = b.getAtomic();
			if( bead && bead->isBlankLiteral() ) {
				if( !d_followsBlank ) 
					d_followsBlank = true;
			} else 
				break;
		}
	}
	bool operator()( const BarzerLiteral& dta ) 
	{
		BarzelTrieFirmChildKey firmKey; 
		// forming firm key
		bool curDtaIsBlank = firmKey.set(dta,d_followsBlank).isBlankLiteral();
		if( curDtaIsBlank ) 
			return false; // this should never happen (skipBlanks is called prior)

		const BarzelTrieNode* ch = d_tn->getFirmChild( firmKey ); 
		if( ch ) {
			d_mtChild.push_back( NodeAndBeadVec::value_type(ch,d_rng.first) );
		} 
		/// retrying in case 
		if( !d_followsBlank ) {
			firmKey.noLeftBlanks = 0;
			ch = d_tn->getFirmChild( firmKey ); 
			if( ch )
				d_mtChild.push_back( NodeAndBeadVec::value_type(ch,d_rng.first) );
		}

		if( !curDtaIsBlank && d_followsBlank ) 
			d_followsBlank = false;

		// ch is 0 here 

		if( d_tn->hasValidWcLookup() ) 
			return false;

		const BarzelWCLookup* wcLookup = d_btmi.universe.getWCLookup( d_tn->getWCLookupId() );
		if( !wcLookup ) { // this should never happen
			AYLOG(ERROR) << "null lookup" << std::endl;
		} else
			doWildcards( *wcLookup );	
		return (d_mtChild.size() > 0);
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

};

//// TEMPLATE MATCHING
template <typename P>
struct evalWildcard_vis : public boost::static_visitor<bool> {
	const P& d_pattern;

	evalWildcard_vis( const P& p ) : d_pattern(p) {}
	template <typename T>
	bool operator()( const T& ) const {
		/// for all types except for the specialized wildcard wont match
		return false;
	}
};
/// number templates 
template <>  template<>
inline bool evalWildcard_vis<BTND_Pattern_Number>::operator()<BarzerLiteral> ( const BarzerLiteral& dta ) const
{
	/// some literals may be "also numbers" 
	/// special handling for that needs to be added here 
	return false;
}
template <> template <>
inline bool evalWildcard_vis<BTND_Pattern_Number>::operator()<BarzerNumber> ( const BarzerNumber& dta ) const
{ return d_pattern.checkNumber( dta ); }
//// end of number template matching
//// other patterns 
/// for now these patterns always match on a right data type (date matches any date, datetime any date time etc..)
/// they can be certainly further refined
template <> template <>
inline bool evalWildcard_vis<BTND_Pattern_Date>::operator()<BarzerDate> ( const BarzerDate& dta ) const
{ return true; }

template <> template <>
inline bool evalWildcard_vis<BTND_Pattern_DateTime>::operator()<BarzerDateTime> ( const BarzerDateTime& dta )  const
{ return true; }

template <> template <>
inline bool evalWildcard_vis<BTND_Pattern_Time>::operator()<BarzerTimeOfDay> ( const BarzerTimeOfDay& dta )  const
{ return true; }

template <> template <>
inline bool evalWildcard_vis<BTND_Pattern_Wildcard>::operator()<BarzerLiteral> ( const BarzerLiteral& dta )  const
{ return true; }
//// end of other template matching

}

bool BTMIterator::evalWildcard( const BarzelWCKey& wcKey, BeadList::iterator fromI, BeadList::iterator theI, uint8_t tokSkip ) const
{
	const BarzelWildcardPool& wcPool = universe.getWildcardPool();
	const BarzelBeadAtomic* atomic = theI->getAtomic();
	if( !atomic ) 
		return false;
	
	switch( wcKey.wcType ) {
	case BTND_Pattern_Number_TYPE:
	{
		const BTND_Pattern_Number* p = wcPool.get_BTND_Pattern_Number(wcKey.wcId);
		return( p ?  boost::apply_visitor( evalWildcard_vis<BTND_Pattern_Number>(*p), atomic->dta ) : false );
	}
	case BTND_Pattern_Wildcard_TYPE:
	{
		const BTND_Pattern_Wildcard* p = wcPool.get_BTND_Pattern_Wildcard(wcKey.wcId);
		return( p ?  boost::apply_visitor( evalWildcard_vis<BTND_Pattern_Wildcard>(*p), atomic->dta ) : false );
	}
	case BTND_Pattern_Date_TYPE:
	{
		const BTND_Pattern_Date* p = wcPool.get_BTND_Pattern_Date(wcKey.wcId);
		return( p ?  boost::apply_visitor( evalWildcard_vis<BTND_Pattern_Date>(*p), atomic->dta ) : false );
	}
	case BTND_Pattern_Time_TYPE:
	{
		const BTND_Pattern_Time* p = wcPool.get_BTND_Pattern_Time(wcKey.wcId);
		return( p ?  boost::apply_visitor( evalWildcard_vis<BTND_Pattern_Time>(*p), atomic->dta ) : false );
	}
	case BTND_Pattern_DateTime_TYPE:
	{
		const BTND_Pattern_DateTime* p = wcPool.get_BTND_Pattern_DateTime(wcKey.wcId);
		return( p ?  boost::apply_visitor( evalWildcard_vis<BTND_Pattern_DateTime>(*p), atomic->dta ) : false );
	}
	default:
		return false;
	}
	return false;
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

	vis.skipBlanks();
	boost::apply_visitor( vis, bead->dta );
	return mtChild.size();
}

void BTMIterator::matchBeadChain( const BeadRange& rng, const BarzelTrieNode* trieNode )
{
	ay::vector_raii<NodeAndBeadVec>( d_matchPath, NodeAndBeadVec::value_type( trieNode, rng.first ) );

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

void BTMBestPaths::addPath( const NodeAndBeadVec& nb )
{
	// ghetto
	if( nb.size() > d_bestHardPath.size() )
		d_bestHardPath = nb;
}

int BarzelMatcher::matchInRange( RewriteUnit& rwrUnit, const BeadRange& curBeadRange ) 
{
	int  score = 0;
	const BarzelTrieNode* trieRoot = &(universe.getBarzelTrie().getRoot());

	BTMIterator btmi(curBeadRange,universe);	
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
