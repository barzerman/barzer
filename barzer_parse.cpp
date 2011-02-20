#include <barzer_parse.h>


using namespace barzer;

int QSemanticParser::semanticize( PUWPVec& pv, const CTWPVec& cv )
{
	err.clear();
	return 0;
}

int QParser::parse( Barz& barz, const char* q )
{
	barz.clear();

	err.tokRc = barz.tokenize( tokenizer, q ) ;
	err.lexRc = barz.classifyTokens( lexer );
	err.semRc = barz.semanticParse( semanticizer );
	
	return 0;
}
