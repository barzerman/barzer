#include <barzer_parse.h>
#include <barzer_universe.h>
#include <barzer_el_matcher.h>
#include <barzer_barz.h>

using namespace barzer;
///////// semantic parser 
int QSemanticParser::semanticize( Barz& barz, const QuestionParm& qparm  )
{
	err.clear();
	universe.getToFirstTrie();
	/*
	BarzelMatcher matcher( universe );
	matcher.matchAndRewrite( barz );
	*/
	do {
		barzelMatcher.matchAndRewrite( barz );
	} while( universe.getToNextTrie() );
	universe.getToFirstTrie();
	return 0;
}

/// general parser 
QParser::QParser( const StoredUniverse& u ) : 
	universe(u),
	lexer(&(u.getDtaIdx())),
	semanticizer(u)
{}

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
	barz.clear();
	tokenize_only( barz, q, qparm );
	lex_only( barz, qparm );
	semanticize_only( barz, qparm );
	
	return 0;
}
