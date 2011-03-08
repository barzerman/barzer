#include <barzer_parse_types.h>
#include <barzer_parse.h>

namespace barzer {

void CToken::syncClassInfoFromSavedTok()
{
#warning implement CToken::syncClassInfoFromSavedTok
/// copy settings from storedToken's info sub-object into cInfo
}


std::ostream& TToken::print ( std::ostream& fp ) const
{
	if( !buf || !len ) {
		return ( fp << 0 <<':' );
	} else {
		std::string tmp( buf, len );
		return (fp << len << ':' << tmp);
	}
}

std::ostream& operator<<( std::ostream& fp, const TTWPVec& v )
{
	for( TTWPVec::const_iterator i = v.begin(); i!= v.end(); ++i ) 
		fp << *i << "\n";
	return fp;
}

int Barz::tokenize( QTokenizer& tokenizer, const char* q, const QuestionParm& qparm )
{
	/// invalidating all higher order objects
	puVec.clear();
	ctVec.clear();
	ttVec.clear();

	question.assign(q);
	return tokenizer.tokenize( ttVec, question.c_str(), qparm );
}

int Barz::classifyTokens( QLexParser& lexer, const QuestionParm& qparm )
{
	/// invalidating all higher order objects
	puVec.clear();
	ctVec.clear();

	return lexer.lex( ctVec, ttVec, qparm );
}

void Barz::clear()
{
	puVec.clear();
	ctVec.clear();
	ttVec.clear();

	question.clear();
}

int Barz::semanticParse( QSemanticParser& sem, const QuestionParm& qparm )
{
	/// invalidating all higher order objects
	puVec.clear();

	return sem.semanticize( puVec, ctVec, qparm );
}

} // barzer namespace 
