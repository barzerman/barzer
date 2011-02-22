#include <barzer_parse.h>


using namespace barzer;

int QSemanticParser::semanticize( PUWPVec& pv, const CTWPVec& cv, const QuestionParm& qparm  )
{
	err.clear();
	return 0;
}

int QParser::parse( Barz& barz, const char* q, const QuestionParm& qparm )
{
	barz.clear();

	err.tokRc = barz.tokenize( tokenizer, q, qparm ) ;
	err.lexRc = barz.classifyTokens( lexer, qparm );
	err.semRc = barz.semanticParse( semanticizer, qparm );
	
	return 0;
}
