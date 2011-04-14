#include <barzer_el_trie.h>
#include <barzer_el_wildcard.h>
#include <list>
#include <ay/ay_logger.h>


namespace barzer {
///// output operators 

const BarzelWCLookup*  BELPrintContext::getWildcardLookup( uint32_t id ) const
{
return( trie.wcPool->getWCLookup( id ));
}

void BarzelTrieNode::clear()
{
	firmMap.clear();
	wcLookupId = 0xffffffff;
}
void BELTrie::clear()
{
	rewrPool->clear();
	wcPool->clear();
	root.clear();
}

std::ostream& BELTrie::print( std::ostream& fp, BELPrintContext& ctxt  ) const
{

	fp << "/" << std::endl;
	root.print( fp, ctxt );
	return fp;
}

/// print one trie node 
std::ostream& BarzelTrieNode::print( std::ostream& fp , BELPrintContext& ctxt ) const
{
	ctxt.descend();

	bool needNewline = true;
	if( hasFirmChildren() ) {
		print_firmChildren( fp, ctxt );
		fp << "\n";
		needNewline = false;
	} else 
		fp << "[no firm]";
	
	if( hasWildcardChildren() ) {
		print_wcChildren(fp, ctxt );
		fp << "\n";
		needNewline = false;
	} else 
		fp << "[no wildcards]";

	if( isLeaf() ) 
		print_translation( (fp << " *LEAF*=> ["), ctxt) << "]";
	
	if( needNewline ) 
		fp << "\n";

	ctxt.ascend();
	return fp;
}

std::ostream& BarzelTrieNode::print_translation( std::ostream& fp, const BELPrintContext& ctxt ) const
{

	return translation.print( fp, ctxt );
}

std::ostream& BarzelTrieNode::print_firmChildren( std::ostream& fp, BELPrintContext& ctxt ) const
{
	for( BarzelFCMap::const_iterator i = firmMap.begin(); i!= firmMap.end() ; ++i ) {
		i->first.print( (fp << ctxt.prefix), ctxt);
		if( ctxt.needDescend() ) {
			const BarzelTrieNode& child = i->second;
			child.print( fp, ctxt );
		} else {
			fp << "/\n";
		}
	}
	return fp;
}

std::ostream& BarzelTrieNode::print_wcChildren( std::ostream& fp, BELPrintContext& ctxt ) const
{
	const BarzelWCLookup* wcLookup = ctxt.getWildcardLookup( wcLookupId );
	if( !wcLookup ) {
		std::cerr << "Barzel Trie FATAL: lookup id " << std::hex << wcLookupId << " is invalid\n";
		return fp;
	}
	for( BarzelWCLookup::const_iterator i = wcLookup->begin(); i != wcLookup->end(); ++i ) {
		const BarzelWCLookupKey& lupKey = i->first;
		ctxt.printBarzelWCLookupKey( (fp << ctxt.prefix << "W:"), lupKey );
		if( ctxt.needDescend() ) {
			const BarzelTrieNode& child = i->second;
			child.print( fp, ctxt );
		} else {
			fp << "/\n";
		}
	}
	return fp;
}

std::ostream& BELPrintContext::printBarzelWCLookupKey( std::ostream& fp, const BarzelWCLookupKey& key ) const
{
	const BarzelTrieFirmChildKey& firmKey = key.first;
	const BarzelWCKey& wcKey = key.second;
	
	trie.wcPool->print( fp, wcKey, *this );
	if( !firmKey.isBlank() ) {
		fp << "-->";
		firmKey.print( fp, *this );
	}
	return fp;
}


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
	if( !hasValidWcLookup() ) 
		trie.wcPool->addWCLookup( wcLookupId );
	
	BarzelWCLookup* wcLookup = trie.wcPool->getWCLookup( wcLookupId );
	BarzelWCLookupKey lkey;
	lkey.first = fk;
	trie.produceWCKey( lkey.second, p );
	BarzelWCLookup::iterator i = wcLookup->find( lkey );	
	if( i == wcLookup->end() ) 
		i = wcLookup->insert( BarzelWCLookup::value_type( lkey, BarzelTrieNode()  ) ).first;	
	
	return &(i->second);
}
std::ostream& BarzelTrieFirmChildKey::print( std::ostream& fp ) const
{
	return( fp << BTNDDecode::typeName_Pattern( type )  << ":" << std::hex << id );
}

std::ostream& BarzelTrieFirmChildKey::print( std::ostream& fp, const BELPrintContext& ctxt ) const
{
	return( fp << BTNDDecode::typeName_Pattern( type )  << ":" << std::hex << id << "[" << 
	ctxt.printableString(id) <<
	"]" );
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

	AYLOG(DEBUG) << "Adding new path (" << path.size() << " elements)";

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
				for( WCPatDtaList::iterator li = firstWC; li != wcpdList.end(); ++li ) {
					li->second = firmKey;
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
				n = n->addFirmPattern( *this, *i );
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
	switch( x.getType() ) {
	case BTND_Rewrite_Literal::T_STRING:
		type = T_STRING;
		id = x.getId();
		break;
	case BTND_Rewrite_Literal::T_COMPOUND:
		type = T_COMPWORD;
		//#warning logic to autogenerate compound word ids can go here

		id = x.getId();
		break;
	case BTND_Rewrite_Literal::T_STOP:
		type = T_STOP;
		id = 0xffffffff;
		break;
	case BTND_Rewrite_Literal::T_BLANK:
		type = T_BLANK;
		id = 0xffffffff;
		break;
	case BTND_Rewrite_Literal::T_PUNCT:
		type = T_PUNCT;
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
		if( x.isReal() ) 
			id = x.getReal();
		else
			id = x.getInt();
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

std::ostream& BarzelTranslation::print( std::ostream& fp, const BELPrintContext& ctxt) const
{
	switch(type ) {
	case T_NONE: return ( fp << "<none>" );
	case T_STOP: return ( fp << "#" );
	case T_STRING: return ( fp << "STRING" );
	case T_COMPWORD: return ( fp << "COMP" );
	case T_NUMBER_INT: return ( fp << "INTEGER" );
	case T_NUMBER_REAL: return ( fp << "REAL" );
	case T_REWRITER:
	{
		BarzelRewriterPool::BufAndSize bas; 
		ctxt.trie.rewrPool->resolveTranslation( bas, *this );
		fp << "RWR(";
		fp << "rewriter bytes[" << bas.second << "]";
		fp << ")";
	}
		break;
		
	}
	return ( fp << "unknown translation" );
}

} // end namespace barzer
