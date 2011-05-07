#ifndef BARZER_EL_VARIABLE_H
#define BARZER_EL_VARIABLE_H
#include <stdint.h>
#include <vector>
#include <map>
#include <ay/ay_pool_with_id.h>

namespace barzer {
class BarzelTrieNode;
typedef std::vector<uint32_t> BELSingleVarPath;
typedef std::vector<BELSingleVarPath> BELVarInfo;


/// variable info for a single translation
typedef std::multimap< BELSingleVarPath, const BarzelTrieNode* > BarzelSingleTranVarInfo;

class BarzelVariableIndex {
	typedef std::map< uint32_t, BarzelSingleTranVarInfo > TranIdToVarInfoMap;
	TranIdToVarInfoMap d_tranIdMap;
	
	ay::InternerWithId<BELSingleVarPath> d_pathInterner;
public:

	const BarzelSingleTranVarInfo* getBarzelSingleTranVarInfo( uint32_t tranId ) const
	{
		TranIdToVarInfoMap::const_iterator i = d_tranIdMap.find( tranId );
		return( i == d_tranIdMap.end() ? 0 : &(i->second) );
	}

	BarzelSingleTranVarInfo* produceBarzelSingleTranVarInfo( uint32_t tranId )
	{
		BarzelSingleTranVarInfo* varInfo = const_cast<BarzelSingleTranVarInfo*>( getBarzelSingleTranVarInfo(tranId) );
		if( !varInfo ) { // need to make a new one
			std::pair< TranIdToVarInfoMap::iterator, bool> insResult = d_tranIdMap.insert( 
				TranIdToVarInfoMap::value_type(tranId,BarzelSingleTranVarInfo()) );
			varInfo = &(insResult.first->second);
		}
		return varInfo;
	}
	uint32_t produceVarIdFromPathForTran( const BELSingleVarPath& p )
	{
		return d_pathInterner.produceIdByObj( p );
	}
	const BELSingleVarPath* getPathFromTranVarId( uint32_t tvid ) const
	{
		return d_pathInterner.getObjById(tvid);
	}
};

}
#endif // BARZER_EL_VARIABLE_H
