#include <barzer_dtaindex.h>
#include <boost/pool/singleton_pool.hpp>
#include <ay/ay_slogrovector.h>

namespace barzer {

StoredEntity& addOneEntity( bool& madeNew, const StoredEntityUniqId& uniqId )
{
	std::pair< UniqIdToEntIdMap::iterator, bool > insPair = 
		euidMap.insert( 	
			UniqIdToEntIdMap::value_type(
				uniqId, INVALID_STORED_ID
			)
		);
	madeNew = insPair.second;
	if( !madeNew ) { // this is not a new uniqId
		StoredEntityId entId =insPair->second;

		return storEnt.vec[ entId ];
	}
		
	StoredEntityId entId = storEnt.vec.size();
	*(insPair->second) = entId; // update euid --> entid map value with the newly generated entid
	
	StoredEntity& e = storEnt.extend(); 
	e.setAll( entId, uniqId );
	return e;
}
StoredToken& StoredTokenPool::addSingleTok( bool& newAdded, const char* t)
{
	newAdded = false;
	StoredToken* sTok = getTokByString(t);
	if( sTok ) 
		return *sTok;
	
	StoredTokenId sid = strPool->internIt( t );
	const char* internedStr = strPool->resolveId( sid );

	StoredTokenId tokId = storTok.vec.size();
	StoredToken& newTok = storTok.extend();
	newTok.setSingle( tokId, sid, strlen(internedStr) );

	singleTokMap.insert( 
		std::make_pair(
			internedStr, &newTok
		)
	);
	newAdded = true;
	return newTok;
}


void DtaIndex::clear()
{
	tokPool.clear();
	entPool.clear();
}

DtaIndex::DtaIndex(ay::UniqueCharPool* sPool) :
	strPool(sPool),
	tokPool(strPool),
	cwTree(0),
	semFSA(0)
{
}
DtaIndex::~DtaIndex()
{
}


} // namespace barzer
