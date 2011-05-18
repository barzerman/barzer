#include <barzer_universe.h>

namespace barzer {

///////////////// generic entities 
namespace {

struct GenericEntData {
	uint16_t cl, subcl;
	const char* id;
	const char* name;
	
	GenericEntData( uint16_t sc, const char* n ) : 
		cl(1), subcl(sc) , id(0), name(n)
	{}
	GenericEntData( uint16_t sc, const char* idStr, const char* n ) : 
		cl(1), subcl(sc) , id(idStr), name(n)
	{}
};

static GenericEntData g_genDta[] = {
	GenericEntData(1,"price"), // price 
	GenericEntData(2,"length"), // length
	GenericEntData(3,"weight"), // weight
	GenericEntData(4,"age"), // weight
	GenericEntData(4,"area"), // weight
	GenericEntData(4,"volume"), // weight
	// can't take this shit!
	GenericEntData(5,"shit", "some shit"), // weight
	// Astronomical units
	GenericEntData(1,"USD", "US dollar"),
	GenericEntData(1,"EUR", "Euro euro!"),
	GenericEntData(1,"JPY", "Japanese yen"),
	GenericEntData(1,"CNY", "Renminbi"),
	GenericEntData(1,"GBP", "Pound sterling"),
	GenericEntData(1,"RUB", "Рупь")
};
} // anon namespace ends

GlobalPools::~GlobalPools() 
{
	for( UniverseMap::iterator i =  d_uniMap.begin(); i!= d_uniMap.end(); ++i ) {
		if( i->second ) 
			delete i->second;
	}
}
GlobalPools::GlobalPools() :
	dtaIdx( &stringPool),
	barzelRewritePool(64*1024),
	funSt(*this),
	dateLookup(*this),
	globalTriePool( 
			&barzelRewritePool,
			&barzelWildcardPool,
			&barzelFirmChildPool,
			&barzelTranslationPool 
		),
	settings(*this)
{
	createGenericEntities();

	/// create default universe 
	produceUniverse(DEFAULT_UNIVERSE_ID);
}

void GlobalPools::createGenericEntities()
{
	for( size_t i=0; i< ARR_SZ( g_genDta ); ++i ) {
		const GenericEntData& gd = g_genDta[i];
		if( gd.id ) {
			dtaIdx.addGenericEntity( gd.id, gd.cl, gd.subcl );
		} else {
			dtaIdx.addGenericEntity( gd.cl, gd.subcl );
		}
	}

	dtaIdx.addGenericEntity("wine", 2, 1);
}

StoredUniverse::StoredUniverse(GlobalPools& g) :
	gp(g),
	trieCluster(g.globalTriePool),
	trieClusterIter(trieCluster) /*,
	settings(*this) //*/
{
}

void StoredUniverse::clear()
{
	getBarzelTrie().clear();
	getDtaIdx().clear();
	getWildcardPool().clear();
	getRewriterPool().clear();
	gp.stringPool.clear();
}
std::ostream& StoredUniverse::printBarzelTrie( std::ostream& fp, const BELPrintFormat& fmt ) const
{
	BELPrintContext ctxt( getBarzelTrie(), gp.stringPool, fmt );
	return getBarzelTrie().print( fp, ctxt );
}
std::ostream& StoredUniverse::printBarzelTrie( std::ostream& fp ) const
{
	BELPrintFormat fmt;
	BELPrintContext ctxt( getBarzelTrie(), gp.stringPool, fmt );
	return getBarzelTrie().print( fp, ctxt );
}

const char* StoredUniverse::getGenericSubclassName( uint16_t subcl ) const
{
	if( subcl< ARR_SZ(g_genDta) ) 
		return g_genDta[subcl].name;
	else 
		return "<unknown>";
}


//// end of generic entities 

} // namespace barzer ends
