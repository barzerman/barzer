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

	GenericEntData(2,"length"), // length-width-height-distance (cm,inch,m,foot,km,mile,kkm,kmile)
	GenericEntData(3,"weight"), // weight in 
	GenericEntData(4,"age"), // age in years, months, days, hours, minutes
	GenericEntData(5,"area"), // square area
	GenericEntData(6,"volume"), // cubic volume
	GenericEntData(7,"wearsize"), // clothing/shoe sizes
	GenericEntData(8,"time"), // time

	// can't take this shit!
	GenericEntData(69,"shit", "some shit"), // weight
	// Astronomical units

	/// currencies 
	GenericEntData(1,"USD", "US dollar"),
	GenericEntData(1,"EUR", "Euro euro!"),
	GenericEntData(1,"JPY", "Japanese yen"),
	GenericEntData(1,"CNY", "Renminbi"),
	GenericEntData(1,"GBP", "Pound sterling"),
	GenericEntData(1,"RUB", "Рупь")
};
} // anon namespace ends

GlobalTriePool::ClassTrieMap& GlobalTriePool::produceTrieMap( const std::string& trieClass ) {
	return d_trieMap[trieClass]; // XAXAXA
}



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
			*this,
			&barzelRewritePool,
			&barzelWildcardPool,
			&barzelFirmChildPool,
			&barzelTranslationPool 
		),
	settings(*this),
	d_isAnalyticalMode(false),
	d_maxAnalyticalModeMaxSeqLength(3)
{
	/// create default universe 
	produceUniverse(DEFAULT_UNIVERSE_ID);
	
	createGenericEntities();

}

std::ostream& GlobalPools::printTanslationTraceInfo( std::ostream& fp, const BarzelTranslationTraceInfo& traceInfo ) const
{
	const char* srcName = internalString_resolve( traceInfo.source );
	
	return ( fp << ( srcName ? srcName : "(null)" ) << ':' << traceInfo.statementNum << '.' <<
	traceInfo.emitterSeqNo );
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
	trieCluster(g.globalTriePool)
{
}

void StoredUniverse::clear()
{
	// getBarzelTrie().clear();
	getDtaIdx().clear();
	// getWildcardPool().clear();
	// getRewriterPool().clear();
	// gp.stringPool.clear();
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
