#include <barzer_dtaindex.h>
#include <boost/pool/singleton_pool.hpp>
#include <ay/ay_slogrovector.h>

namespace barzer {


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
