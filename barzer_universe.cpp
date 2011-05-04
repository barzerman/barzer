#include <barzer_universe.h>

namespace barzer {

StoredUniverse::StoredUniverse() :
	dtaIdx(&stringPool),
	barzelRewritePool(64*1024),
	barzelTrie(&barzelRewritePool,&barzelWildcardPool,&barzelFirmChildPool),
	funSt(*this),
	dateLookup(*this)
{

}

void StoredUniverse::clear()
{
	barzelTrie.clear();
	dtaIdx.clear();
	barzelWildcardPool.clear();
	barzelRewritePool.clear();
	stringPool.clear();
}
std::ostream& StoredUniverse::printBarzelTrie( std::ostream& fp, const BELPrintFormat& fmt ) const
{
	BELPrintContext ctxt( barzelTrie, stringPool, fmt );
	return barzelTrie.print( fp, ctxt );
}
std::ostream& StoredUniverse::printBarzelTrie( std::ostream& fp ) const
{
	BELPrintFormat fmt;
	BELPrintContext ctxt( barzelTrie, stringPool, fmt );
	return barzelTrie.print( fp, ctxt );
}


} // namespace barzer ends
