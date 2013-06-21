
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#pragma once
#include <map>
#include <barzer_el_rewriter.h>

/// barzel stored procedures . BarzelProcs stores procedures unique per trie 
namespace barzer {

class BELTrie;
struct BELParseTreeNode;
class BarzelEvalResult;
class BarzelEvalNode;
class BarzelEvalContext;

class BarzelProcs {
	typedef std::map< uint32_t, BarzelEvalNode > InternalStrIdToEvalNodeMap;
	InternalStrIdToEvalNodeMap d_evalNodeMap;
	BELTrie& d_trie;
public:
	BarzelProcs( BELTrie& trie ) : d_trie(trie) {}
	BarzelEvalNode& produceNewEvalNode( bool& wasNew, uint32_t strId );
	const BarzelEvalNode* getEvalNode( uint32_t strId ) const;
	BarzelEvalNode* getEvalNode( uint32_t strId ) ;

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
