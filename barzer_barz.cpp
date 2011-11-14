#include <barzer_barz.h>
#include <barzer_parse.h>
#include <barzer_el_chain.h>


namespace barzer {

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
	BeadRange rng = beadChain.getFullRange();
	//// collapsing consecutive entities
	return 0;
}


} // barzer namepace ends
