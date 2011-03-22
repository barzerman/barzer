#include <barzer_parse.h>


using namespace barzer;
///////// semantic parser 
int QSemanticParser::semanticize( PUWPVec& pv, const CTWPVec& cv, const QuestionParm& qparm  )
{
	err.clear();
	return 0;
}

/// general parser 
int QParser::semanticize_only( Barz& barz, const QuestionParm& qparm )
{
	err.semRc = barz.semanticParse( semanticizer, qparm );
	return 0;
}

int QParser::lex_only( Barz& barz, const QuestionParm& qparm )
{
	err.lexRc = barz.classifyTokens( lexer, qparm );
	return 0;
}
int QParser::tokenize_only( Barz& barz, const char* q, const QuestionParm& qparm )
{
	err.tokRc = barz.tokenize( tokenizer, q, qparm ) ;
	return 0;
}


int QParser::parse( Barz& barz, const char* q, const QuestionParm& qparm )
{
	tokenize_only( barz, q, qparm );
	lex_only( barz, qparm );
	semanticize_only( barz, qparm );
	
	return 0;
}
