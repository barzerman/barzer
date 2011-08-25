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

GlobalTriePool::ClassTrieMap& GlobalTriePool::produceTrieMap( const std::string& trieClass ) 
{
	return d_trieMap[trieClass]; 
}



GlobalPools::~GlobalPools() 
{
	for( UniverseMap::iterator i =  d_uniMap.begin(); i!= d_uniMap.end(); ++i ) {
		if( i->second ) 
			delete i->second;
	}
}

BELTrie* GlobalTriePool::mkNewTrie() 
{
	return new BELTrie( d_gp);
}

GlobalTriePool::ClassTrieMap* GlobalTriePool::getTrieMap( const std::string& trieClass ) 
{
	TrieMap::iterator i = d_trieMap.find( trieClass );
	if( i == d_trieMap.end() ) 
		return 0;
	else
		return &(i->second);
}
const GlobalTriePool::ClassTrieMap* GlobalTriePool::getTrieMap( const std::string& trieClass ) const
{
	TrieMap::const_iterator i = d_trieMap.find( trieClass );
	if( i == d_trieMap.end() ) 
		return 0;
	else
		return &(i->second);
}
BELTrie* GlobalTriePool::getTrie( const std::string& trieClass, const std::string& trieId ) 
{
	ClassTrieMap* ctm = getTrieMap( trieClass );
	if( !ctm )
		return 0;
	else {
		ClassTrieMap::iterator j = ctm->find( trieId );
		if( j == ctm->end() ) 
			return 0;
		else
			return (j->second);
	}
}

const BELTrie* GlobalTriePool::getTrie( const std::string& trieClass, const std::string& trieId ) const
{
	const ClassTrieMap* ctm = getTrieMap( trieClass );
	if( !ctm )
		return 0;
	else {
		ClassTrieMap::const_iterator j = ctm->find( trieId );
		if( j == ctm->end() ) 
			return 0;
		else
			return (j->second);
	}
}
GlobalTriePool::~GlobalTriePool()
{
	for(TrieMap::iterator i= d_trieMap.begin(); i!= d_trieMap.end(); ++i ) {
		for( ClassTrieMap::iterator t = i->second.begin(); t != i->second.end(); ++t ) {
			delete t->second;
			t->second = 0;
		}
	}	
	d_trieMap.clear();
}

BELTrie& GlobalTriePool::produceTrie( const std::string& trieClass, const std::string& trieId ) 
{
	ClassTrieMap&  ctm = produceTrieMap( trieClass );

	ClassTrieMap::iterator i = ctm.find( trieId );

	if( i == ctm.end() ) {
		BELTrie* newTrieP = new BELTrie( d_gp );

		i = ctm.insert( ClassTrieMap::value_type(
			trieId, 
			newTrieP
		)).first;
		newTrieP->setGlobalTriePoolId( d_triePool.size() );
		d_triePool.push_back( newTrieP );
	}	
	return *(i->second);
}
GlobalPools::GlobalPools() :
	dtaIdx( &stringPool),
	funSt(*this),
	dateLookup(*this),
	globalTriePool( *this ),
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

StoredUniverse::StoredUniverse(GlobalPools& g, uint32_t id ) :
	d_userId(id),
	gp(g),
	trieCluster(g.globalTriePool,*this),
	hunspell(*this)
{}

size_t   StoredUniverse::internString( const char* s, bool asUserSpecific )
{
	uint32_t id = ( gp.string_intern( s ) ); 
	if( asUserSpecific ) {
		userSpecificStringSet[ id ] = true;
	}
	if( bzSpell )
		bzSpell->addExtraWordToDictionary( id, frequency );

	return id;
}
void StoredUniverse::clearSpell()
{
	BarzerHunspellInvoke spellChecker(hunspell,gp);
	spellChecker.clear();
}

void StoredUniverse::clearTries()
{
	UniverseTrieClusterIterator clusterIter( trieCluster );
	for( ;!clusterIter.isAtEnd(); clusterIter.advance() ) {
		BELTrie& trie = clusterIter.getCurrentTrie();
		{
			BELTrie::WriteLock w_lock(trie.getThreadLock());
			if( trie.getNumUsers() == 1 ) 
				trie.clear();
		}
	} 
}
void StoredUniverse::clear()
{
	clearTries();
	clearSpell();
}

BZSpell* StoredUniverse::initBZSpell( const StoredUniverse* secondaryUniverse )
{
	if( bzSpell ) 
		delete bzSpell;
	bzSpell = new BZSpell( *this );
	bzSpell->init( secondaryUniverse ); 
	return bzSpell;
}

const char* StoredUniverse::getGenericSubclassName( uint16_t subcl ) const
{
	if( subcl< ARR_SZ(g_genDta) ) 
		return g_genDta[subcl].name;
	else 
		return "<unknown>";
}


	UniverseTrieCluster::UniverseTrieCluster( GlobalTriePool& triePool, StoredUniverse& u ) : 
		d_triePool( triePool ) ,
		d_universe(u)
	{
		d_trieList.push_back( &(d_triePool.init()) ) ;
	}

	BELTrie& UniverseTrieCluster::appendTrie( const std::string& trieClass, const std::string& trieId )
	{
		BELTrie& tr = d_triePool.produceTrie(trieClass,trieId);
		d_trieList.push_back( &tr );
		tr.registerUser( d_universe.getUserId() );
		return tr;
	}
	BELTrie& UniverseTrieCluster::prependTrie( const std::string& trieClass, const std::string& trieId )
	{
		BELTrie& tr = d_triePool.produceTrie(trieClass,trieId);
		d_trieList.remove(&tr);
		d_trieList.push_front( &tr );
		tr.registerUser( d_universe.getUserId() );
		return tr;
	}
//// end of generic entities 

} // namespace barzer ends
