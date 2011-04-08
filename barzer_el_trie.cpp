#include <barzer_el_trie.h>

namespace barzer {
BarzelTrieNode* BarzelTrieNode::addPattern( BELTrie& rewrPool, const BTND_PatternData& p )
{
	BarzelTrieFirmChildKey key(p);
	if( !key.isBlank() ) {
		// here we should try to generate a WCCLookupKey 
		return this;
	} else {
		BarzelFCMap::iterator i = firmMap.find( key );
		if( i == firmMap.end() ) { // creating new one
			i = firmMap.insert( BarzelFCMap::value_type( key, BarzelTrieNode() ) ).first;
		}
		return &(i->second);
	}
	 
}
std::ostream& BarzelTrieFirmChildKey::print( std::ostream& fp ) const
{
	return( fp << BTNDDecode::typeName_Pattern( type )  << ":" << std::hex << id );
}

void BELTrie::addPath( const BTND_PatternDataVec& path, const BELParseTreeNode& trans )
{
	BarzelTrieNode* n = &root;
	for( BTND_PatternDataVec::const_iterator i = path.begin(); i!= path.end(); ++i ) {
		if( n ) 
			n = n->addPattern( *this, *i );
		else {
			AYTRACE("addPattern returned NULL") ;
			return; // this is impossible
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
		#warning logic to autogenerate compound word ids can go here

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
		id = x.val;
	else {
		AYTRACE( "inconsistent number encountered in Barzel rewrite" );
		setStop();
		break;
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
			set( boost::get<BTND_Rewrite_Literal>( *rwrDta ) );
			return;
		case BTND_Rewrite_Number_TYPE:
			set( boost::get<BTND_Rewrite_Number>( *rwrDta ) );
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
