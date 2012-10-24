#include <barzer_el_trie.h>
#include <barzer_el_wildcard.h>
#include <list>
#include <ay/ay_logger.h>
#include <barzer_universe.h>


namespace barzer {
///// output operators 

namespace {
std::ostream& glob_printRewriterByteCode( std::ostream& fp, const BarzelRewriterPool::BufAndSize& bas, const BELPrintContext& ctxt )
{
	BRBCPrintCB printer( fp, ctxt );
	BarzelRewriteByteCodeProcessor<BRBCPrintCB> processor(printer,bas);
	processor.run();
	return fp;
}
} // end of anon namespace 

void BELTrie::addStemSrc ( uint32_t stemId, uint32_t srcId ) 
{ 
    d_stemSrcs [stemId].insert(srcId); 
    globalPools.addStemSrc( stemId, srcId );
}
void BELTrie::setTrieClassAndId( const char* c, const char* i ) 
{ 
    d_trieClass_strId = globalPools.internString_internal( c? c:"" ) ;
    d_trieId_strId    = globalPools.internString_internal( i?i:"" );
}
BELTrie::BELTrie( const BELTrie& a ) : 
	globalPools(a.globalPools), 
	d_globalTriePoolId(a.d_globalTriePoolId),
	d_spellPriority(0),
    d_trieClass_strId(a.globalPools.internString_internal("")),
    d_trieId_strId(a.globalPools.internString_internal("")),
	procs(*this)
{}
    // const char * s = gp.internalString_resolve( *i );

WordMeaningBufPtr BELTrie::getMeanings (const BarzerLiteral& l) const
{
	return WordMeaningBufPtr(0, 0);
}

const char* BELTrie::getTrieClass() const { 
    const char* s = globalPools.internalString_resolve( d_trieClass_strId );
    return ( s ? s : "" );
}
const char* BELTrie::getTrieId() const 
{ 
    const char* s = globalPools.internalString_resolve( d_trieId_strId );
    return ( s ? s : "" );
}
BELTrie::~BELTrie( )
{
	delete d_wcPool;
	delete d_fcPool;
	delete d_tranPool;
	delete d_rewrPool;
}
BELTrie::BELTrie( GlobalPools& gp ) :
	globalPools(gp),
	d_rewrPool(0),
	d_wcPool(0),
	d_fcPool(0),
	d_tranPool(0),
	d_globalTriePoolId(0xffffffff),
	d_spellPriority(0),
	procs(*this)
{
	initPools();
}

void BELTrie::initPools() 
{
	d_varIndex.clear();

	if( d_rewrPool ) 
		delete d_rewrPool;
	d_rewrPool= new BarzelRewriterPool(1024);
	if( d_wcPool ) 
		delete d_wcPool;
	d_wcPool= new BarzelWildcardPool;
	if( d_fcPool ) 
		delete d_fcPool;
	d_fcPool= new BarzelFirmChildPool;
	if( d_tranPool ) 
		delete d_tranPool;
	d_tranPool= new BarzelTranslationPool;
}

std::ostream& BELTrie::printVariableName( std::ostream& fp, uint32_t varId ) const
{
	const BELSingleVarPath* bsvp = getVarIndex().getPathFromTranVarId( varId );
		
	const GlobalPools& gp = getGlobalPools();
	if( bsvp && bsvp->size() ) {
		BELSingleVarPath::const_iterator i = bsvp->begin(); 
		const char * s = gp.internalString_resolve( *i );

		if( s ) fp << s;
		for( ++i; i!= bsvp->end(); ++i ) {
			fp << ".";
			const char* s = gp.internalString_resolve( *i );
			if( s ) fp << s;
		}
	}
	return fp;
}
std::ostream& BELPrintContext::printRewriterByteCode( std::ostream& fp, const BarzelTranslation& t ) const
{
	BarzelRewriterPool::BufAndSize bas;
	if( trie.getRewriterPool().resolveTranslation( bas, t ) ) {
		return glob_printRewriterByteCode( fp, bas, *this );
	} else {
		AYDEBUG( "NO BYTE CODE" );
	}
	return fp;
}

const BarzelWCLookup*  BELPrintContext::getWildcardLookup( uint32_t id ) const
{
return( trie.getWCPool().getWCLookup( id ));
}

void BarzelTrieNode::clearFirmMap()
{
	d_firmMapId = 0xffffffff;
}
void BarzelTrieNode::clearWCMap()
{

	d_wcLookupId = 0xffffffff;
}
void BarzelTrieNode::clear()
{
	clearFirmMap();
	clearWCMap();
}
void BELTrie::clear()
{
	root.clear();
    macros.clear();
    procs.clear();
    d_linkedTranInfoMap.clear();
	initPools();
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

	const BarzelTranslation* trans = getTranslation(ctxt.trie);
	return ( trans ? trans->print( fp, ctxt ) : fp << "(null)" ) ;
}

std::ostream& BarzelTrieNode::print_firmChildren( std::ostream& fp, BELPrintContext& ctxt ) const
{
	const BarzelFCMap* fm= ( hasFirmChildren() ? ctxt.trie.getBarzelFCMap(*this) : 0 );
	if( !fm ) {
		return fp;
	}
	const BarzelFCMap& firmMap = *fm;
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
	const BarzelWCLookup* wcLookup = ctxt.getWildcardLookup( d_wcLookupId );
	if( !wcLookup ) {
		std::cerr << "Barzel Trie FATAL: lookup id " << std::hex << d_wcLookupId << " is invalid\n";
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
	
	trie.getWCPool().print( fp, wcKey, *this );
	if( !firmKey.isNull() ) {
		fp << "-->";
		firmKey.print( fp, *this );
	}
	return fp;
}


namespace {
	typedef std::pair< BTND_PatternDataVec::const_iterator, BarzelTrieFirmChildKey > WCPatDta;
	typedef std::list< WCPatDta > WCPatDtaList;


/// forms firm child key given a BTND_Pattern_XXX 
/// returns whether or not firmkey could be formed
struct BarzelTrieFirmChildKey_form : public boost::static_visitor<bool> {
	BELTrie& trie;
	BarzelTrieFirmChildKey& key ;
	BarzelTrieFirmChildKey_form( BarzelTrieFirmChildKey& x, BELTrie& tr ) : 
		trie(tr),
		key(x) 
	{ }

	template <typename T>
	bool operator()( const T& ) {
		key.type=BTND_Pattern_None_TYPE; // isNone will return true for this
		key.id=0xffffffff;
        return false;
	}
    /*
	bool operator()( const  BTND_Pattern_Wildcard& p ) 
	{
		key.type = (uint8_t)BTND_Pattern_Wildcard_TYPE;
		key.id = p.getType();	
        key.d_matchMode = p.d_matchMode;
        return true;
	}
    */
	bool operator()( const  BTND_Pattern_StopToken& p ) 
	{
		key.type = (uint8_t)BTND_Pattern_StopToken_TYPE;
		key.id = p.stringId;	
        key.d_matchMode = p.d_matchMode;
        return true;
	}
	bool operator()( const BTND_Pattern_Meaning& m ) {
		key.type = (uint8_t)BTND_Pattern_Meaning_TYPE;
		key.id = m.meaningId;	
        key.d_matchMode = m.d_matchMode;
        return true;
		}
	bool operator()( const BTND_Pattern_Token& p ) {
		key.type = (uint8_t)BTND_Pattern_Token_TYPE;
		key.id = p.stringId;	
        key.d_matchMode = p.d_matchMode;
        return true;
		}
	bool operator()( const BTND_Pattern_Punct& p ) {
		key.type = (uint8_t) BTND_Pattern_Punct_TYPE;
		key.id = p.theChar;
        key.d_matchMode = p.d_matchMode;
        return true;
		}
	bool operator()( const BTND_Pattern_CompoundedWord& p ) {
		key.type = (uint8_t) BTND_Pattern_CompoundedWord_TYPE;
		key.id = p.compWordId;
        key.d_matchMode = p.d_matchMode;
        return true;
	}
	bool operator()( const BTND_Pattern_DateTime& p ) {
		BarzelWCKey wcKey;
		trie.getWCPool().produceWCKey( wcKey, p );
		if( wcKey.wcType != BTND_Pattern_DateTime_TYPE ) {
			AYDEBUG( "TRIE PANIC" );
		}
		key.type=BTND_Pattern_DateTime_TYPE;
		key.id=wcKey.wcId;
        key.d_matchMode = p.d_matchMode;
        return false;
	}
	bool operator()( const BTND_Pattern_Date& p ) {
		switch( p.type ) {
		
		case BTND_Pattern_Date::T_ANY_DATE:
			key.type=BTND_Pattern_Date_TYPE; 
			key.id=0xffffffff;
			break;
		case BTND_Pattern_Date::T_ANY_FUTUREDATE:
		case BTND_Pattern_Date::T_ANY_PASTDATE:
		case BTND_Pattern_Date::T_DATERANGE: {
			BarzelWCKey wcKey;
			trie.getWCPool().produceWCKey( wcKey, p );
			if( wcKey.wcType != BTND_Pattern_Date_TYPE ) {
				AYDEBUG( "TRIE PANIC" );
			}
			key.type=BTND_Pattern_Date_TYPE;
			key.id=wcKey.wcId;
		}
			break;
		default:
			key.type=BTND_Pattern_None_TYPE; // isNone will return true for this
			key.id=0xffffffff;
			break;
		}
        key.d_matchMode = p.d_matchMode;
        return false;
	}
	bool operator()( const BTND_Pattern_Number& p ) {
        uint32_t id=0xffffffff;
        if( p.isTrivialInt(id) ) { // encoding trivial integer 
            key.type=BTND_Pattern_Number_TYPE;
            key.id=id;
            // key.setFirmNonLiteral(); 
            return true;
        } else {
            BarzelWCKey wcKey;
            trie.getWCPool().produceWCKey( wcKey, p );
            if( wcKey.wcType != BTND_Pattern_Number_TYPE ) { AYDEBUG( "TRIE PANIC" ); }

            key.type=BTND_Pattern_Number_TYPE;
            key.id=wcKey.wcId;
            return false;
        }
	}
	bool operator()( const BTND_Pattern_ERCExpr& p ) {
		BarzelWCKey wcKey;
		trie.getWCPool().produceWCKey( wcKey, p );
		if( wcKey.wcType != BTND_Pattern_ERCExpr_TYPE ) { AYDEBUG( "TRIE PANIC" ); }

		key.type = BTND_Pattern_ERCExpr_TYPE;
		key.id = wcKey.wcId;
        key.d_matchMode = p.d_matchMode;
        return false;
	}
	bool operator()( const BTND_Pattern_Entity& p ) {
		BarzelWCKey wcKey;
		trie.getWCPool().produceWCKey( wcKey, p );
		if( wcKey.wcType != BTND_Pattern_Entity_TYPE ) { AYDEBUG( "TRIE PANIC" ); }

		key.type=BTND_Pattern_Entity_TYPE;
		key.id=wcKey.wcId;
        key.d_matchMode = p.d_matchMode;
        return false;
	}
	bool operator()( const BTND_Pattern_ERC& p ) {
		BarzelWCKey wcKey;
		trie.getWCPool().produceWCKey( wcKey, p );
		if( wcKey.wcType != BTND_Pattern_ERC_TYPE ) { AYDEBUG( "TRIE PANIC" ); }

		key.type=BTND_Pattern_ERC_TYPE;
		key.id=wcKey.wcId;
        key.d_matchMode = p.d_matchMode;
        return false;
	}
	bool operator()( const BTND_Pattern_Range& p ) {
		BarzelWCKey wcKey;
		trie.getWCPool().produceWCKey( wcKey, p );
		if( wcKey.wcType != BTND_Pattern_Range_TYPE ) { AYDEBUG( "TRIE PANIC" ); }

		key.type=BTND_Pattern_Range_TYPE;
		key.id=wcKey.wcId;
        key.d_matchMode = p.d_matchMode;
        return false;
	}
}; // BarzelTrieFirmChildKey_form

} // anon namespace ends 

/// barzel TRIE node methods
BarzelTrieNode* BarzelTrieNode::addFirmPattern( BELTrie& trie, const BarzelTrieFirmChildKey& key )
{
	if( key.isNull() ) {
		AYTRACE( "illegally attempting to add a wildcard" );
		return this;
	} else {
		
		BarzelFCMap* fm = ( hasFirmChildren() ? 
			trie.getBarzelFCMap(*this) : 
			trie.makeNewBarzelFCMap(d_firmMapId) 
		);
		BarzelFCMap& firmMap = *fm;
		BarzelFCMap::iterator i = firmMap.find( key );
		if( i == firmMap.end() ) { // creating new one
			i = firmMap.insert( BarzelFCMap::value_type( key, BarzelTrieNode(this) ) ).first;
		}
		return &(i->second);
	}
}


BarzelTrieNode* BarzelTrieNode::addWildcardPattern( BELTrie& trie, const BTND_PatternData& p, const BarzelTrieFirmChildKey& fk  )
{
	if( !hasValidWcLookup() ) 
		trie.getWCPool().addWCLookup( d_wcLookupId );
	
	BarzelWCLookup* wcLookup = trie.getWCPool().getWCLookup( d_wcLookupId );
	BarzelWCLookupKey lkey;
	lkey.first = fk;
	trie.produceWCKey( lkey.second, p );
	BarzelWCLookup::iterator i = wcLookup->find( lkey );	
	if( i == wcLookup->end() ) 
		i = wcLookup->insert( BarzelWCLookup::value_type( lkey, BarzelTrieNode(this)  ) ).first;	
	i->second.setFlag_isWcChild();	

	return &(i->second);
}
//// BarzelTrieFirmChildKey
std::ostream& BarzelTrieFirmChildKey::print( std::ostream& fp ) const
{
	return( fp << BTNDDecode::typeName_Pattern( type )  << ":" << std::hex << id << (noLeftBlanks? "+":"") );
}

std::ostream& BarzelTrieFirmChildKey::print( std::ostream& fp, const BELPrintContext& ctxt ) const
{
	return( print( fp ) << "[" << ctxt.printableString(id) << "]" );
}


//// bel TRIE methods
void BELTrie::produceWCKey( BarzelWCKey& key, const BTND_PatternData& btnd  )
{
	d_wcPool->produceWCKey(key, btnd );
}
void BELTrie::setTanslationTraceInfo( BarzelTranslation& tran, const BELStatementParsed& stmt, uint32_t emitterSeqNo )
{
	tran.traceInfo.source = stmt.getSourceNameStrId();
	tran.traceInfo.statementNum = stmt.getStmtNumber();
	tran.traceInfo.emitterSeqNo = emitterSeqNo;
}

const BarzelTrieNode* BELTrie::addPath( 
	const BELStatementParsed& stmt, 
	const BTND_PatternDataVec& path, 
	uint32_t transId, 
	const BELVarInfo& varInfo,
	uint32_t emitterSeqNo )
{
	BarzelTrieNode* n = &root;

	//AYLOG(DEBUG) << "Adding new path (" << path.size() << " elements)";

	// forming the list of wildcard pattern data. data includes iterator to the actual
	// wildcard as well as the next firm match key (if any)
	WCPatDtaList wcpdList;
	WCPatDtaList::iterator firstWC = wcpdList.end();

	BarzelTrieFirmChildKey firmKey;
	BarzelTrieFirmChildKey_form keyFormer( firmKey, *this ) ;
    
	for( BTND_PatternDataVec::const_iterator i = path.begin(); i!= path.end(); ++i ) {
		//BarzelTrieFirmChildKey shitKey(*i);
		bool isFirm = boost::apply_visitor( keyFormer, *i );

		firmKey.noLeftBlanks = 0;
		//if( !(shitKey == firmKey) ) {
			//std::cerr << "SHIT FUCK different keys formed\n";
		//}
		if( firmKey.isNull() || (!isFirm && path.begin() == i) ) { // either failed to encode firm key or this is a leading wc
			wcpdList.push_back( WCPatDta(i,BarzelTrieFirmChildKey() ) );
            // ACHTUNG! was local
			firstWC = wcpdList.rbegin().base();
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

	BELVarInfo::const_iterator vi=varInfo.begin(); 
	BarzelSingleTranVarInfo* storedTranVarInfo = 0;

	for( BTND_PatternDataVec::const_iterator i = path.begin(); i!= path.end(); ++i, ++vi ) {
		const BarzelTrieNode* prevN = n;
		if( !wcpdList.empty() && i == wcpdList.front().first ) { // we reached a wildcard
			if( n ) 
				n = n->addWildcardPattern( *this, *i, wcpdList.front().second );
			else {
				AYTRACE("addWildcardPattern returned NULL") ;
				return 0; // this is impossible
			}
			wcpdList.pop_front(); // now first element points to the next wildcard (if any are left)
		} else { // reached a firm token
			if( n ) {
				/// this visitor updates firmKey (declared at the top of this function)
				boost::apply_visitor( keyFormer, *i );
				n = n->addFirmPattern( *this, firmKey );
			} else {
				AYTRACE("addFirmPattern returned NULL") ;
				return 0; // this is impossible
			}
		}
		/// n is the new trienode 
		/// trying to create variableinfo link to it
		/// transid,varInfo --> n
		if( vi->size() ) { // it's a vector of stringIds corresponding to context names
			if( !storedTranVarInfo ) 
				storedTranVarInfo = d_varIndex.produceBarzelSingleTranVarInfo( transId );		
			// storedTranVarInfo is guaranteed not to be 0 here
			
			storedTranVarInfo->insert( BarzelSingleTranVarInfo::value_type( *vi,  prevN) );
		}
	}
	if( n ) {
		if( n->hasValidTranslation() && n->getTranslationId() != transId ) {
			if( !tryAddingTranslation(n,transId,stmt,emitterSeqNo) ) {
				//std::cerr << "\nBARZEL TRANSLATION CLASH:" << stmt.getSourceName() << ":" << stmt.getStmtNumber() << "\n";
			}
		} else
			n->setTranslation( transId );
	} else 
		AYTRACE("inconsistent state for setTranslation");
	return n;
}

std::ostream& BELTrie::printTanslationTraceInfo( std::ostream& fp, const BarzelTranslationTraceInfo& traceInfo ) const
{
	return globalPools.printTanslationTraceInfo( fp, traceInfo );
}
bool BELTrie::tryAddingTranslation( BarzelTrieNode* n, uint32_t id, const BELStatementParsed& stmt, uint32_t emitterSeqNo )
{
	BarzelTranslation* tran = getBarzelTranslation(*n);
	if( tran ) {
		EntityGroup* entGrp = 0;
		switch( tran->getType() ) {
		case BarzelTranslation::T_MKENT: {
			uint32_t entId = tran->getId_uint32();
			uint32_t grpId = 0;
			entGrp = getEntityCollection().addEntGroup( grpId );
			tran->setMkEntList( grpId );
			entGrp->addEntity( entId );
		}
			break;
		case BarzelTranslation::T_MKENTLIST:
			entGrp = getEntityCollection().getEntGroup( tran->getId_uint32() );
			break;
		default: { // CLASH 
            if( !(stmt.getSourceNameStrId()== tran->traceInfo.source && stmt.getStmtNumber() == tran->traceInfo.statementNum) ) {
                std::ostream& os = stmt.getErrStream();
			    os << "<error type=\"RULE CLASH\"> <rule>" << stmt.getSourceName() << ':' << stmt.getStmtNumber()  << '.' << emitterSeqNo <<
			    " </rule><rule> " ;
			    printTanslationTraceInfo( stmt.getErrStream(), tran->traceInfo ) << "</rule></error>\n";
		    }	

		}
			return false;
		}
		if( entGrp ) {
			const BarzelTranslation* newTran = d_tranPool->getObjById(id);
            
			if( newTran && newTran->isMkEntSingle() ) {
				uint32_t newEntId = newTran->getId_uint32();

				entGrp->addEntity( newEntId );
                if( tran && !tran->traceInfo.sameStatement(newTran->traceInfo) )
                    linkTraceInfo( tran->traceInfo, newTran->traceInfo );
				return true;
			} else {
				// std::cerr <<"\n" <<  __FILE__ << ":" << __LINE__ << "translation clash: " << " types: " << 
				// (size_t)( newTran ? newTran->getType() : -1 ) << " vs " << (size_t)(tran->getType())  << std::endl;
                if( !(stmt.getSourceNameStrId()== tran->traceInfo.source && stmt.getStmtNumber() == tran->traceInfo.statementNum) ) {
                    std::ostream& os = stmt.getErrStream();
                    os << "<error type=\"RULE CLASH\"> <rule>" << stmt.getSourceName() << ':' << stmt.getStmtNumber()  << '.' << emitterSeqNo <<
                    " </rule><rule> " ;
                    printTanslationTraceInfo( stmt.getErrStream(), tran->traceInfo ) << "</rule></error>\n";
                }	
                
				return false;
			}
		} else {
			AYDEBUG("mkent inconsistency");
			return false;
		}
	}
	else  // this should never be the case 
		return false;
	
}
//// BarzelTranslation methods

bool BarzelTranslation::isRewriteFallible( const BarzelRewriterPool& pool ) const
{
	BarzelRewriterPool::BufAndSize rwr; // rewrite bytecode
	if( !pool.resolveTranslation( rwr, *this ) ) {
		AYLOG(ERROR) << "failed to retrieve rewrite\n";
		return false; // 
	} else {
		/// here we need to evaluate rewrite for fallibility (do NOT confuse with rewrite evaluation
		/// which would ned the matched data. for now we assume that nothing is fallible
		BarzelRewriterPool::BufAndSize bas;
		if( pool.resolveTranslation( bas, *this ))
			return pool.isRewriteFallible( bas );
		
		return false;
	}
}

void BarzelTranslation::set(BELTrie& ,const BTND_Rewrite_Variable& x )
{
	switch( x.getIdMode() ) {
	case BTND_Rewrite_Variable::MODE_WC_NUMBER:
		type = T_VAR_WC_NUM;
		id =  x.getVarId();
		return;
	case BTND_Rewrite_Variable::MODE_VARNAME:
		type = T_VAR_NAME;
		id =  x.getVarId();
		return;
	case BTND_Rewrite_Variable::MODE_PATEL_NUMBER:
		type = T_VAR_EL_NUM;
		id =  x.getVarId();
		return;
	case BTND_Rewrite_Variable::MODE_WCGAP_NUMBER:
		type = T_VAR_GN_NUM;
		id =  x.getVarId();
		return;
	case BTND_Rewrite_Variable::MODE_REQUEST_VAR:
		type = T_REQUEST_VAR_NAME;
		id =  x.getVarId();
		return;
	default: 
		AYTRACE( "inconsistent literal encountered in Barzel rewrite" );
		setStop(); // this should never happen
		break;
	}
}

void BarzelTranslation::set(BELTrie& ,const BTND_Rewrite_Literal& x )
{
	//AYLOGDEBUG(x.getType());
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
		id = x.getId();
		break;
	case BTND_Rewrite_Literal::T_BLANK:
		type = T_BLANK;
		id = 0xffffffff;
		break;
	case BTND_Rewrite_Literal::T_PUNCT:
		type = T_PUNCT;
		id = x.getId();
		break;
	default: 
		AYTRACE( "inconsistent literal encountered in Barzel rewrite" );
		setStop(); // this should never happen
		break;
	}
}
void BarzelTranslation::set(BELTrie&, const BTND_Rewrite_Function& x )
{
	type = T_FUNCTION;
	id = x.getNameId();
    argStrId = x.getArgStrId();
}
void BarzelTranslation::set(BELTrie&, const BTND_Rewrite_MkEnt& x )
{
	if( x.isSingleEnt() ) 
		type = T_MKENT;
	else 
		type = T_MKENTLIST;

	id   = x.getRawId();
}
void BarzelTranslation::set(BELTrie&, const BTND_Rewrite_Number& x )
{
	if( x.isConst )  {
		if( x.isReal() ) {
			type = T_NUMBER_REAL;
			id = (float)(x.getReal());
		} else {
			type = T_NUMBER_INT;
			id = (int64_t)x.getInt();
		}
	} else {
		AYTRACE( "inconsistent number encountered in Barzel rewrite" );
		setStop();
	}
}

namespace {

struct BarzelTranslation_set_visitor : public boost::static_visitor<int> {
	BarzelTranslation& d_tran;
	BELTrie& d_trie;

	BarzelTranslation_set_visitor( BELTrie& trie, BarzelTranslation& tran ) : d_tran(tran), d_trie(trie) {}
	
	template <typename T>
	int operator() ( const T&x ) 
		{ return d_tran.setBtnd( d_trie, x ); }
};

} // anon namespace ends 

int BarzelTranslation::encodeIt( BarzelRewriterPool::byte_vec& enc, BELTrie& trie, const BELParseTreeNode& tn ) 
{
	const BTND_RewriteData* rwrDta = tn.getTrivialRewriteData();    
	if( rwrDta ) {
		BarzelTranslation_set_visitor vis( trie, *this  );
		boost::apply_visitor( vis, *rwrDta );
		return ENCOD_TRIVIAL;
	} else { // non trivial rewrite 
		if( !BarzelRewriterPool::encodeParseTreeNode(enc,tn) ) {
			if( !enc.size() ) {
				AYTRACE( "inconsistent encoding produced");
				return ENCOD_FAILED;
			} else
				return ENCOD_REWRITER;
		} 
	}
	return ENCOD_FAILED;
}

int BarzelTranslation::set( BELTrie& trie, const BELParseTreeNode& tn ) 
{
	/// diagnosing trivial rewrite usually root will have one childless child
	/// that childless child may be a trivial transform 
	/// theoretically root itself may contain the transform (if we process attribs)
	const BTND_RewriteData* rwrDta = tn.getTrivialRewriteData();
	if( rwrDta ) {
		BarzelTranslation_set_visitor vis( trie, *this  );
		return boost::apply_visitor( vis, *rwrDta );
	} else { // non trivial rewrite 
		if( trie.getRewriterPool().produceTranslation( *this, tn ) != BarzelRewriterPool::ERR_OK ) {
			AYTRACE("Failed to produce valid rewrite translation" );
			setStop();
            return 1;
		} else 
            return 0;
	}
}
void BarzelTranslation::fillRewriteData( BTND_RewriteData& d ) const
{
	switch(type) {
	case T_NONE: d = BTND_Rewrite_None(); return;

	case T_STOP: { BTND_Rewrite_Literal l; l.setStop(); l.setId(getId_uint32()); d = l; } return;
	case T_STRING: { BTND_Rewrite_Literal l; l.setString(getId_uint32()); d = l; } return;
	case T_BLANK: { BTND_Rewrite_Literal l; l.setBlank(); d = l; } return;
	case T_PUNCT: { BTND_Rewrite_Literal l; l.setPunct(getId_uint32()); d = l; } return;
	case T_COMPWORD: { BTND_Rewrite_Literal l; l.setCompound(getId_uint32()); d = l; } return;
	case T_NUMBER_INT: { BTND_Rewrite_Number n; n.set(getId_int()); d=n;} return;
	case T_NUMBER_REAL: { BTND_Rewrite_Number n; n.set(getId_double()); d=n;} return;

	case T_VAR_NAME: { BTND_Rewrite_Variable n; n.setVarId(getId_uint32()); d=n; } return;
	case T_VAR_WC_NUM: { BTND_Rewrite_Variable n; n.setWildcardNumber(getId_uint32()); d=n; } return;
	case T_VAR_EL_NUM: { BTND_Rewrite_Variable n; n.setPatternElemNumber(getId_uint32()); d=n; } return;
	case T_VAR_GN_NUM: { BTND_Rewrite_Variable n; n.setWildcardGapNumber(getId_uint32()); d=n; } return;
	case T_MKENT: { BTND_Rewrite_MkEnt n; n.setEntId( getId_uint32() ); d=n; } return;
	case T_MKENTLIST: { BTND_Rewrite_MkEnt n; n.setEntGroupId( getId_uint32() ); d=n; } return;
	case T_FUNCTION: { BTND_Rewrite_Function n; n.setNameId( getId_uint32() ); n.setArgStrId( getArgStrId() ); d=n; } return;
	case T_REQUEST_VAR_NAME: { BTND_Rewrite_Variable n; n.setRequestVarId(getId_uint32()); d=n; } return;

	default: d = BTND_Rewrite_None(); return;
	}
}
std::ostream& BarzelTranslation::print( std::ostream& fp, const BELPrintContext& ctxt) const
{
#define CASEPRINT(x) case T_##x: return( fp << #x );
	switch(type ) {
	CASEPRINT(NONE)
	CASEPRINT(STOP)
	CASEPRINT(STRING)
	CASEPRINT(COMPWORD)
	CASEPRINT(NUMBER_INT)
	CASEPRINT(NUMBER_REAL)

	case T_REWRITER:
	{
		BarzelRewriterPool::BufAndSize bas; 
		ctxt.printRewriterByteCode( fp << "RWR{", *this ) << "}";
	}
		return fp;
	CASEPRINT(BLANK)
	CASEPRINT(PUNCT)
	CASEPRINT(VAR_NAME)
	CASEPRINT(VAR_WC_NUM)
	CASEPRINT(VAR_EL_NUM)

	default:
		return ( fp << "unknown translation" );
	}
#undef CASEPRINT
}

} // end namespace barzer
