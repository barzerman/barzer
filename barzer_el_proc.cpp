#include <barzer_el_proc.h>
#include <barzer_el_trie.h>
#include <barzer_el_matcher.h>
#include <barzer_el_eval.h>

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

	BarzelEvalNode* BarzelProcs::getEvalNode( uint32_t strId ) 
	{
		return const_cast<BarzelEvalNode*>( 
			const_cast<const BarzelProcs*>(this)->getEvalNode( strId )
		);
	}
	const BarzelEvalNode* BarzelProcs::getEvalNode( uint32_t strId ) const
	{ 
		InternalStrIdToEvalNodeMap::const_iterator i = d_evalNodeMap.find( strId );
		return( i == d_evalNodeMap.end() ? 0 : &(i->second) );
	}
	BarzelEvalNode& BarzelProcs::produceNewEvalNode( bool& wasNew, uint32_t strId )
	{
		InternalStrIdToEvalNodeMap::iterator i = d_evalNodeMap.find( strId );
		if( i != d_evalNodeMap.end() ) 
			return ( wasNew= false, i->second );
		else {	
			return (
				wasNew= true,
				d_evalNodeMap.insert( InternalStrIdToEvalNodeMap::value_type(
					strId, BarzelEvalNode()
				)).first->second
			);
		}
	}
} // barzer namespace ends
