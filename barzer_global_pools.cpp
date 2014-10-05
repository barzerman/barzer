/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#include <barzer_global_pools.h>

#include <barzer_universe.h>
#include <barzer_bzspell.h>
#include <barzer_ghettodb.h>
#include <barzer_meaning.h>
#include <barzer_geoindex.h>
#include <barzer_settings.h>
#include <ay/ay_cmdproc.h>
#include <ay/ay_ngrams.h>
#include <boost/filesystem.hpp>
#include <barzer_el_function.h>

namespace barzer {

namespace {

void get_cfg_path( std::string& out, const barzer::BarzerSettings& st, const char* fname  )
{
    if( !fname ) {
        const char *barzerHome = std::getenv("BARZER_HOME");
        const char *dataPath = ( st.getHomeOverridePath().length() ? st.getHomeOverridePath().c_str() : barzerHome );
        std::stringstream ss;

        if( dataPath ) {
            out = ( boost::filesystem::path( dataPath ) / boost::filesystem::path(DEFAULT_CONFIG_FILE) ).string();
        } else 
            out = DEFAULT_CONFIG_FILE;
    } else 
        out.assign(fname);
}

} //namespace 
GlobalTriePool::ClassTrieMap& GlobalTriePool::produceTrieMap( uint32_t trieClass ) { return d_trieMap[trieClass]; }

void GlobalPools::init_cmdline( ay::CommandLineArgs & cmdlProc)
{
    barzer::BarzerSettings &st = getSettings();
	if( cmdlProc.hasArg("-anlt") ) { /// analytical mode is set
		std::cerr << "RUNNING IN ANALYTICAL MODE!\n";
		setAnalyticalMode();
	}

    bool hasArg = false;
    const char *homeArg = cmdlProc.getArgVal(hasArg, "-home", 0);
    if( hasArg ) {
        std::cerr << "CAUTION: entering HOME path\n";
        if( !homeArg ) {
            std::cerr << "BARZER_HOME will be autodeduced" << std::endl;
            st.setEnvPath_AUTO();
        } else { 
            std::cerr << "BARZER_HOME env variable will be ignored " << ( homeArg ? homeArg : "current directory" ) << " used " << std::endl;
            st.setEnvPath_ENV_IGNORE(homeArg);
        }
    }
    if( const char *x = cmdlProc.getArgVal(hasArg, "-inst", 0) ) { // instance 
        if( hasArg )
            setInstanceId( atoi(x) );
    }
    const char *fname = cmdlProc.getArgVal(hasArg, "-cfglist", 0);
    barzer::BELReader reader( *this, &(std::cerr)) ;
    if (hasArg && fname)
    	st.loadListOfConfigs(reader, fname);
    else {

        fname = cmdlProc.getArgVal(hasArg, "-cfg", 0);
        std::string fnameStr;
        if( !fname ) {
            get_cfg_path(fnameStr, st, fname  );
            fname=  fnameStr.c_str();
            std::cerr << "Picked up default config:" << fname << std::endl;
        }
    	st.load(reader, fname);
    }
}

GlobalPools::~GlobalPools()
{
	for( UniverseMap::iterator i =  d_uniMap.begin(); i!= d_uniMap.end(); ++i ) {
		if( i->second )
			delete i->second;
	}

	delete m_utf8langModelMgr;
	delete m_asciiLangModelMgr;
    delete settings;
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

ParseSettings& GlobalPools::parseSettings() { return settings->parseSettings(); }
const ParseSettings& GlobalPools::parseSettings() const { return settings->parseSettings(); }

GlobalPools::GlobalPools(bool fullMode) :
	m_utf8langModelMgr(new ay::UTF8TopicModelMgr),
	m_utf8langModelMgr(new ay::UTF8TopicModelMgr),
	m_asciiLangModelMgr(new ay::ASCIITopicModelMgr),
	dtaIdx( *this, &stringPool),
	funSt( fullMode ? new BELFunctionStorage(*this, fullMode):0 ),
	dateLookup(*this),
	globalTriePool( *this ),
	settings( new BarzerSettings(*this,0) ),
	d_isAnalyticalMode(false),
	d_maxAnalyticalModeMaxSeqLength(5)
{
    BELTrie* defaultTrie = &(globalTriePool.init());

	/// create default universe
	StoredUniverse& defaultUniverse = produceUniverse(DEFAULT_UNIVERSE_ID);
    defaultUniverse.appendTriePtr( defaultTrie,0 ) ;

    globalTriePool.init();
    ///  full mode only initializations
    if( fullMode ) {
        dateLookup.init(*this);
    }
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

	return ( fp << ( srcName ? srcName : "(null)" ) << std::dec << ':' << traceInfo.statementNum << '.' <<
	traceInfo.emitterSeqNo );
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


UniverseTrieCluster::UniverseTrieCluster( GlobalTriePool& triePool, StoredUniverse& u ) :
    d_triePool( triePool ) ,
    d_universe(u)
{ }

BELTrie& UniverseTrieCluster::appendTrie( uint32_t trieClass, uint32_t trieId, GrammarInfo* gi )
{
    BELTrie* tr = d_triePool.produceTrie(trieClass,trieId);
    d_trieList.push_back( TheGrammar(tr,gi) );
    auto uniqId = tr->getUniqueTrieId();

    auto i = d_ownTrieMap.find(uniqId);
    if( i == d_ownTrieMap.end() )
        d_ownTrieMap.insert( UniqIdTrieMap::value_type( uniqId, tr));

    tr->registerUser( d_universe.getUserId() );
    return *tr;
}
BELTrie& UniverseTrieCluster::appendTrie( const char* tc, const char* tid, GrammarInfo* gi )
{
    uint32_t trieClass = d_universe.getGlobalPools().internString_internal( tc );
    uint32_t trieId = d_universe.getGlobalPools().internString_internal( tid );
    return appendTrie( trieClass, trieId, gi );

}


const BELTrie* UniverseTrieCluster::getTrieByClassAndId( const char* trieClass, const char* trieId ) const
    { return getTrieByUniqueId( getUniqueTrieId(trieClass, trieId) ); }
BELTrie* UniverseTrieCluster::getTrieByClassAndId( const char* trieClass, const char* trieId ) 
    { return const_cast<BELTrie*>(getTrieByUniqueId( getUniqueTrieId(trieClass, trieId) )); }

UniqueTrieId UniverseTrieCluster::getUniqueTrieId ( const char* trieClass, const char* trieId ) const 
{
    return {    
        d_universe.getGlobalPools().internalString_getId(trieClass), 
        d_universe.getGlobalPools().internalString_getId(trieId) 
    };
}

} // namespace barzer 
