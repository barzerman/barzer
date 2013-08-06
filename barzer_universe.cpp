
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#include <barzer_universe.h>
#include <barzer_bzspell.h>
#include <barzer_ghettodb.h>
#include <barzer_meaning.h>
#include <barzer_geoindex.h>
#include <ay/ay_cmdproc.h>
#include <ay/ay_ngrams.h>
#include <boost/filesystem.hpp>
#include <zurch_docidx.h>
#include <barzer_beni.h>
#include <barzer_el_trie_ruleidx.h>

namespace barzer {

void StoredUniverse::setUBits( const char* str )
{
    /// setting ubits from FLAG
    for( const char* s = str; *s; ++s ) {
        switch( *s ) {
        case 'c': setBit(StoredUniverse::UBIT_NEED_CONFIDENCE); break;
        }
    }
}

uint32_t StoredUniverse::recordLangWord( int16_t lang )
{
    return d_langInfo.incrementLangCounter( lang );
}
StoredUniverse::StoredUniverse(GlobalPools& g, uint32_t id ) :
	d_userId(id),
    d_entNameIdx(0),
    d_entIdLookupBENI(0),
    m_ruleIdx(new TrieRuleIdx(*this)),
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
    d_entIdLookupBENI = new SubclassBENI( *this );
}

void StoredUniverse::searchEntitiesInZurch( BENIFindResults_t& out, const char* str, const QuestionParm& qparm ) const
{
    if( d_entNameIdx )
        d_entNameIdx->zurchEntities( out, str, qparm );
}
void StoredUniverse::searchEntitiesByName( BENIFindResults_t& out, const char* str, const QuestionParm& qparm ) const
{
    if( d_entNameIdx ) 
        d_entNameIdx->search( out, str, d_settings.d_beni_Cutoff );
}

void StoredUniverse::beniInit( )
{
    delete d_entNameIdx;
    d_entNameIdx = new SmartBENI(*this);
}

SmartBENI& StoredUniverse::beni( )
{
    if( !d_entNameIdx )
        beniInit();
    return *d_entNameIdx;
}

void StoredUniverse::indexEntityNames( const StoredEntityClass& ec ) 
{
    if( !d_entNameIdx )  
        d_entNameIdx = new SmartBENI( *this );
    d_entNameIdx->addEntityClass(ec);
}

StoredUniverse::~StoredUniverse()
{
    // cleaning zurch 
    for( auto i= d_zurchIndexPool.begin(); i != d_zurchIndexPool.end(); ++i )  {
        delete i->second;
        i->second = 0;
    }
    d_zurchIndexPool.clear();

	delete m_ruleIdx;
    delete bzSpell;
    delete d_ghettoDb;
    delete m_meanings;
	delete m_geo;

    delete d_entNameIdx;
    delete d_entIdLookupBENI;
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

void StoredUniverse::clearMeanings()
{
    m_meanings = ( delete m_meanings, new MeaningsStorage() );
}
void StoredUniverse::clearGeo()
{
    m_geo->clear();
}
void StoredUniverse::clear()
{
    topicTrieCluster.clearTries();
    clearTrieList();
    m_hints.clear();
    clearMeanings();
	clearSpelling();
    clearGeo();
    userSpecificStringSet.clear();
    d_entSeg.clear();
    m_defLocale->clear();
    m_otherLocales.clear();
    d_langInfo.clear();
    d_tokenizerStrat.clear();
    m_soundsLike=false;
    d_ghettoDb->clear();
    d_biflags.clear();
	m_hints.initFromUniverse(this);

    delete d_entNameIdx;
    d_entNameIdx = 0;

    delete d_entIdLookupBENI;
    d_entIdLookupBENI=new SubclassBENI( *this );
}

StoredToken& StoredUniverse::internString( int lang, const char* t, BELTrie* triePtr, const char* unstemmed)
{
	// here we may want to tweak some (nonexistent yet) fields in StoredToken
	// to reflect the fact that this thing is actually in the trie
	bool wasNew = false;

    uint32_t priorId = gp.string_getId( t );

    char w[ BZSpell::MAX_WORD_LEN ]; 
    size_t t_len = strlen(t);
    if( t_len >= BZSpell::MAX_WORD_LEN )
        t_len = BZSpell::MAX_WORD_LEN-1;

    if( priorId == 0xffffffff ) { // this is a brand new word - de-casing it
        wasNew = true;
        memcpy( w,t, t_len );
        w[ t_len ] = 0;
        Lang::stringToLower( w, t_len, lang );
        t= w;
    }

    /// at this point t is GUARANTEED TO BE LOWER CASE 
    uint32_t       internId = gp.string_intern( t );
	StoredToken& sTok =  gp.getDtaIdx().addToken( lang, wasNew, t );
	const uint32_t origId = sTok.getStringId();

	if (!unstemmed)
		sTok.setStemmed(false);
	else if (wasNew)
		sTok.setStemmed(true);

    BZSpell* bzSpell= getBZSpell();
    
    if( wasNew && (internId != origId) && (sTok.getLength()  < BZSpell::MAX_WORD_LEN) ) {
        strncpy( w, t, t_len );
        w[ t_len ] = 0;

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
                if( internId != sTok.getStringId()  ) {
                    bzSpell->addExtraWordToDictionary( internId );
                }
            }
        } else {
			auto unstemmedLen = strlen(unstemmed);
			if (unstemmedLen >= BZSpell::MAX_WORD_LEN)
				unstemmedLen = BZSpell::MAX_WORD_LEN - 1;

            strncpy( w, unstemmed, unstemmedLen );
            w[ unstemmedLen ] = 0;
            Lang::stringToLower( w, sTok.getLength(), lang );

            const uint32_t unstmId = gp.getDtaIdx().addToken(w).getStringId();
            trie.addStemSrc( origId, unstmId);
        }
	}
	return sTok;
}

uint32_t StoredUniverse::stemAndIntern( int& lang, const char* s, size_t len, BELTrie* triePtr )
{
	BZSpell* bzSpell = getBZSpell();
    
	std::string scopy;
    if( s[len] ) {
	    scopy.assign(s, len );
        s=scopy.c_str();
    }
    const StoredToken* storedTok = getStoredToken( s );
    
    lang = LANG_UNKNOWN;
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

zurch::DocIndexAndLoader* StoredUniverse::initZurchIndex( uint32_t id )
{
    auto i = d_zurchIndexPool.find(id);
    if( i == d_zurchIndexPool.end() )  {
        i= d_zurchIndexPool.insert( std::pair< uint32_t, zurch::DocIndexAndLoader*>(id,new zurch::DocIndexAndLoader()) ).first;
    }
    i->second->init( *this );
    return (i->second);
}

zurch::DocIndexAndLoader* StoredUniverse::getZurchIndex( uint32_t idxId )
{
    auto i = d_zurchIndexPool.find(idxId);
    return( i == d_zurchIndexPool.end() ? 0: i->second );
}

const zurch::DocIndexAndLoader* StoredUniverse::getZurchIndex( uint32_t idxId ) const
{
    auto i = d_zurchIndexPool.find(idxId);
    return( i == d_zurchIndexPool.end() ? 0: i->second );
}
//// end of generic entities

void StoredUniverse::entLookupBENIAddSubclass( const StoredEntityClass& ec, const char* pat, const char* rep ) 
{
    d_entIdLookupBENI->addSubclassIds( ec, pat, rep );
}
int StoredUniverse::entLookupBENISearch( BENIFindResults_t& out, const char* query, const StoredEntityClass& ec, const QuestionParm& qparm ) const
{
    int rc = d_entIdLookupBENI->search( out, query, ec );
    return rc;
}
} // namespace barzer ends
