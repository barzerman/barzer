#include <barzer_el_chain.h>
#include <barzer_el_matcher.h>
#include <barzer_el_btnd.h>
#include <barzer_universe.h>
#include <barzer_barz.h>
#include <ay_logger.h>

namespace barzer {

int BTMIterator::addTerminalPath( const NodeAndBead& nb )
{
	ay::vector_raii<NodeAndBeadVec> raii( d_matchPath, nb );

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

struct MatchChildInfo {
};

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
		if( d_btmi.evalWildcard( wcKey, d_rangeStart, iter, tokSkip ) ) {
			BarzelBeadChain::Range goodRange(d_rng.first,iter);
			d_mtChild.push_back( NodeAndBeadVec::value_type(ch,goodRange) );
		}
	}
	/// iterates over all matching subordinate wildcards
	inline void partialWCKeyProcess( const BarzelWCLookup& wcLookup, const BarzelWCLookupKey& key, BeadList::iterator iter, uint8_t tokSkip )
	{

		BarzelWCLookupKey nullKey;
		BarzelWCLookup::const_iterator wci = ( wcLookup.lower_bound(nullKey));
		bool wcTermPathNotFound = (wci == wcLookup.end()); 
		if( wcTermPathNotFound ) {
			/// this means here are no wc terminated paths (a *) and 
			/// we got null key on input which means we had exhausted 
			/// all potential candidate paths and there's nothing left to do 
			if( key.first.isNull() )
				return;
			BarzelWCLookup::const_iterator wci = ( wcLookup.lower_bound(key));
		} else {
			/// otherwise we will iterate over everything
			wci = wcLookup.begin();
		} 
			
		/// this here means that there are wc terminated paths (a*) 
		/// which, in turn means we need to match every wildcard in the subtree
		/// in order for the algo to be correct
		for( ; wci!= wcLookup.end() && 
			( wcTermPathNotFound || partialWCKeyProcess_key_eq( wci->first,key) ); 
			++wci 
		) {
			if( wci->first.second.maxSpan>= tokSkip ) {
				/// if match is successful tryWildcardMatch will push into d_mtChild
				tryWildcardMatch( wci, iter, tokSkip );
			}
		}
	}

	inline void doWildcards( )
	{
		if( !d_tn->hasValidWcLookup() ) 
			return;

		const BarzelWCLookup* wcLookup_ptr = d_btmi.universe.getWCLookup( d_tn->getWCLookupId() );
		if( !wcLookup_ptr ) { // this should never happen
			AYLOG(ERROR) << "null lookup" << std::endl;
			return;
		} 
		const BarzelWCLookup& wcLookup  = *wcLookup_ptr;

		uint8_t tokSkip = 0;
		BeadList::iterator i = d_rng.first;
		for( ; i!= d_rng.second; ++i ) {
			BarzelWCLookupKey key;
			const BarzelBeadAtomic* bead = i->getAtomic();
			BarzelWCLookupKey_form key_form( key, tokSkip );
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

	template <typename T>
	bool operator()( const T& dta ) 
	{
		doWildcards( );
		return (d_mtChild.size() > 0);
	}

};
	template <>
inline	bool findMatchingChildren_visitor::operator()<BarzerLiteral> ( const BarzerLiteral& dta ) 
	{
		BarzelTrieFirmChildKey firmKey; 
		// forming firm key
		bool curDtaIsBlank = firmKey.set(dta,d_followsBlank).isBlankLiteral();
		if( curDtaIsBlank ) 
			return false; // this should never happen (skipBlanks is called prior)

		const BarzelTrieNode* ch = d_tn->getFirmChild( firmKey ); 
		if( ch ) {
			BeadList::iterator endIt = d_rng.first;
			++endIt;
			d_mtChild.push_back( NodeAndBeadVec::value_type(ch,BarzelBeadChain::Range(d_rng.first,endIt)) );
		} 
		/// retrying in case the token immediately follows non blank 
		if( !d_followsBlank ) {
			firmKey.noLeftBlanks = 0;
			ch = d_tn->getFirmChild( firmKey ); 
			if( ch ) {
				BeadList::iterator endIt = d_rng.first;
				++endIt;
				d_mtChild.push_back( NodeAndBeadVec::value_type(ch, BarzelBeadChain::Range(d_rng.first,endIt)));
			}
		}

		if( !curDtaIsBlank && d_followsBlank ) 
			d_followsBlank = false;
		// ch is 0 here 
		doWildcards( );
		return (d_mtChild.size() > 0);
	}

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
	ay::vector_raii<NodeAndBeadVec> raii( d_matchPath, NodeAndBeadVec::value_type( trieNode, BarzelBeadChain::Range(rng.first,rng.first)) );

	if( rng.first == rng.second ) {
		return; // range is empty
	}
	NodeAndBeadVec mtChild;

	if( !findMatchingChildren( mtChild, rng, trieNode ) ) 
		return; // no matching children found

	const BarzelTrieNode* tn = 0;
	BeadList::iterator nextBead = rng.second;

	for( NodeAndBeadVec::const_iterator ch = mtChild.begin(); ch != mtChild.end(); ++ ch ) {
		if(tn && ch->first == tn && ch->second.second == nextBead) 
			continue;
		
		tn = ch->first;
		// nextBead is set to the second iterator in the range
		// remember ranges stored in d_mtChain are inclusive, that is 
		// if only one bead was matched iterator to it is stored in both 
		// first and second of the range
		nextBead = ch->second.second;

		if( tn->isLeaf() ) {
			addTerminalPath( *ch );
		}

		if( nextBead != rng.second ) {  // recursion
			BeadList::iterator beadPastMatch = nextBead;
			// advancing to the next bead and starting new recursion from there			
			matchBeadChain( BeadRange(++beadPastMatch,rng.second), tn );
		}
	}
}


int BTMBestPaths::setRewriteUnit( RewriteUnit& ru )
{
	BarzelMatchInfo& ruMatchInfo = ru.first;
	ruMatchInfo.setScore( getBestScore() );
	ruMatchInfo.setFullBeadRange( getFullRange() );
	const NodeAndBeadVec& winningPath = getBestPath();
	ruMatchInfo.setPath( winningPath );
	ru.second = getTranslation();

	return 0;
}
/// bool indicates fallibility 
/// int is the score in case path doesnt fail
std::pair< bool, int > BTMBestPaths::scorePath( const NodeAndBeadVec& nb ) const
{
	if( !nb.size() ) 
		return( std::pair< bool, int >(true,0) );

	bool isTranslationFallible = universe.isBarzelTranslationFallible( nb.back().first->translation );

	std::pair< bool, int >  retVal( isTranslationFallible, 0 );
	
	BeadList::iterator fromBead = d_fullRange.first;
	BeadList::iterator endBead = nb.rbegin()->second.second;

	if( endBead != d_fullRange.second )
		++endBead; // need to look past the end of the last bead in range

	int numTokensConsumed = 0; // number of *tokens* in the match  
	for( BeadList::iterator i = fromBead; i!= endBead; ++i ) {
		numTokensConsumed += i->getFullNumTokens();
	}

	int matchStyleScore = 0; // wc match gets 10 firm - 100
	/// backtraces nodes up to root
	for( const BarzelTrieNode* tn = nb.rbegin()->first; tn && tn->getParent(); tn= tn->getParent() ) {
		matchStyleScore += ( tn->isWcChild() ? 10 : 100 );	
	}
	retVal.second = numTokensConsumed * matchStyleScore;	

	return retVal;
}

void BTMBestPaths::addPath(const NodeAndBeadVec& nb )
{
	// ghetto
	std::pair< bool, int > score = scorePath( nb );	
	if( score.second <= d_bestInfallibleScore ) 
		return;
	d_bestInfallibleScore = score.second;
	if( score.first ) { /// fallible 
		d_falliblePaths.push_back( NABVScore(nb,score.second) );
	} else { 
		d_bestInfalliblePath = nb;
	}
}


void BarzelMatchInfo::setPath( const NodeAndBeadVec& p )
{
	d_thePath = p;
	d_wcVec.clear();
	d_wcVec.reserve( d_thePath.size() );
	d_varNameMap.clear();
	d_substitutionBeadRange.first = d_substitutionBeadRange.second = d_beadRng.second;
	for( NodeAndBeadVec::const_iterator i = d_thePath.begin(); i!= d_thePath.end(); ++i ) {
		
		if( i->first->isWcChild() ) 
			d_wcVec.push_back( &(*i) );
		/// if we want to add variable names we can have trie node hold var name 
		/// so right in this loop we will just update varNameMap 
	}
	if( d_thePath.size() ) {
		d_substitutionBeadRange.first = d_thePath.front().second.first;
		d_substitutionBeadRange.second = d_thePath.back().second.second;
		if( d_substitutionBeadRange.second != d_beadRng.second ) 
			++d_substitutionBeadRange.second;
	}
}
int BarzelMatcher::matchInRange( RewriteUnit& rwrUnit, const BeadRange& curBeadRange ) 
{
	int  score = 0;
	const BarzelTrieNode* trieRoot = &(universe.getBarzelTrie().getRoot());

	BTMIterator btmi(curBeadRange,universe);	
	btmi.findPaths(trieRoot);
	
	btmi.bestPaths.setRewriteUnit( rwrUnit );
	// findWinningPath( rwrUnit, btmi.bestPaths );
	score = rwrUnit.first.getScore();

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

/// 
int BarzelMatcher::rewriteUnit( RewriteUnit& ru, BarzelBeadChain& chain )
{
	BarzelMatchInfo& matchInfo = ru.first;
	
	const BarzelTranslation* transP = ru.second;
	if( matchInfo.isSubstitutionBeadRangeEmpty() ) { // this should never happen 
		AYLOG(ERROR) << "empty bead range submitted for rewriting" << std::endl;
		return 0;
	}
	const BeadRange& range = matchInfo.getSubstitutionBeadRange();
	BarzelBead& theBead = *(range.first);

	if( !transP ) { // null translatons shouldnt happen  .. 
		AYLOG(WARNING) << "null translation detected" << std::endl;
		return 0;
	}

	const BarzelTranslation& translation = *transP;

	BarzelEvalResult transResult;
	BarzelEvalNode evalNode;

	//AYDEBUG( matchInfo.getSubstitutionBeadRange() );
	BarzelEvalContext ctxt( matchInfo, universe );
	if( translation.isRewriter() ) { /// translation is a rewriter 
		BarzelRewriterPool::BufAndSize bas;
		if( !universe.getBarzelRewriter( bas, translation )) {
			AYLOG(ERROR) << "no bytecode in rewriter" << std::endl;
			theBead.setStopLiteral();
			return 0;
		}

		/// constructing eval tree
		if( !evalNode.growTree( bas, ctxt ) ) {
			AYLOG(ERROR) << "eval tree construction failed" << std::endl;
			theBead.setStopLiteral();
			return 0;
		}
		/// end of custom rewriter handling 
	} else {  /// trivial translation (non-rewrite - no bytecode there)

		// creating a childless evalNode 
		translation.fillRewriteData( evalNode.getBtnd() );
	}
	if( !evalNode.eval( transResult, ctxt ) ) {
		AYLOG(ERROR) << "evaluation failed" << std::endl;
		theBead.setStopLiteral();
		return 0;
	}

	// the range will be replaced with a single bead . 
	// so we will simply delete everything past the first bead
	theBead.setData(  transResult.getBeadData() );

	chain.collapseRangeLeft( range );
	return 0;
}
int BarzelMatcher::matchAndRewrite( Barz& barz )
{
	//AYTRACE( "BarzelMatcher::matchAndRewrite unimplemented" );
	clear();

	BarzelBeadChain& beads = barz.getBeads();

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
