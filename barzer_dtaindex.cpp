#include <barzer_dtaindex.h>
#include <boost/pool/singleton_pool.hpp>
#include <ay/ay_slogrovector.h>

namespace barzer {

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

const StoredToken* DtaIndex::getStoredToken( const char* ) const
{
	return 0;
}
const StoredToken* DtaIndex::getStoredTokenById( StoredTokenId ) const
{
	return 0;
}

DtaIndex::~DtaIndex()
{
}


} // namespace barzer
