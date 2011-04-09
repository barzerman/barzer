#include <barzer_el_trie.h>
#include <barzer_el_wildcard.h>

namespace barzer {
/// barzel TRIE node methods
BarzelTrieNode* BarzelTrieNode::addFirmPattern( BELTrie& trie, const BTND_PatternData& p )
{
	BarzelTrieFirmChildKey key(p);
	if( key.isBlank() ) {
		AYTRACE( "illegally attempting to add a wildcard" );
		return this;
	} else {
		BarzelFCMap::iterator i = firmMap.find( key );
		if( i == firmMap.end() ) { // creating new one
			i = firmMap.insert( BarzelFCMap::value_type( key, BarzelTrieNode() ) ).first;
		}
		return &(i->second);
	}
	 
}

BarzelTrieNode* BarzelTrieNode::addWildcardPattern( BELTrie& trie, const BTND_PatternData& p, const BarzelTrieFirmChildKey& fk  )
{
	if( key.isBlank() ) {
	}
	 
}
std::ostream& BarzelTrieFirmChildKey::print( std::ostream& fp ) const
{
	return( fp << BTNDDecode::typeName_Pattern( type )  << ":" << std::hex << id );
}

//// bel TRIE methods
void BELTrie::produceWCKey( BarzelWCKey& key, const BTND_PatternData& btnd  )
{
	wcPool->produceWCKey(key, btnd );
}

namespace {
	typedef std::pair< BTND_PatternDataVec::const_iterator, BarzelTrieFirmChildKey > WCPatDta;
	typedef std::list< WCPatDta > WCPatDtaList;

}

void BELTrie::addPath( const BTND_PatternDataVec& path, const BELParseTreeNode& trans )
{
	BarzelTrieNode* n = &root;

	// forming the list of wildcard pattern data. data includes iterator to the actual
	// wildcard as well as the next firm match key (if any)
	WCPatDtaList wcpdList;
	WCPatDtaList::iterator firstWC = wcpdList.end();

	for( BTND_PatternDataVec::const_iterator i = path.begin(); i!= path.end(); ++i ) {
		BarzelTrieFirmChildKey firmKey(*i);

		if( firmKey.isBlank() ) {
			wcpdList.push_back( WCPatDta(i,BarzelTrieFirmChildKey() ) );
			WCPatDtaList::iterator firstWC = wcpdList.rbegin().base();
		} else {
			if( firstWC != wcpdList.end() ) {
				for( WCPatDtaList::iterator li = firstWC; li != firstWC.end(); ++li ) {
					li->nextFirmKey = firmKey;
				}
				firstWC = wcpdList.end();
			}
		}
	}
	/// at this point wcpdList has data on all wildcards in the path - it stores pairs 
	/// (iterator of path, nextFirmKey). now we run through the path again 

	for( BTND_PatternDataVec::const_iterator i = path.begin(); i!= path.end(); ++i ) {
		if( !wcpdList.empty() && i == wcpdList.front().first ) { // we reached a wildcard
			if( n ) 
				n = n->addWildcardPattern( *this, *i, wcpdList.front().second );
			else {
				AYTRACE("addPattern for wildcard returned NULL") ;
				return; // this is impossible
			}
			wcpdList.pop_front(); // now first element points to the next wildcard (if any are left)
		} else { // reached a firm token
			if( n ) 
				n = n->addPattern( *this, *i );
			else {
				AYTRACE("addPattern returned NULL") ;
				return; // this is impossible
			}
		}
	}
	if( n )
		n->setTranslation( *this, trans );
}
//// BarzelTranslation methods

void BarzelTranslation::set(BELTrie& ,const BTND_Rewrite_Literal& x )
{
	switch( x.type ) {
	case BTND_Rewrite_Literal::T_STRING:
		type = T_STRING;
		id = x.theId;
		break;
	case BTND_Rewrite_Literal::T_COMPOUND:
		type = T_COMPWORD;
		//#warning logic to autogenerate compound word ids can go here

		id = x.theId;
		break;
	case BTND_Rewrite_Literal::T_STOP:
		type = T_STOP;
		id = 0xffffffff;
		break;
	default: 
		AYTRACE( "inconsistent literal encountered in Barzel rewrite" );
		setStop(); // this should never happen
		break;
	}
}
void BarzelTranslation::set(BELTrie&, const BTND_Rewrite_Number& x )
{
	if( x.isConst ) 
		id = ( x.val.which() ? boost::get<double>(x.val) : boost::get<int>(x.val) );
	else {
		AYTRACE( "inconsistent number encountered in Barzel rewrite" );
		setStop();
	}
}

void BarzelTranslation::set( BELTrie& trie, const BELParseTreeNode& tn ) 
{
	/// diagnosing trivial rewrite usually root will have one childless child
	/// that childless child may be a trivial transform 
	/// theoretically root itself may contain the transform (if we process attribs)
	const BTND_RewriteData* rwrDta = tn.getTrivialRewriteData();
	if( rwrDta ) {
		switch( rwrDta->which() ) {
		case BTND_Rewrite_Literal_TYPE:
			set( trie,boost::get<BTND_Rewrite_Literal>( *rwrDta ) );
			return;
		case BTND_Rewrite_Number_TYPE:
			set( trie,boost::get<BTND_Rewrite_Number>( *rwrDta ) );
			return;
		default: 
			AYTRACE("invalid trivial rewrite" );
			setStop(); 
			return;
		}
	} else { // non trivial rewrite 
		if( trie.rewrPool->produceTranslation( *this, tn ) != BarzelRewriterPool::ERR_OK ) {
			setStop();
		}
	}
}


} // end namespace barzer
