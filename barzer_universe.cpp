#include <barzer_universe.h>
#include <barzer_bzspell.h>

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

GlobalTriePool::ClassTrieMap& GlobalTriePool::produceTrieMap( uint32_t trieClass ) 
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

GlobalTriePool::ClassTrieMap* GlobalTriePool::getTrieMap( uint32_t trieClass ) 
{
	TrieMap::iterator i = d_trieMap.find( trieClass );
	if( i == d_trieMap.end() ) 
		return 0;
	else
		return &(i->second);
}
const GlobalTriePool::ClassTrieMap* GlobalTriePool::getTrieMap( uint32_t trieClass ) const
{
	TrieMap::const_iterator i = d_trieMap.find( trieClass );
	if( i == d_trieMap.end() ) 
		return 0;
	else
		return &(i->second);
}
BELTrie* GlobalTriePool::getTrie( uint32_t trieClass, uint32_t trieId ) 
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
BELTrie* GlobalTriePool::getTrie( const char* trieClass, const char* trieId ) 
{
    uint32_t tc = d_gp.internalString_getId(  trieClass ), tid = d_gp.internalString_getId( trieId ); 
    return getTrie( tc, tid );
}
const BELTrie* GlobalTriePool::getTrie( const char* trieClass, const char* trieId ) const
{
    uint32_t tc = d_gp.internalString_getId(  trieClass ), tid = d_gp.internalString_getId( trieId ); 
    return getTrie( tc, tid );
}

const BELTrie* GlobalTriePool::getTrie( uint32_t trieClass, uint32_t trieId ) const
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

BELTrie* GlobalTriePool::produceTrie( uint32_t trieClass, uint32_t trieId ) 
{
	ClassTrieMap&  ctm = produceTrieMap( trieClass );

	ClassTrieMap::iterator i = ctm.find( trieId );

	if( i == ctm.end() ) {
		BELTrie* newTrieP = new BELTrie( d_gp );
        newTrieP->setTrieClassAndId( trieClass, trieId );

		i = ctm.insert( ClassTrieMap::value_type(
			trieId, 
			newTrieP
		)).first;
		newTrieP->setGlobalTriePoolId( d_triePool.size() );
		d_triePool.push_back( newTrieP );
		return newTrieP;
	} else 
		return( i->second ); 
}
GlobalPools::GlobalPools() :
	dtaIdx( &stringPool),
	funSt(*this),
	dateLookup(*this),
	globalTriePool( *this ),
	settings(*this,0),
	d_isAnalyticalMode(false),
	d_maxAnalyticalModeMaxSeqLength(3)
{
    BELTrie* defaultTrie = &(globalTriePool.init());

	/// create default universe 
	StoredUniverse& defaultUniverse = produceUniverse(DEFAULT_UNIVERSE_ID);
    defaultUniverse.appendTriePtr( defaultTrie,0 ) ;	

	createGenericEntities();
    globalTriePool.init();
}

size_t GlobalPools::readDictionaryFile( const char* fname ) 
{
	FILE* fp = fopen( fname, "r" );
	if( !fp ) {
		std::cerr << "failed to open file " << fname << " for reading\n";
		return 0;
	}
	char buf[ 256 ] ;
	size_t oldSize = d_dictionary.size();
	while( fgets( buf, sizeof(buf), fp ) ) {
		buf[ sizeof(buf)-1 ] = 0;
		size_t buf_len = strlen(buf);
        if( buf_len )
		    buf[ buf_len-1] = 0;
		addWordToDictionary( string_intern( buf ) );
	}
	
	return (d_dictionary.size() - oldSize);
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
	topicTrieCluster(g.globalTriePool,*this),
	hunspell(*this),
	bzSpell(new BZSpell(*this))
{}

size_t   StoredUniverse::internString( const char* s, bool asUserSpecific, uint8_t frequency )
{
	uint32_t id = ( gp.string_intern( s ) ); 
	if( asUserSpecific ) {
		userSpecificStringSet[ id ] = true;
	}
	if( bzSpell ) {
		bzSpell->addExtraWordToDictionary( id, frequency );
	}
	return id;
}
void StoredUniverse::clearSpell()
{
	/*
	BarzerHunspellInvoke spellChecker(hunspell,gp);
	spellChecker.clear();
	*/
	delete bzSpell;
	bzSpell = new BZSpell(*this);
}

void UniverseTrieCluster::clearTries()
{
	for( TheGrammarList::iterator i = d_trieList.begin(); i!= d_trieList.end(); ++i ) { 
		BELTrie& trie = i->trie();
		{
			BELTrie::WriteLock w_lock(trie.getThreadLock());
			if( trie.getNumUsers() == 1 ) 
				trie.clear();
		}
	} 
}

void StoredUniverse::clear()
{
    topicTrieCluster.clearTries();
    trieCluster.clearTries();
    d_topicEntLinkage.clear();
	clearSpell();
}

BZSpell* StoredUniverse::initBZSpell( const StoredUniverse* secondaryUniverse )
{
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
void StoredUniverse::clearSpelling() 
{
    if( bzSpell ) 
        bzSpell->clear();
}


	UniverseTrieCluster::UniverseTrieCluster( GlobalTriePool& triePool, StoredUniverse& u ) : 
		d_triePool( triePool ) ,
		d_universe(u)
	{
		// https://github.com/barzerman/barzer/issues/108
        /*
        if( !d_universe.getUserId() ) {
            d_trieList.push_back( &(d_triePool.init()) ) ;
        }
        */
	}

	BELTrie& UniverseTrieCluster::appendTrie( uint32_t trieClass, uint32_t trieId, GrammarInfo* gi )
	{
		BELTrie* tr = d_triePool.produceTrie(trieClass,trieId);
		d_trieList.push_back( TheGrammar(tr,gi) );
		tr->registerUser( d_universe.getUserId() );
		return *tr;
	}
	BELTrie& UniverseTrieCluster::appendTrie( const char* tc, const char* tid, GrammarInfo* gi )
    {
        uint32_t trieClass = d_universe.getGlobalPools().internString_internal( tc );
        uint32_t trieId = d_universe.getGlobalPools().internString_internal( tid );
        return appendTrie( trieClass, trieId, gi );

    }
//// end of generic entities 

} // namespace barzer ends
