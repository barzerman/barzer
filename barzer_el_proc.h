#ifndef BARZER_EL_PROC_H
#define BARZER_EL_PROC_H

#include <map>
#include <barzer_el_rewriter.h>

/// barzel stored procedures . BarzelProcs stores procedures unique per trie 
namespace barzer {

class BELTrie;
struct BELParseTreeNode;
class BarzelEvalResult;
struct BarzelEvalContext;

class BarzelProcs {
	typedef std::map< uint32_t, BarzelEvalNode > InternalStrIdToEvalNodeMap;
	InternalStrIdToEvalNodeMap d_evalNodeMap;
	BELTrie& d_trie;
public:
	BarzelProcs( BELTrie& trie ) : d_trie(trie) {}
	inline BarzelEvalNode& produceNewEvalNode( bool& wasNew, uint32_t strId )
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
	inline const BarzelEvalNode* getEvalNode( uint32_t strId ) const
	{ 
		InternalStrIdToEvalNodeMap::const_iterator i = d_evalNodeMap.find( strId );
		return( i == d_evalNodeMap.end() ? 0 : &(i->second) );
	}
	inline BarzelEvalNode* getEvalNode( uint32_t strId ) 
	{
		return const_cast<BarzelEvalNode*>( 
			const_cast<const BarzelProcs*>(this)->getEvalNode( strId )
		);
	}

	enum {
		ERR_OK, // no error

		// UNUSED error for now - future
		ERR_LINK, // "linkage" failed - there were un-resolved references 
			      // this error may be ignored but then there will be runtime errors

		ERR_DUP, // attempt to define the same function twice (duplicate)
		ERR_INTERNAL, // internal error occured 
		ERR_TRIVIAL, // trivial rewrites are ignored 

		ERR_CALL_BADNAME, // attempted to call a non existing proc
		ERR_CALL_FAIL     // existing proc call failed
	};
	// returns one of the ERR_XXX constants (ERR_OK or 0 when new stored proc was created 
	// and no errors encountered)
	int generateStoredProc( uint32_t nameStrId, const BELParseTreeNode& );
	int callStoredProc( uint32_t nameStrId, BarzelEvalResult& transResult, BarzelEvalContext& ctxt, const BarzelEvalResultVec& inputArgs );
    void clear() { d_evalNodeMap.clear(); }
};

} // barzer namespace ends 

#endif // BARZER_EL_PROC_H
