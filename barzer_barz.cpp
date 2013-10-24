/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#include <barzer_barz.h>
#include <barzer_parse.h>
#include <barzer_el_chain.h>
#include <barzer_universe.h>
#include <barzer_server.h>
#include <barzer_el_cast.h>
#include <barzer_server.h>

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

void BarzelTrace::pushError( const char* err ) 
{
    if( d_tvec.size() ) {
        if( d_tvec.back().errVec.size() < BARZEL_TRACE_MAX_ERR )  
            d_tvec.back().errVec.push_back( err );
        else if( d_tvec.back().errVec.size() < BARZEL_TRACE_MAX_ERR+1 ) {
            d_tvec.back().errVec.push_back( "Too many errors to report..." );
        }
    }
}
void BarzelTrace::setError( const char* err )
{
    return pushError(err);
}
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

const BarzerDateTime* Barz::getNowPtr() const
{
    return ( d_serverReqEnv? d_serverReqEnv->getNowPtr() : 0 ) ;  
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
    if( !tokenizer.universe().checkBit( UBIT_NO_EXTRA_NORMALIZATION ) )
        extraNormalization(qparm);
	questionOrigUTF8.assign(questionOrig.c_str());

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

    if( !tokenizer.universe().checkBit( UBIT_NO_EXTRA_NORMALIZATION ) )
        extraNormalization(qparm);
	questionOrigUTF8.assign(questionOrig.c_str());

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
    confidenceData.clear();
}
void Barz::clearWithTraceAndTopics()
{
    topicInfo.clear();
	barzelTrace.clear();
    clear();
}

void Barz::clear()
{
    clearBeads();
	ctVec.clear();
	ttVec.clear();

	question.clear();
	questionOrig.clear();
	questionOrigUTF8.clear();
    d_beni.clear();
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
        // AYDEBUG( beadChain.getFullRange() );
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
    bool operator()( const BarzerEntity& l, const BarzerEntity& r ) const {
        const EntityData::EntProp* rd = universe.getEntPropData( r );
        if( rd ) {
            const EntityData::EntProp* ld = universe.getEntPropData( l );
            if( ld ) 
                return( ld->relevance < rd->relevance );
            else 
                return true;
        } else 
            return false;
    }
    bool operator()( const BarzerEntity& l) const {
        const EntityData::EntProp* ld = universe.getEntPropData( l );
        return( !ld || !ld->relevance );
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
namespace {

void offset_sz_vec_add( std::vector<std::pair<size_t,size_t>>& vec, const std::pair<size_t,size_t>& p, const std::string& str ) 
{
    if( !vec.size() ) 
        vec.push_back( p );
    else { 
        size_t prevEnd = vec.back().first + vec.back().second;
        if( p.first == prevEnd )
            vec.back().second += p.second;
        else if( prevEnd+1== p.first && isspace(str[prevEnd]))
            vec.back().second += (p.second+1);
        else 
            vec.push_back( p );
    } 
}

} // anon namespace

void BarzConfidenceData::sortAndInvert( const std::string& origStr, OffsetAndLengthPairVec& vec )
{
    struct sort_by_offset_sz {
        bool operator()( const std::pair<size_t,size_t>& l, const std::pair<size_t,size_t>&r ) const {
            return( l.first < r.first ? true : ( r.first < l.first ? false : r.second < l.second) );
        }
    } ;

    std::sort( vec.begin(), vec.end(), sort_by_offset_sz() );
    OffsetAndLengthPairVec newVec;
    for( OffsetAndLengthPairVec::const_iterator i = vec.begin(); i!= vec.end(); ++i ) 
        offset_sz_vec_add( newVec, *i, origStr );

    newVec.swap( vec );
    newVec.clear();
    
    /// at this point the vector has been compacted (contained in vec) newVec is empty 
    size_t offs = 0;
    for( OffsetAndLengthPairVec::const_iterator i = vec.begin(); i!= vec.end(); ++i )  {
        if( i->first != offs ) {
            size_t len = i->first-offs;
            for( size_t j=0; j< len && isspace(origStr[offs]); ++j ) {
                    ++offs;
                    --len;
            }
            while( len && isspace(origStr[offs+len-1]) ) --len; 
            newVec.push_back( std::pair< size_t, size_t >(offs, len ) );
        }
        offs= i->second+i->first;
    }
    if( offs < origStr.length() ) {
        size_t len = origStr.length()-offs;
        
        for( size_t j=0; j< len && len && isspace(origStr[offs]); ++j ) {
            ++offs;
            --len;
        }
        while( len && isspace(origStr[offs+len-1]) ) --len; 
        newVec.push_back( std::pair< size_t, size_t >(offs, len) );
    }
    newVec.swap( vec );
}
void BarzConfidenceData::sortAndInvert( const std::string& origStr )
{

    if( d_hiCnt ) 
        sortAndInvert( origStr, d_noHi );

    if( d_medCnt ) {
        sortAndInvert( origStr, d_noMed );
    }
    if( d_loCnt ) {
        sortAndInvert( origStr, d_noLo );
    }
}
void BarzConfidenceData::fillString( std::vector<std::string>& dest, const std::string& src, int conf ) const
{
    const std::vector<std::pair<size_t,size_t>>* vec = ( conf == BarzelBead::CONFIDENCE_HIGH ? &d_noHi : 
            ( conf == BarzelBead::CONFIDENCE_MEDIUM ? &d_noMed : &d_noLo )
        );
    for( OffsetAndLengthPairVec::const_iterator i = vec->begin(); i!= vec->end(); ++i ) {
        if( i->second && i->first< src.length() && i->first+i->second <= src.length() ) {
            std::string tmp= src.substr(i->first, i->second );
            if( dest.empty() || dest.back() != tmp )
               dest.push_back( tmp );
        }
    }
}
int Barz::computeConfidence( const StoredUniverse& u, const QuestionParm& qparm, const char* q, const ConfidenceMode& mode )
{
	BeadRange rng = beadChain.getFullRange();
    for( BeadList::iterator i = rng.first; i!= rng.second; ++i ) {
        if( !i->isComplexAtomicType() ) 
            continue;
        int conf = i->computeBeadConfidence( &u );
        if( mode.mode == ConfidenceMode::MODE_ENTITY ) {
            if( const BarzerEntity* ent = i->isEntity() ){
                if( !mode.eclass.ec || (ent->eclass.ec == mode.eclass.ec && ( !mode.eclass.subclass || (ent->eclass.subclass == mode.eclass.subclass) )) ) {
                    conf = BarzelBead::CONFIDENCE_HIGH;
                }
            } else if( const BarzerLiteral* l = i->getLiteral() ) {
                if( l->isStop() ) 
                    conf = BarzelBead::CONFIDENCE_HIGH;
            }
        }
        i->setBeadConfidence( conf );

        const CTWPVec& ctoks = i->getCTokens();
        for( CTWPVec::const_iterator ci = ctoks.begin(), ci_end = ctoks.end(); ci != ci_end; ++ci ) {
            const TTWPVec& ttv = ci->first.getTTokens();

            for( TTWPVec::const_iterator ti = ttv.begin(); ti!= ttv.end() ; ++ti ) {
                const TToken& ttok = ti->first;
                if( !ttok.buf.length() ) 
                    continue;

                std::pair<size_t,size_t> szAndLength = ttok.getOrigOffsetAndLength();
                switch( conf ) {
                case BarzelBead::CONFIDENCE_HIGH:
                    confidenceData.d_noHi.push_back( szAndLength );
                    confidenceData.d_noMed.push_back( szAndLength );
                    confidenceData.d_noLo.push_back( szAndLength );
                    confidenceData.d_hiCnt++;
                    break;
                case BarzelBead::CONFIDENCE_MEDIUM:
                    confidenceData.d_noMed.push_back( szAndLength );
                    confidenceData.d_noLo.push_back( szAndLength );
                    confidenceData.d_medCnt++;
                    break;
                case BarzelBead::CONFIDENCE_LOW:
                    confidenceData.d_noLo.push_back( szAndLength );
                    confidenceData.d_loCnt++;
                    break;
                }
            }
        }
    }
    confidenceData.sortAndInvert(questionOrig);
    /// inverting 

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
                ay::zerosort( entList->theList().rbegin(), entList->theList().rend(), EntityRelevanceEstimator(u) );
            }
        }
    }
    return 0;
}

namespace {

inline bool beni_string_likely_isid( const std::string& s, size_t numGlyphs ) 
{
    if( numGlyphs < 18 )
        return true;
    size_t num_spaces  = 1;
    for( const auto& i : s ) {
        if( isdigit(i) ) 
            return true;
        else if( i =='.' || i=='/' || i== '-' )
            return true;
        else if( isspace(i) ) 
            ++num_spaces;
    }
    return ( num_spaces * 4 > numGlyphs ) ;
}

}
int Barz::beniSearch( const StoredUniverse& u, const QuestionParm& qparm )
{
    if( qparm.d_beniMode == QuestionParm::BENI_NO_BENI ) 
        return 0;
    enum { MAX_BENI_LENGTH = 34 };

    size_t numGlyphs = questionOrigUTF8.length() ;
    if( qparm.mustBeni() || numGlyphs  < MAX_BENI_LENGTH ) {
        d_beni.d_entVec.clear();
        if( !qparm.mustBeni() && u.checkBit(UBIT_USE_BENI_IDS) ) {
            if( !beni_string_likely_isid( questionOrig, numGlyphs ) ) 
                return 1;
        }
        u.searchEntitiesByName( d_beni.d_entVec, questionOrig.c_str(), qparm );

        const double NONEED_ZURCH_COVERAGE = 0.7;
        if( !d_beni.d_zurchEntVec.empty() && d_beni.d_zurchEntVec[0].coverage >= NONEED_ZURCH_COVERAGE ) 
            return 0;
        u.searchEntitiesInZurch( d_beni.d_zurchEntVec, questionOrig.c_str(), qparm );
    }
    return 0;
}

const RequestVariableMap* Barz::getRequestVariableMap()  const 
    { if( const RequestEnvironment* p = getServerReqEnv() ) return p->getReqVarPtr(); else return 0; }
RequestVariableMap* Barz::getRequestVariableMap() 
    { if( RequestEnvironment* p = getServerReqEnv() ) return p->getReqVarPtr(); else return 0; }

bool Barz::getReqVarValue( BarzerString& v, const char* n ) const
{
    v.clear();
    if( const RequestEnvironment* p = getServerReqEnv()  ) {
        if( const BarzelBeadAtomic_var*  var = p->getReqVar().getValue(n) )
            return BarzerAtomicCast(getUniverse()).convert(v,*var) < BarzerAtomicCast::CASTERR_FAIL;
        else
            return false;
    } 
    return false;
}
bool Barz::getReqVarValue( BarzelBeadAtomic_var& v, const char* n ) const
{
    if( const RequestEnvironment* p = getServerReqEnv() ) 
        return p->getReqVar().getValue(v,n);
    else 
        return false;
}
const char* Barz::getReqVarAsChars( const char* n ) const
{
    if( const RequestEnvironment* p = getServerReqEnv() ) {
        const BarzelBeadAtomic_var*  v =  p->getReqVar().getValue(n);
        if( v ) {
            if( const BarzerString* s = boost::get<BarzerString>(v) ) {
                return s->c_str();
            }
        }
    } 

    return 0;
        
}
bool Barz::hasReqVarEqualTo( const char* n, const char* val ) const
{
    if( const RequestEnvironment* p = getServerReqEnv() ) {
       const BarzelBeadAtomic_var*  v =  p->getReqVar().getValue(n);
       if( const BarzerString* s = boost::get<BarzerString>(v) ) 
           return (s->getStr()==val);
    }
    return false;
}
bool Barz::hasReqVarNotEqualTo( const char* n, const char* val ) const
{
    if( const RequestEnvironment* p = getServerReqEnv() ) {
       const BarzelBeadAtomic_var*  v =  p->getReqVar().getValue(n);
       if( const BarzerString* s = boost::get<BarzerString>(v) ) 
           return (s->getStr()!=val);
    }
    return false;
}
bool Barz::hasReqVar( const char* n) const
{ 
    if( const RequestEnvironment* p = getServerReqEnv() ) 
       return p->getReqVar().hasVar(n);
    else
        return false;
}
const BarzelBeadAtomic_var*  Barz::getReqVarValue( const char* n ) const
{
    if( const RequestEnvironment* p = getServerReqEnv() ) 
        return p->getReqVar().getValue(n);
    else 
        return 0;
}

void Barz::setReqVarValue( const char* n, const BarzelBeadAtomic_var& v )
{
    if( RequestEnvironment* p = getServerReqEnv() ) 
        p->getReqVar().setValue(n,v);
}

bool Barz::unsetReqVar( const char* n )
{
    if( RequestEnvironment* p = getServerReqEnv() ) 
        return p->getReqVar().unset(n);
    else return false;
        
}
///// extra normalization
int Barz::extraNormalization( const QuestionParm& qparm )
{
    return ay::unicode_normalize_punctuation( questionOrig );
}

std::string Barz::getPositionalQuestion() const
{
	std::string result;
	result.reserve(question.size());
	std::copy(question.begin(), question.end(), std::back_inserter(result));
	return result;
}

} // barzer namepace ends
