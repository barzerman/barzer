#include <barzer_barz.h>
#include <barzer_parse.h>
#include <barzer_el_chain.h>
#include <barzer_universe.h>


namespace barzer {

//// BarzHints 
void  BarzHints::initFromUniverse( const StoredUniverse* u ) 
{
	d_universe = u;
    d_hasAsciiLang = true;
	clear();
	if (u) {
		switch (u->getLangInfo().getDominantLanguage())
		{
		case LANG_RUSSIAN:
			setHint(BHB_DECIMAL_COMMA);
			break;
		default:
			break;
		}
	}
}

void BarzHints::setHint(HintsFlag flag, bool val)
{
	d_bhb.set(flag, val);
}

void BarzHints::clearHint(BarzHints::HintsFlag flag)
{
	d_bhb.unset(flag);
}

bool BarzHints::testHint(BarzHints::HintsFlag flag) const
{
	return d_bhb[flag];
}

/// end of BarzHints 

bool BarzelTrace::detectLoop( ) const
{
    if( !d_tvec.size() ) 
        return false;
    // lets start with the simplest loop 
    size_t tailRepeats = 0;
    TraceVec::const_reverse_iterator lastI = d_tvec.rbegin();
    TraceVec::const_reverse_iterator i = lastI;

    for( ++i; i != d_tvec.rend(); ++i ) {
        if( i->eq(*lastI) ) {
            ++tailRepeats;
            if( tailRepeats> MAX_TAIL_REPEAT ) 
                return true;
        } else
            return false;
    }
    return false;
}

void Barz::setUniverse (const StoredUniverse *u)
{
	m_hints.clear();
	if (u)
		m_hints = u->getBarzHints();
}

const BarzHints& Barz::getHints() const
{
	return m_hints;
}

void Barz::syncQuestionFromTokens()
{
	size_t qLen = 0;
	for( TTWPVec::const_iterator i = ttVec.begin(); i!= ttVec.end(); ++i ) {
		qLen+= (i->first.buf.length()+1);
	}
	if( !qLen ) 
		return ;
	question.clear();
	question.resize( qLen );
	size_t pos = 0;
	for( TTWPVec::iterator i = ttVec.begin(); i!= ttVec.end(); ++i ) {
		TToken& t = i->first;
		char* pBuf = &(question[pos]);
		memcpy( pBuf, t.buf.c_str(), t.buf.length() );
		t.buf = pBuf;
		pBuf[t.buf.length()] = 0;
		pos+= (t.buf.length()+1);
	}
	
}

int Barz::tokenize( const TokenizerStrategy& strat, QTokenizer& tokenizer, const char* q, const QuestionParm& qparm )
{
	beadChain.clear();
	ctVec.clear();
	ttVec.clear();
	questionOrig.assign(q);
	questionOrigUTF8.assign(q);

	int rc = tokenizer.tokenize( *this, strat, qparm );
    return 0;
}

int Barz::tokenize( QTokenizer& tokenizer, const char* q, const QuestionParm& qparm )
{
	/// invalidating all higher order objects
	beadChain.clear();
	ctVec.clear();
	ttVec.clear();

	questionOrig.assign(q);
	questionOrigUTF8.assign(q);

	int rc = tokenizer.tokenize( ttVec, questionOrig.c_str(), qparm );
	
	syncQuestionFromTokens();
	return rc;
}

int Barz::classifyTokens( const TokenizerStrategy& strat, QTokenizer& tokenizer, QLexParser& lexer, const char* q, const QuestionParm& qparm )
{
	/// invalidating all higher order objects
	beadChain.clear();
	ctVec.clear();
    return lexer.lex( *this, strat, tokenizer, q, qparm );
}
int Barz::classifyTokens( QLexParser& lexer, const QuestionParm& qparm )
{
	/// invalidating all higher order objects
	beadChain.clear();
	ctVec.clear();
	return lexer.lex( *this, qparm );
}

void Barz::clearBeads()
{
	beadChain.clear();
}
void Barz::clearWithTraceAndTopics()
{
    topicInfo.clear();
	barzelTrace.clear();
    clear();
}

void Barz::clear()
{
	beadChain.clear();
	ctVec.clear();
	ttVec.clear();

	question.clear();
	questionOrig.clear();
	questionOrigUTF8.clear();
}


int Barz::chainInit( const QuestionParm& qparm ) 
{
	beadChain.init(ctVec);
    return 0;
}

int Barz::analyzeTopics( QSemanticParser& sem, const QuestionParm& qparm, bool needInit )
{
    if( needInit )
	    beadChain.init(ctVec);
    return sem.analyzeTopics( *this, qparm );
}
int Barz::parse_Autocomplete( MatcherCallback& cb, QSemanticParser& sem, const QuestionParm& qparm )
{
	beadChain.init(ctVec);

    BeadList& lst = getBeadList();
    if( !lst.empty()  ) {
        BeadList::reverse_iterator bi = lst.rbegin();

        {
            sem.parse_Autocomplete( cb, *this, qparm, QSemanticParser_AutocParms() );
            CTWPVec& ctVec = bi->getCTokens(); 
            if( ctVec.size() == 1 ) {
                CToken& ctok = ctVec.front().first;
                if( ctok.isString() && ctok.qtVec.size()== 1 ) {
                    TToken& ttok = ctok.qtVec[0].first;
                    const StoredUniverse& universe = sem.getUniverse();
                    /*
                    const ay::UniqueCharPool& ucpool = universe.getGlobalPools().getStringPool();
                    const ay::UniqueCharPool::CharIdMap& cidMap = ucpool.getCharIdMap();
                    */

                    const BZSpell::char_cp_to_strid_map* wordMapPtr = universe.getValidWordMapPtr();
                    if( wordMapPtr ) {
                        const DtaIndex& dtaIdx = universe.getDtaIdx(); 
                        BZSpell::char_cp_to_strid_map::const_iterator i = wordMapPtr->lower_bound(ttok.buf.c_str());
                        for( ;i!= wordMapPtr->end(); ++i ) {
                            const char* str = i->first;
                            if( !str || strncmp(ttok.buf.c_str(),str,ttok.buf.length()) ) 
                                break;
                            if( !str[ttok.buf.length()] )
                                continue;
                            uint32_t strId = i->second;
                            if( universe.isWordValidInUniverse(strId) ) {
                                const StoredToken* stok = dtaIdx.getStoredToken( str );
                                if( stok ) {
                                    ctok.setStoredTok_raw( stok );
                                    bi->initFromCTok( ctok );
                                    uint32_t str_len = strlen(str);
                                    QSemanticParser_AutocParms autocParm( (str_len>ttok.buf.length()) ? (str_len-ttok.buf.length()): 0 );
                                    sem.parse_Autocomplete( cb, *this, qparm, autocParm );
                                }
                            }
                        }
                    }
                }
            }
        } 
    }
	return 0;
}
int Barz::semanticParse( QSemanticParser& sem, const QuestionParm& qparm, bool needInit )
{
    if( needInit ) {
	    /// invalidating and initializing all higher order objects
        if( sem.needTopicAnalyzis() ) {
            analyzeTopics( sem, qparm );
            clearBeads(); // we don't need to tokenize again really - just purge the beads  
        }
	    beadChain.init(ctVec);
    }

	return sem.semanticize( *this, qparm );
}
//// post-semantcial interpretation 
int Barz::postSemanticParse( QSemanticParser& sem, const QuestionParm& qparm )
{
	/// potprocessing the beadChain
	typedef BarzelBeadChain::Range  BeadRange;
	// BeadRange rng = beadChain.getFullRange();
	//// collapsing consecutive entities
	return 0;
}

/// entity segregation
namespace {

typedef std::vector< BeadList::iterator > BeadListIterVec;

typedef std::pair< BarzerEntity, BeadList::iterator > EntListIterPair;

struct EntListIterPair_comp_eq {
    inline bool operator() ( const EntListIterPair& l, const EntListIterPair& r ) const 
    { return ( l.first == r.first && l.second == r.second ); }
};

struct BeadList_iteartor_comp_less {
    inline bool operator() ( const BeadList::iterator& l, const BeadList::iterator& r ) const 
    { return (&(*l) < &(*r)); }
};

struct EntListIterPair_comp_less {
    inline bool operator() ( const EntListIterPair& l, const EntListIterPair& r ) const 
    {
        if( l.first.eclass < r.first.eclass ) {
            return true;
        } else if( r.first.eclass < l.first.eclass ) {
            return false;
        } else 
            return ( &(*(l.second)) < &(*(r.second)) );
    }
};
typedef std::vector< EntListIterPair > EntListPairVec;

struct EntityRelevanceEstimator {
    const StoredUniverse& universe;
    EntityRelevanceEstimator( const StoredUniverse& u ) : universe(u) {}

    /// compares relevances 
    bool operator()( const BarzerEntity& r, const BarzerEntity& l ) const {
        const EntityData::EntProp* rd = universe.getGlobalPools().entData.getEntPropData( r );
        if( rd ) {
            const EntityData::EntProp* ld = universe.getGlobalPools().entData.getEntPropData( l );
            if( ld ) 
                return( ld->relevance < rd->relevance );
            else 
                return true;
        } else 
            return false;
    }
    bool operator()( const BarzerEntity& l) const {
        const EntityData::EntProp* ld = universe.getGlobalPools().entData.getEntPropData( l );
        return( ld && ld->relevance );
    }
};

} // anon namespace ends
int Barz::segregateEntities( const StoredUniverse& u, const QuestionParm& qparm, const char* q )
{
	typedef BarzelBeadChain::Range  BeadRange;
	BeadRange rng = beadChain.getFullRange();
    

    // vector of beads with entities
    EntListPairVec elpVec;
    typedef std::vector< BeadList::iterator > BLIVec ;
    BLIVec entBeadVec;
    /// figuring out whether we need to do anything  
    for( BeadList::iterator i = rng.first; i!= rng.second; ++i ) {
        const BarzelBeadAtomic* atomic = i->getAtomic();
        if( !atomic ) 
            continue;
         
        const BarzerEntity* ent = atomic->getEntity();
        
        if( ent ) { // entity 
            elpVec.push_back( EntListIterPair( *ent, i ) );
            entBeadVec.push_back( i );
        } else { 
            const BarzerEntityList* entList =atomic->getEntityList();
            if( !entList )
                continue;

            const BarzerEntityList::EList& elst = entList->getList();
            if( elst.size() ) {
                entBeadVec.push_back( i );
                for( BarzerEntityList::EList::const_iterator ei = elst.begin(); ei != elst.end(); ++ei ) 
                    elpVec.push_back( EntListIterPair( (*ei), i ) );
            } else 
                i->become( BarzelBeadBlank() );
        }
    }

    
    BeadList& beadList = beadChain.getList();


    // if all we have is one entity there's really no point in changing anything
    if( elpVec.size() < 2 ) 
        return 0;

    std::sort( elpVec.begin(), elpVec.end(),  EntListIterPair_comp_less() );

    std::set< BeadList::iterator, BeadList_iteartor_comp_less > absorbedBeads;

    StoredEntityClass prevEC = elpVec[0].first.eclass;
    BarzerEntityList* curEntList = beadChain.appendBlankAtomicVal<BarzerEntityList>();

    BeadList::iterator bi = elpVec[0].second;
    curEntList->addEntity( elpVec[0].first );
    curEntList->setClass( prevEC );
    beadList.back().absorbBead( *bi );
    absorbedBeads.insert( bi );

    for( EntListPairVec::const_iterator i=(elpVec.begin()+1); i!= elpVec.end(); ++i ) {
        bi = i->second;
        if( prevEC != i->first.eclass ) {
            prevEC = i->first.eclass;

            curEntList = beadChain.appendBlankAtomicVal<BarzerEntityList>();
            curEntList->setClass( prevEC );

            absorbedBeads.clear();
        }
        if( !curEntList ) {
            std::cerr << "fatal error " << __FILE__ << ":" << __LINE__ << "\n";
        } else 
            curEntList->addEntity( i->first );
        if( absorbedBeads.insert(bi).second ) 
            beadList.back().absorbBead( *bi );
    }

    // at this point new beads have been appended to the list 
    // we will clean out the trash
    /// starting with beads containing multiple entity classes 
    for( BLIVec::iterator bi= entBeadVec.begin(); bi != entBeadVec.end(); ++bi ) {
        beadList.erase( *bi );
    }

    return 0;
}
int Barz::sortEntitiesByRelevance( const StoredUniverse& u, const QuestionParm& qparm, const char* q )
{
    /// end of figuring out whether we need anything 
	BeadRange rng = beadChain.getFullRange();
    for( BeadList::iterator i = rng.first; i!= rng.second; ++i ) {
        BarzelBeadAtomic* atomic = i->getAtomic();
        if( atomic ) {
            BarzerEntityList* entList =atomic->getEntityList();
            if( entList && entList->theList().size() ) {
                ay::zerosort( entList->theList().begin(), entList->theList().end(), EntityRelevanceEstimator(u) );
            }
        }
    }
    return 0;
}

} // barzer namepace ends
