#include <barzer_el_chain.h>
#include <barzer_el_matcher.h>
#include <barzer_el_btnd.h>
#include <barzer_universe.h>
#include <barzer_barz.h>
#include <ay_logger.h>

namespace barzer {

namespace {
inline std::ostream& print_NodeAndBeadVec( std::ostream& fp, const NodeAndBeadVec& v, BeadList::iterator end )
{
	for( NodeAndBeadVec::const_iterator i = v.begin(); i!= v.end() ; ++i) {
		fp << "* ELEMENT[" << i-v.begin() << "]" << ( i->first->isWcChild() ? " WC" : "~") << "\n";
		i->second.first->print( fp << "**FIRST** " );
		if( i->second.second != end ) 
			i->second.second->print( fp << "**SECOND** " );
		else 
			fp << "**SECOND ===>  END OF LIST\n";
	}
	return fp;
}
}
int BTMIterator::addTerminalPath( const NodeAndBead& nb )
{
	NodeAndBead theNb(nb);
	BeadRange& br = theNb.second;
	if( br.first == br.second ) {
		if( br.second != bestPaths.getAbsoluteEnd() && br.second->isBlankLiteral() ) {
			++br.second;
		}
	}
	//AYDEBUG(br);
	ay::vector_raii<NodeAndBeadVec> raii( d_matchPath, theNb );
	//print_NodeAndBeadVec( std::cerr << "**ADD TERMINAL >>>", d_matchPath, bestPaths.getAbsoluteEnd() ) <<
	//" <<<<< END\n" ;
	bestPaths.addPath( d_matchPath );
	return 0;
}

namespace {

/// T is the atomic bead variant member type
template <typename T> struct BeadToPattern_convert { typedef  BTND_Pattern_None PatternType; };

template <> struct BeadToPattern_convert<BarzerNumber> { typedef BTND_Pattern_Number PatternType; };
template <> struct BeadToPattern_convert<BarzerDate> { typedef BTND_Pattern_Date PatternType; };
template <> struct BeadToPattern_convert<BarzerDateTime> { typedef BTND_Pattern_DateTime PatternType; };
template <> struct BeadToPattern_convert<BarzerEntityList> { typedef BTND_Pattern_Entity PatternType; };
template <> struct BeadToPattern_convert<BarzerEntity> { typedef BTND_Pattern_Entity PatternType; };
template <> struct BeadToPattern_convert<BarzerEntityRangeCombo> { typedef BTND_Pattern_Entity PatternType; };
template <> struct BeadToPattern_convert<BarzerERCExpr> { typedef BTND_Pattern_ERCExpr PatternType; };


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
{ return d_pattern( dta ); }
//// end of number template matching
//// other patterns 
/// for now these patterns always match on a right data type (date matches any date, datetime any date time etc..)
/// they can be certainly further refined
template <> template <>
inline bool evalWildcard_vis<BTND_Pattern_Date>::operator()<BarzerDate> ( const BarzerDate& dta ) const
{ 
	return d_pattern( dta );
}

template <> template <>
inline bool evalWildcard_vis<BTND_Pattern_DateTime>::operator()<BarzerDateTime> ( const BarzerDateTime& dta )  const
{ return d_pattern( dta ); }

template <> template <>
inline bool evalWildcard_vis<BTND_Pattern_Time>::operator()<BarzerTimeOfDay> ( const BarzerTimeOfDay& dta )  const
{ return true; }

template <> template <>
inline bool evalWildcard_vis<BTND_Pattern_Entity>::operator()<BarzerEntityList> ( const BarzerEntityList& dta )  const
{ return d_pattern( dta ); }
template <> template <>
inline bool evalWildcard_vis<BTND_Pattern_Entity>::operator()<BarzerEntity> ( const BarzerEntity& dta )  const
{ return d_pattern( dta ); }

template <> template <>
inline bool evalWildcard_vis<BTND_Pattern_Entity>::operator()<BarzerEntityRangeCombo> ( const BarzerEntityRangeCombo& dta )  const
{ return d_pattern( dta ); }

template <> template <>
inline bool evalWildcard_vis<BTND_Pattern_Wildcard>::operator()<BarzerLiteral> ( const BarzerLiteral& dta )  const
{ return true; }
//// end of other template matching


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
	const BeadRange&		d_rng;
	
	const BarzelTrieNode* 	d_tn;

	bool d_followsBlank;
	
	findMatchingChildren_visitor( BTMIterator& bi, bool followsBlanks, NodeAndBeadVec& mC, const BeadRange& r, const BarzelTrieNode* t ):
		d_btmi(bi), 
		d_mtChild(mC), 
		d_rng(r), 
		d_tn(t), 
		d_followsBlank(followsBlanks)
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
		if( d_btmi.evalWildcard( wcKey, d_rng.first, iter, tokSkip ) ) {
			// trimming blanks from left and right
			BarzelBeadChain::Range goodRange(d_rng.first,iter);
			//BarzelBeadChain::trimBlanksFromRange( goodRange );
			//AYDEBUG(goodRange);
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

	template <typename T>
	bool doFirmMatch( const BarzelFCMap& fcmap, const T& dta, bool allowBlanks=false ) {
		return false;
	}

	template <typename T>
	bool doFirmMatch_default( const BarzelFCMap& fcmap, const T& dta, bool allowBlanks=false )
	{
		typedef typename BeadToPattern_convert<T>::PatternType PatternType;
		BTND_Pattern_TypeId_Resolve typeIdResolve;
		uint8_t patternTypeId = typeIdResolve(dta);

		BarzelTrieFirmChildKey firmKey(patternTypeId,0xffffffff); 
		// forming firm key
		// we ignore the whole allow blanks thing for dates - blanks will just always be allowed
		BarzelFCMap::const_iterator i = fcmap.lower_bound( firmKey );
		const BarzelWildcardPool& wcPool = d_btmi.universe.getWildcardPool();
		for( ; i!= fcmap.end() && i->first.type == patternTypeId; ++i ) {
			/// we're looping over all date wildcards on the current node 
			/// extracting the actual wildcard 
			if( i->first.id == 0xffffffff ) {
				const BarzelTrieNode* ch = &(i->second);
				BarzelBeadChain::Range goodRange(d_rng.first,d_rng.first);
				d_mtChild.push_back( NodeAndBeadVec::value_type(ch,goodRange) );
			} else {
				const BTND_Pattern_Entity* dpat = wcPool.get_BTND_Pattern< PatternType > ( i->first.id );
				if( !dpat ) return false;
				if( evalWildcard_vis< PatternType >(*dpat)( dta )  ) {
					d_mtChild.push_back( NodeAndBeadVec::value_type(
						&(i->second),
						BarzelBeadChain::Range(d_rng.first,d_rng.first)) );
				}
			}
		}
		return true;
	}

	template <typename T>
	bool operator()( const T& dta ) 
	{
		const BarzelFCMap* fcmap = d_btmi.universe.getBarzelTrie().getBarzelFCMap( *d_tn );
		if( fcmap ) {
			/// trying to match single wildcard literals
			doFirmMatch( *fcmap, dta, true );
	
			if( d_followsBlank ) 
				d_followsBlank = false;
		}
		doWildcards( );
		return (d_mtChild.size() > 0);
	}
	
};
	template <>
	bool findMatchingChildren_visitor::doFirmMatch<BarzerLiteral>( const BarzelFCMap& fcmap, const BarzerLiteral& dta, bool allowBlanks) 
	{
		BarzelTrieFirmChildKey firmKey; 
		// forming firm key
		bool curDtaIsBlank = firmKey.set(dta,d_followsBlank).isBlankLiteral();
		if( !allowBlanks && curDtaIsBlank ) {
			return false; // this should never happen blanks are skipped
		}

		const BarzelTrieNode* ch = d_tn->getFirmChild( firmKey, fcmap ); 
		if( ch ) {
			BeadList::iterator endIt = d_rng.first;
			//++endIt;
			BarzelBeadChain::Range goodRange(d_rng.first,endIt);
			// AYDEBUG(goodRange);
			d_mtChild.push_back( NodeAndBeadVec::value_type(ch, goodRange ) );
		} 
		/// retrying in case the token immediately follows non blank 
		if( !d_followsBlank ) {
			firmKey.noLeftBlanks = 0;
			ch = d_tn->getFirmChild( firmKey, fcmap ); 
			if( ch ) {
				BeadList::iterator endIt = d_rng.first;

				BarzelBeadChain::Range goodRange(d_rng.first,endIt);
				d_mtChild.push_back( NodeAndBeadVec::value_type(ch,goodRange) );
			}
		}
	
		return true;
	}
	template <>
	bool findMatchingChildren_visitor::doFirmMatch<BarzerDate>( const BarzelFCMap& fcmap, const BarzerDate& dta, bool allowBlanks) 
	{
		BarzelTrieFirmChildKey firmKey((uint8_t)(BTND_Pattern_Date_TYPE),0xffffffff); 
		// forming firm key
		// we ignore the whole allow blanks thing for dates - blanks will just always be allowed
		BarzelFCMap::const_iterator i = fcmap.lower_bound( firmKey );
		const BarzelWildcardPool& wcPool = d_btmi.universe.getWildcardPool();
		for( ; i!= fcmap.end() && i->first.type == BTND_Pattern_Date_TYPE; ++i ) {
			/// we're looping over all date wildcards on the current node 
			/// extracting the actual wildcard 
			if( i->first.id == 0xffffffff ) {
				const BarzelTrieNode* ch = &(i->second);
				BarzelBeadChain::Range goodRange(d_rng.first,d_rng.first);
				d_mtChild.push_back( NodeAndBeadVec::value_type(ch,goodRange) );
			} else {
				const BTND_Pattern_Date* dpat = wcPool.get_BTND_Pattern_Date( i->first.id );
				if( !dpat ) return false;
				if( evalWildcard_vis<BTND_Pattern_Date>(*dpat)( dta )  ) {
				//if( boost::apply_visitor( evalWildcard_vis<BTND_Pattern_Date>(*dpat), dta ) ) {}
					d_mtChild.push_back( NodeAndBeadVec::value_type(
						&(i->second),
						BarzelBeadChain::Range(d_rng.first,d_rng.first)) );
				}
			}
		}
		return true;
	}
	template <>
	bool findMatchingChildren_visitor::doFirmMatch<BarzerDateTime>( const BarzelFCMap& fcmap, const BarzerDateTime& dta, bool allowBlanks) 
	{
		BarzelTrieFirmChildKey firmKey((uint8_t)(BTND_Pattern_DateTime_TYPE),0xffffffff); 
		// forming firm key
		// we ignore the whole allow blanks thing for dates - blanks will just always be allowed
		BarzelFCMap::const_iterator i = fcmap.lower_bound( firmKey );
		const BarzelWildcardPool& wcPool = d_btmi.universe.getWildcardPool();
		for( ; i!= fcmap.end() && i->first.type == BTND_Pattern_DateTime_TYPE; ++i ) {
			/// we're looping over all date wildcards on the current node 
			/// extracting the actual wildcard 
			if( i->first.id == 0xffffffff ) {
				const BarzelTrieNode* ch = &(i->second);
				BarzelBeadChain::Range goodRange(d_rng.first,d_rng.first);
				d_mtChild.push_back( NodeAndBeadVec::value_type(ch,goodRange) );
			} else {
				const BTND_Pattern_DateTime* dpat = wcPool.get_BTND_Pattern_DateTime( i->first.id );
				if( !dpat ) return false;
				if( evalWildcard_vis<BTND_Pattern_DateTime>(*dpat)( dta )  ) {
				//if( boost::apply_visitor( evalWildcard_vis<BTND_Pattern_DateTime>(*dpat), dta ) ) {}
					d_mtChild.push_back( NodeAndBeadVec::value_type(
						&(i->second),
						BarzelBeadChain::Range(d_rng.first,d_rng.first)) );
				}
			}
		}
		return true;
	}
	template <>
	bool findMatchingChildren_visitor::doFirmMatch<BarzerNumber>( const BarzelFCMap& fcmap, const BarzerNumber& dta, bool allowBlanks) 
	{
		BarzelTrieFirmChildKey firmKey((uint8_t)(BTND_Pattern_Number_TYPE),0xffffffff); 
		// forming firm key
		// we ignore the whole allow blanks thing for dates - blanks will just always be allowed
		BarzelFCMap::const_iterator i = fcmap.lower_bound( firmKey );
		const BarzelWildcardPool& wcPool = d_btmi.universe.getWildcardPool();
		for( ; i!= fcmap.end() && i->first.type == BTND_Pattern_Number_TYPE; ++i ) {
			/// we're looping over all date wildcards on the current node 
			/// extracting the actual wildcard 
			if( i->first.id == 0xffffffff ) {
				const BarzelTrieNode* ch = &(i->second);
				BarzelBeadChain::Range goodRange(d_rng.first,d_rng.first);
				d_mtChild.push_back( NodeAndBeadVec::value_type(ch,goodRange) );
			} else {
				const BTND_Pattern_Number* dpat = wcPool.get_BTND_Pattern_Number( i->first.id );
				if( !dpat ) return false;
				if( evalWildcard_vis<BTND_Pattern_Number>(*dpat)( dta )  ) {
				//if( boost::apply_visitor( evalWildcard_vis<BTND_Pattern_Number>(*dpat), dta ) ) {}
					d_mtChild.push_back( NodeAndBeadVec::value_type(
						&(i->second),
						BarzelBeadChain::Range(d_rng.first,d_rng.first)) );
				}
			}
		}
		return true;
	}

	template <>
	bool findMatchingChildren_visitor::doFirmMatch<BarzerEntityList>( const BarzelFCMap& fcmap, const BarzerEntityList& dta, bool allowBlanks) 
	{
		return doFirmMatch_default( fcmap, dta, allowBlanks );
	}

	template <>
	bool findMatchingChildren_visitor::doFirmMatch<BarzerEntity>( const BarzelFCMap& fcmap, const BarzerEntity& dta, bool allowBlanks) 
	{
		BarzelTrieFirmChildKey firmKey((uint8_t)(BTND_Pattern_Entity_TYPE),0xffffffff); 
		// forming firm key
		// we ignore the whole allow blanks thing for dates - blanks will just always be allowed
		BarzelFCMap::const_iterator i = fcmap.lower_bound( firmKey );
		const BarzelWildcardPool& wcPool = d_btmi.universe.getWildcardPool();
		for( ; i!= fcmap.end() && i->first.type == BTND_Pattern_Entity_TYPE; ++i ) {
			/// we're looping over all date wildcards on the current node 
			/// extracting the actual wildcard 
			if( i->first.id == 0xffffffff ) {
				const BarzelTrieNode* ch = &(i->second);
				BarzelBeadChain::Range goodRange(d_rng.first,d_rng.first);
				d_mtChild.push_back( NodeAndBeadVec::value_type(ch,goodRange) );
			} else {
				const BTND_Pattern_Entity* dpat = wcPool.get_BTND_Pattern_Entity( i->first.id );
				if( !dpat ) return false;
				if( evalWildcard_vis<BTND_Pattern_Entity>(*dpat)( dta )  ) {
				//if( boost::apply_visitor( evalWildcard_vis<BTND_Pattern_Entity>(*dpat), dta ) ) {}
					d_mtChild.push_back( NodeAndBeadVec::value_type(
						&(i->second),
						BarzelBeadChain::Range(d_rng.first,d_rng.first)) );
				}
			}
		}
		return true;
	}
	template <>
	bool findMatchingChildren_visitor::doFirmMatch<BarzerEntityRangeCombo>( const BarzelFCMap& fcmap, const BarzerEntityRangeCombo& dta, bool allowBlanks) 
	{
		BarzelTrieFirmChildKey firmKey((uint8_t)(BTND_Pattern_Entity_TYPE),0xffffffff); 
		// forming firm key
		// we ignore the whole allow blanks thing for dates - blanks will just always be allowed
		BarzelFCMap::const_iterator i = fcmap.lower_bound( firmKey );
		const BarzelWildcardPool& wcPool = d_btmi.universe.getWildcardPool();
		for( ; i!= fcmap.end() && i->first.type == BTND_Pattern_Entity_TYPE; ++i ) {
			/// we're looping over all date wildcards on the current node 
			/// extracting the actual wildcard 
			if( i->first.id == 0xffffffff ) {
				const BarzelTrieNode* ch = &(i->second);
				BarzelBeadChain::Range goodRange(d_rng.first,d_rng.first);
				d_mtChild.push_back( NodeAndBeadVec::value_type(ch,goodRange) );
			} else {
				const BTND_Pattern_Entity* dpat = wcPool.get_BTND_Pattern_Entity( i->first.id );
				if( !dpat ) return false;
				if( evalWildcard_vis<BTND_Pattern_Entity>(*dpat)( dta )  ) {
				//if( boost::apply_visitor( evalWildcard_vis<BTND_Pattern_Entity>(*dpat), dta ) ) {}
					d_mtChild.push_back( NodeAndBeadVec::value_type(
						&(i->second),
						BarzelBeadChain::Range(d_rng.first,d_rng.first)) );
				}
			}
		}
		return true;
	}
	template <>
	bool findMatchingChildren_visitor::doFirmMatch<BarzerERCExpr>( const BarzelFCMap& fcmap, const BarzerERCExpr& dta, bool allowBlanks) 
	{
		BarzelTrieFirmChildKey firmKey((uint8_t)(BTND_Pattern_ERCExpr_TYPE),0xffffffff); 
		// forming firm key
		// we ignore the whole allow blanks thing for dates - blanks will just always be allowed
		BarzelFCMap::const_iterator i = fcmap.lower_bound( firmKey );
		const BarzelWildcardPool& wcPool = d_btmi.universe.getWildcardPool();
		for( ; i!= fcmap.end() && i->first.type == BTND_Pattern_ERCExpr_TYPE; ++i ) {
			/// we're looping over all date wildcards on the current node 
			/// extracting the actual wildcard 
			if( i->first.id == 0xffffffff ) {
				const BarzelTrieNode* ch = &(i->second);
				BarzelBeadChain::Range goodRange(d_rng.first,d_rng.first);
				d_mtChild.push_back( NodeAndBeadVec::value_type(ch,goodRange) );
			} else {
				const BTND_Pattern_ERCExpr* dpat = wcPool.get_BTND_Pattern_ERCExpr( i->first.id );
				if( !dpat ) return false;
				if( evalWildcard_vis<BTND_Pattern_ERCExpr>(*dpat)( dta )  ) {
				//if( boost::apply_visitor( evalWildcard_vis<BTND_Pattern_ERCExpr>(*dpat), dta ) ) {}
					d_mtChild.push_back( NodeAndBeadVec::value_type(
						&(i->second),
						BarzelBeadChain::Range(d_rng.first,d_rng.first)) );
				}
			}
		}
		return true;
	}

	template <>
inline	bool findMatchingChildren_visitor::operator()<BarzerLiteral> ( const BarzerLiteral& dta ) 
	{
		const BarzelFCMap* fcmap = d_btmi.universe.getBarzelTrie().getBarzelFCMap( *d_tn );
		if( fcmap ) {
			doFirmMatch( *fcmap, dta );
			/// trying to match single wildcard literals
			BarzerLiteral wcLit = dta;
			wcLit.setId( 0xffffffff );
			doFirmMatch( *fcmap, wcLit, true );
	
			if( d_followsBlank ) 
				d_followsBlank = false;
		}
		// ch is 0 here 
		doWildcards( );
		return (d_mtChild.size() > 0);
	}
	template <>
inline	bool findMatchingChildren_visitor::operator()<BarzerString> ( const BarzerString& dta ) 
	{
		const BarzelFCMap* fcmap = d_btmi.universe.getBarzelTrie().getBarzelFCMap( *d_tn );
		if( fcmap ) {
			BarzerLiteral wcLit;
			doFirmMatch( *fcmap, wcLit, true );
			wcLit.setBlank();
			doFirmMatch( *fcmap, wcLit, true );
			if( d_followsBlank ) 
				d_followsBlank = false;
		}
		doWildcards( );
		return (d_mtChild.size() > 0 );
	}

}

bool BTMIterator::evalWildcard( const BarzelWCKey& wcKey, BeadList::iterator fromI, BeadList::iterator theI, uint8_t tokSkip ) const
{
	const BarzelWildcardPool& wcPool = universe.getWildcardPool();
	const BarzelBeadAtomic* atomic = theI->getAtomic();
	if( !atomic ) 
		return false;
	
	switch( wcKey.wcType ) {
	case BTND_Pattern_Number_TYPE: {
		const BTND_Pattern_Number* p = wcPool.get_BTND_Pattern_Number(wcKey.wcId);
		return( p ?  boost::apply_visitor( evalWildcard_vis<BTND_Pattern_Number>(*p), atomic->dta ) : false );
	}
	case BTND_Pattern_Wildcard_TYPE: {
		const BTND_Pattern_Wildcard* p = wcPool.get_BTND_Pattern_Wildcard(wcKey.wcId);
		return( p ?  boost::apply_visitor( evalWildcard_vis<BTND_Pattern_Wildcard>(*p), atomic->dta ) : false );
	}
	case BTND_Pattern_Date_TYPE: {
		const BTND_Pattern_Date* p = wcPool.get_BTND_Pattern_Date(wcKey.wcId);
		return( p ?  boost::apply_visitor( evalWildcard_vis<BTND_Pattern_Date>(*p), atomic->dta ) : false );
	}
	case BTND_Pattern_Time_TYPE: {
		const BTND_Pattern_Time* p = wcPool.get_BTND_Pattern_Time(wcKey.wcId);
		return( p ?  boost::apply_visitor( evalWildcard_vis<BTND_Pattern_Time>(*p), atomic->dta ) : false );
	}
	case BTND_Pattern_DateTime_TYPE: {
		const BTND_Pattern_DateTime* p = wcPool.get_BTND_Pattern_DateTime(wcKey.wcId);
		return( p ?  boost::apply_visitor( evalWildcard_vis<BTND_Pattern_DateTime>(*p), atomic->dta ) : false );
	}
	case BTND_Pattern_Entity_TYPE: {
		const BTND_Pattern_Entity* p = wcPool.get_BTND_Pattern_Entity(wcKey.wcId);
		return( p ?  boost::apply_visitor( evalWildcard_vis<BTND_Pattern_Entity>(*p), atomic->dta ) : false );
	}
	default:
		return false;
	}
	return false;
}

bool BTMIterator::findMatchingChildren( NodeAndBeadVec& mtChild, const BeadRange& r, const BarzelTrieNode* tn )
{
	if( r.first == r.second ) 
		return false; // this should never happen bu just in case

	bool precededByBlanks = false;
	const BarzelBeadAtomic* bead;
	BeadRange rng(r);
	do {
		const BarzelBead& b = *(rng.first);
		bead = b.getAtomic();
		if( !bead )  /// expressions and blanks wont be matched 
			return false; 
		if( bead->isBlankLiteral() ) {
			++rng.first;
			if( !precededByBlanks )
				precededByBlanks= true;
		} else 
			break;
	} while( rng.first != rng.second );

	findMatchingChildren_visitor vis( *this, precededByBlanks, mtChild, rng, tn );

	boost::apply_visitor( vis, bead->dta );
	return mtChild.size();
}

void BTMIterator::matchBeadChain( const BeadRange& rng, const BarzelTrieNode* trieNode )
{
	//AYDEBUG( rng );
	NodeAndBead nbvVal ( trieNode, BarzelBeadChain::Range(rng.first,rng.first));
	ay::vector_raii<NodeAndBeadVec> raii( d_matchPath, nbvVal );

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
		const BeadRange& chRange = ch->second;

		nextBead = chRange.second;

		//print_NodeAndBeadVec( std::cerr << "*** MATCH BEAD CHAIN >>>", d_matchPath, rng.second ) << "<<<< **END\n";

		if( nextBead != rng.second ) {  // recursion
			
			/// setting proper bead range in the path
			
			BeadRange& pathRange = d_matchPath.back().second;
			pathRange.first = chRange.first;
			pathRange.second = chRange.second;
			if( pathRange.second != rng.second ) 
				++pathRange.second;
			if( tn->isLeaf() ) {
				//AYDEBUG( BarzelBeadChain::makeInclusiveRange(ch->second,rng.second) );
				addTerminalPath( *ch );
			}
			//BarzelBeadChain::trimBlanksFromRange( d_matchPath.back().second );

			//const BeadRange& shitRange = d_matchPath.back().second;
			//std::cerr << "************* SHIT recursion\n";
			//AYDEBUG( nextRange );
			// advancing to the next bead and starting new recursion from there			
			BeadRange nextRange( nextBead, rng.second );
			++nextRange.first;
			matchBeadChain( nextRange, tn );
		}  else {
			if( tn->isLeaf() ) {
				addTerminalPath( *ch );
			}
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
	const BarzelTrieNode* tn = getTrieNode();
	ru.second = ( tn ? tn->getTranslation(universe.getBarzelTrie()) : 0 );

	return 0;
}
/// bool indicates fallibility 
/// int is the score in case path doesnt fail
std::pair< bool, int > BTMBestPaths::scorePath( const NodeAndBeadVec& nb ) const
{
	if( !nb.size() ) 
		return( std::pair< bool, int >(true,0) );

	const BarzelTranslation* trans = nb.back().first->getTranslation(universe.getBarzelTrie());
	bool isTranslationFallible = ( trans ? universe.isBarzelTranslationFallible( *trans ) : false);

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



bool BarzelMatchInfo::getDataByVarId( BeadRange& r, uint32_t varId, const BELTrie& trie ) const
{
	const BarzelVariableIndex& varIdx = trie.getVarIndex();
	const BarzelTrieNode * leafNode = getTheLeafNode();
	if( !leafNode ) return false;
	uint32_t translationId = leafNode->getTranslationId();

	// varKey is the vector of uint32_t-s - id's of each variable (context) for 
	// variable a.b.c varKey would have 3 elements - stringId for a, b and c
	const BELSingleVarPath* varKey = varIdx.getPathFromTranVarId( varId );
	if( !varKey || !varKey->size() ) return false;
	/// varInfo - varKey mappings to BarzelTrieNodes (multimap)
	const BarzelSingleTranVarInfo* varInfo = varIdx.getBarzelSingleTranVarInfo( translationId );
	if( !varInfo ) return false;
	// now we iterate over varInfo multimap over all keys prefixed with varKey
	// all matches will be in varNodeSet (below)
	BarzelSingleTranVarInfo::const_iterator mi = varInfo->lower_bound( *varKey );
	if( mi == varInfo->end() ) return false;
	std::set< const BarzelTrieNode* > varNodeSet;
	for( ;mi != varInfo->end(); ++mi  ) {
		// seeing whether varKey is a prefix of mi->first 
		const BELSingleVarPath& miKey = mi->first;
		if( miKey.size() < varKey->size() ) 
			break;
		BELSingleVarPath::const_iterator vki= varKey->begin(), mki = miKey.begin(); 
		bool varKeyIsPrefixOfMiKey = true;
		for( ; vki != varKey->end(); ++vki,++mki ) {
			if( *vki != *mki ) {
				varKeyIsPrefixOfMiKey= false;
				break;
			}
		}
		if( varKeyIsPrefixOfMiKey ) 
			varNodeSet.insert( mi->second );
		else
			break;
	}
	// now computing the union range of all elements of the path such that their trie nodes are in 
	// the varNodeSet. running thru the path 
	if( !varNodeSet.size() ) return false;
	BeadRange resultRange;
	for( NodeAndBeadVec::const_iterator i = d_thePath.begin(); i!= d_thePath.end(); ++i ) {
		const BarzelTrieNode* tn = i->first;
		if( varNodeSet.find(tn) != varNodeSet.end() ) {
			if( resultRange.first == resultRange.second ) { // range is empty
				resultRange = i->second;
			} else { // there is already something in range
				// so we only extend the right edge
				resultRange.second = i->second.second;
			}
		}
	}
	r = resultRange;
	return( resultRange.first != resultRange.second );
}

bool BarzelMatchInfo::getGapRange( BeadRange& r, size_t n ) const
{
	const NodeAndBead* wc = getDataByWildcardNumber(n);
	if( !wc ) return ( r=BeadRange(), false );
	r = d_beadRng;
	if( n>  1 ) {
		if( n< d_thePath.size() ) {
			const NodeAndBead* wc0 = getDataByWildcardNumber(n-1);
		if( !wc0 ) { // this is completely impossible
					std::cerr << "getGapRange inconsistent\n";
				return ( r=BeadRange(), false );
			}
			r.first = wc0->second.second;
			r.second = wc->second.first;
		} else {
			r.first = wc->second.first;
		}
	} else {
		r.second = wc->second.first;
	}
	return true;
}

void BarzelMatchInfo::setPath( const NodeAndBeadVec& p )
{
	d_thePath = p;
	d_wcVec.clear();
	d_wcVec.reserve( d_thePath.size() );
	d_substitutionBeadRange.first = d_substitutionBeadRange.second = d_beadRng.second;
	for( NodeAndBeadVec::const_iterator i = d_thePath.begin(); i!= d_thePath.end(); ++i ) {
		if( i->first->isWcChild() )  {
			if( i != d_thePath.begin() )  {
				// const NodeAndBead& nab = *(i-1);
				d_wcVec.push_back( (i-d_thePath.begin())-1 );
			}
		}
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

	if( transResult.isVec() ) {
		//std::cerr << "*** BEFORE::::>>\n";
		//AYDEBUG( chain.getFullRange() );
		const BarzelEvalResult::BarzelBeadDataVec& bbdv = transResult.getBeadDataVec();
		BeadList::iterator bi = range.first;
		BarzelEvalResult::BarzelBeadDataVec::const_iterator di = bbdv.begin();
		for( ; di != bbdv.end() && bi!= range.second; )  {
			//bi->print( std::cerr );
			bi->setData( *di );
			if( ++di == bbdv.end() ) 
				break;
			if( ++bi == range.second ) 
				break;
		}
		if( di != bbdv.end() ) { // vector is longer than the bead range 
			for( ; di != bbdv.begin(); ++di )  
				chain.insertBead( range.second, *di );
		} else if( bi != range.second ) { // vector is shorter than the list and we need to fold the remainder of the list
			if( bi != range.first ) 
				chain.collapseRangeLeft( BeadRange(bi, range.second) );
		}
		//std::cerr << "<<<<< *** AFTER ::::>>\n";
		//AYDEBUG( chain.getFullRange() );
	} else {
		theBead.setData(  transResult.getBeadData() );
		chain.collapseRangeLeft( range );
	}
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
		//std::cerr << "**************** SHIT BEFORE {\n";
		//AYDEBUG( beads.getFullRange() );
		//std::cerr << "} **************** end SHIT BEFORE {\n";
		int rewrRc = rewriteUnit( rewrUnit, beads );
		//std::cerr << "**************** SHIT AFTER {\n";
		//AYDEBUG( beads.getFullRange() );
		//std::cerr << "} **************** end SHIT AFTER {\n";
		if( rewrRc ) { // should never be the case
			d_err.setFlag( Error::EF_RWRFAILS );
			AYLOG(ERROR) << "rewrite failed"  << std::endl;
		} else
			++rewrCount;
	}
	return rewrCount;
}

} // namespace barzer ends 
