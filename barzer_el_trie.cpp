#include <barzer_el_trie.h>

namespace barzer {
BarzelTrieNode* BarzelTrieNode::addPattern( const BTND_PatternData& p )
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
			n = n->addPattern( *i );
		else {
			AYTRACE("addPattern returned NULL") ;
			return; // this is impossible
		}
	}
	if( n )
		n->setTranslation( trans );
}

} // end namespace barzer
