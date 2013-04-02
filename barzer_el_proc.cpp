#include <barzer_el_proc.h>
#include <barzer_el_trie.h>
#include <barzer_el_matcher.h>

namespace barzer {


int BarzelProcs::callStoredProc( uint32_t nameStrId, BarzelEvalResult& transResult, BarzelEvalContext& ctxt,const BarzelEvalResultVec& inputArgs )
{
	const BarzelEvalNode* evalNode = getEvalNode( nameStrId );
	if( !evalNode ) 
		return ERR_CALL_BADNAME;
	
	return( evalNode->eval(transResult, ctxt) ? ERR_CALL_FAIL : ERR_OK );
}
int BarzelProcs::generateStoredProc( uint32_t nameStrId, const BELParseTreeNode& ptn )
{
	bool wasNew = false;
	// trying to add future "precompild" node to the map
	BarzelEvalNode& ben = produceNewEvalNode( wasNew, nameStrId );
	if( !wasNew ) 
		return ERR_DUP;

    if( BarzelRewriterPool::encodeParseTreeNode( ben, ptn ) != BarzelRewriterPool::ERR_OK ) {
        AYLOG(ERROR) << "encoder inconsistency\n";
        return ERR_INTERNAL;
    }
	
	return ERR_OK;
}

} // barzer namespace ends
