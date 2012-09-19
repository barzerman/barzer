#include <barzer_universe.h>
#include <barzer_bzspell.h>
#include <barzer_ghettodb.h>
#include <barzer_meaning.h>
#include <barzer_geoindex.h>
#include <ay/ay_cmdproc.h>
#include <ay/ay_ngrams.h>
#include <boost/filesystem.hpp>

namespace barzer {


GlobalTriePool::ClassTrieMap& GlobalTriePool::produceTrieMap( uint32_t trieClass )
{
	return d_trieMap[trieClass];
}

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

}

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

GlobalPools::GlobalPools(bool fullMode) :
	m_utf8langModelMgr(new ay::UTF8TopicModelMgr),
	m_asciiLangModelMgr(new ay::ASCIITopicModelMgr),
	dtaIdx( *this, &stringPool),
	funSt(*this),
	dateLookup(*this),
	globalTriePool( *this ),
	settings(*this,0),
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

	return ( fp << ( srcName ? srcName : "(null)" ) << ':' << traceInfo.statementNum << '.' <<
	traceInfo.emitterSeqNo );
}

uint32_t StoredUniverse::recordLangWord( int16_t lang )
{
    return d_langInfo.incrementLangCounter( lang );
}
StoredUniverse::StoredUniverse(GlobalPools& g, uint32_t id ) :
	d_userId(id),
	gp(g),
	trieCluster(g.globalTriePool,*this),
	topicTrieCluster(g.globalTriePool,*this),
	bzSpell(new BZSpell(*this)),
	d_ghettoDb(new Ghettodb()),
	m_meanings(new MeaningsStorage),
	m_geo(new BarzerGeo),
	m_soundsLike(false)
{
	m_hints.initFromUniverse(this);
}

StoredUniverse::~StoredUniverse()
{
    delete bzSpell;
    delete d_ghettoDb;
    delete m_meanings;
	delete m_geo;
}

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
    delete m_meanings;
    m_meanings = ( delete m_meanings, new MeaningsStorage() );
	clearSpell();
}

StoredToken& StoredUniverse::internString( int lang, const char* t, BELTrie* triePtr, const char* unstemmed)
{
	// here we may want to tweak some (nonexistent yet) fields in StoredToken
	// to reflect the fact that this thing is actually in the trie
	bool wasNew = false;

	StoredToken& sTok =  gp.getDtaIdx().addToken( lang, wasNew, t );
	const uint32_t origId = sTok.getStringId();
    uint32_t       caseSensitiveId = gp.string_intern( t );

	if (!unstemmed)
		sTok.setStemmed(false);
	else if (wasNew)
		sTok.setStemmed(true);

    BZSpell* bzSpell= getBZSpell();
    
    if( wasNew && (sTok.getLength()  < BZSpell::MAX_WORD_LEN) ) {
        char w[ BZSpell::MAX_WORD_LEN ]; 
        strncpy( w, t, BZSpell::MAX_WORD_LEN-1 );
        w[ BZSpell::MAX_WORD_LEN-1 ] = 0;

        //bool tolowerWasNew = false;
        if( Lang::stringToLower( w, sTok.getLength(), lang ) ) {
            uint32_t tolowerStrId =  gp.string_intern( w );

            if( bzSpell ) 
                bzSpell->addExtraWordToDictionary( tolowerStrId );
        }
    }
	if( triePtr ) {
		BELTrie& trie = *triePtr; 
        trie.addWordInfo( sTok.getStringId(),unstemmed );
        if( !unstemmed ) {
            if( bzSpell ) {
                bzSpell->addExtraWordToDictionary( sTok.getStringId() );
                if( caseSensitiveId != sTok.getStringId()  ) {
                    bzSpell->addExtraWordToDictionary( caseSensitiveId );
                }
            }
        } else {
            const uint32_t unstmId = gp.getDtaIdx().addToken(unstemmed).getStringId();
            trie.addStemSrc( origId, unstmId);
        }
	}
	return sTok;
}

uint32_t StoredUniverse::stemAndIntern( const char* s, size_t len, BELTrie* triePtr )
{
	BZSpell* bzSpell = getBZSpell();
    
	std::string scopy;
    if( s[len] ) {
	    scopy.assign(s, len );
        s=scopy.c_str();
    }
    const StoredToken* storedTok = getStoredToken( s );
    
    int lang = LANG_UNKNOWN;
	if( bzSpell )  {
        uint32_t i = 0xffffffff;
        if( bzSpell->isUsersWord(i,s) ) {
			bzSpell->addExtraWordToDictionary(i);
            return ( storedTok? storedTok->getStringId() : i );
        }
		std::string stem;
        bool gotStemmed = bzSpell->stem(stem, s,lang);

		if( gotStemmed ) {
			internString( lang, stem.c_str(), triePtr, s );
            std::string stemDeduped;
            enum { MIN_DEDUPE_LENGTH = 5 };
            if( bzSpell->dedupeChars(stemDeduped,stem.c_str(),stem.length(),lang,MIN_DEDUPE_LENGTH) ) {
                internString( lang, stemDeduped.c_str(), triePtr, s );
            }
		}
        recordLangWord( lang );
	}
	return internString( lang,s,triePtr,0).getStringId();
}

BZSpell* StoredUniverse::initBZSpell( const StoredUniverse* secondaryUniverse )
{
	bzSpell->init( secondaryUniverse );
	return bzSpell;
}

void StoredUniverse::addLocale (BarzerLocale_ptr locale, bool isDefault)
{
	if (isDefault)
		m_defLocale = locale;
	else
		m_otherLocales.push_back(locale);
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
