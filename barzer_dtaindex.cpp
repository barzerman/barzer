#include <barzer_dtaindex.h>
//#include <boost/pool/singleton_pool.hpp>
#include <ay/ay_slogrovector.h>
#include <barzer_loader_xml.h>

namespace barzer {

StoredEntity& StoredEntityPool::addOneEntity( bool& madeNew, const StoredEntityUniqId& uniqId )
{
	std::pair< UniqIdToEntIdMap::iterator, bool > insPair = 
		euidMap.insert( 	
			UniqIdToEntIdMap::value_type(
				uniqId, INVALID_STORED_ID
			)
		);
	madeNew = insPair.second;
	if( !madeNew ) { // this is not a new uniqId
		StoredEntityId entId =insPair.first->second;

		return storEnt.vec[ entId ];
	}
		
	StoredEntityId entId = storEnt.vec.size();
	(insPair.first->second) = entId; // update euid --> entid map value with the newly generated entid
	
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

void DtaIndex::addTokenToEntity(
	StoredToken& tok,
	StoredEntity& ent, 
	const EntTokenOrderInfo& ord, 
	const TokenEntityLinkInfo& teli,
	bool unique )
{
	if( ord.nameId || !unique || !ent.hasTokenLinSrch(tok.tokId ) ) {
		tok.addEntity( ent.entId, teli );	
		/// adding token to entity 
		ent.addToken( tok.tokId, ord );
	}
}

void DtaIndex::addTokenToEntity( 
	const char* str, StoredEntity& ent, 
	const EntTokenOrderInfo& ord, const TokenEntityLinkInfo& teli, bool unique )
{
	bool newAdded= false;
	StoredToken& tok = tokPool.addSingleTok( newAdded, str );
	addTokenToEntity(tok,ent,ord,teli,unique);
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
int DtaIndex::loadEntities_XML( const char* fileName )
{
	EntityLoader_XML xmlLoader(this);
	return xmlLoader.readFile( fileName );
}


} // namespace barzer
