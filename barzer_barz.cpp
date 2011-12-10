#include <barzer_barz.h>
#include <barzer_parse.h>
#include <barzer_el_chain.h>
#include <barzer_universe.h>


namespace barzer {

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

void Barz::syncQuestionFromTokens()
{
	size_t qLen = 0;
	for( TTWPVec::const_iterator i = ttVec.begin(); i!= ttVec.end(); ++i ) {
		qLen+= (i->first.len+1);
	}
	if( !qLen ) 
		return ;
	question.clear();
	question.resize( qLen );
	size_t pos = 0;
	for( TTWPVec::iterator i = ttVec.begin(); i!= ttVec.end(); ++i ) {
		TToken& t = i->first;
		char* pBuf = &(question[pos]);
		memcpy( pBuf, t.buf, t.len );
		t.buf = pBuf;
		pBuf[t.len] = 0;
		pos+= (t.len+1);
	}
	
}

int Barz::tokenize( QTokenizer& tokenizer, const char* q, const QuestionParm& qparm )
{
	/// invalidating all higher order objects
	beadChain.clear();
	ctVec.clear();
	ttVec.clear();

	questionOrig.assign(q);
	int rc = tokenizer.tokenize( ttVec, questionOrig.c_str(), qparm );
	
	syncQuestionFromTokens();
	return rc;
}

int Barz::classifyTokens( QLexParser& lexer, const QuestionParm& qparm )
{
	/// invalidating all higher order objects
	beadChain.clear();
	ctVec.clear();

	return lexer.lex( ctVec, ttVec, qparm );
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
}

int Barz::chainInit( const QuestionParm& qparm ) 
{
	beadChain.init(ctVec);
    return 0;
}

int Barz::analyzeTopics( QSemanticParser& sem, const QuestionParm& qparm )
{
	beadChain.init(ctVec);
    return sem.analyzeTopics( *this, qparm );
}
int Barz::semanticParse( QSemanticParser& sem, const QuestionParm& qparm )
{
	/// invalidating and initializing all higher order objects
    if( sem.needTopicAnalyzis() ) {
        analyzeTopics( sem, qparm );
        clearBeads(); // we don't need to tokenize again really - just purge the beads  
    }
	beadChain.init(ctVec);

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
    /// end of figuring out whether we need anything 

    return 0;
}

} // barzer namepace ends
