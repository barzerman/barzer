#include <barzer_barz.h>
#include <barzer_parse.h>


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

void Barz::clear()
{
	beadChain.clear();
	ctVec.clear();
	ttVec.clear();

	question.clear();
}

int Barz::semanticParse( QSemanticParser& sem, const QuestionParm& qparm )
{
	/// invalidating all higher order objects
	beadChain.clear();

	return sem.semanticize( *this, qparm );
}

} // barzer namepace ends
