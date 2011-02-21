#include <barzer_parse_types.h>
#include <barzer_parse.h>

namespace barzer {

std::ostream& TToken::print ( std::ostream& fp ) const
{
	if( !buf || !len ) {
		return ( fp << "" );
	} else {
		std::string tmp( buf, len );
		return (fp << tmp);
	}
}

std::ostream& operator<<( std::ostream& fp, const TTWPVec& v )
{
	for( TTWPVec::const_iterator i = v.begin(); i!= v.end(); ++i ) 
		fp << *i << "\n";
	return fp;
}

int Barz::tokenize( QTokenizer& tokenizer, const char* q )
{
	/// invalidating all higher order objects
	puVec.clear();
	ctVec.clear();
	ttVec.clear();

	question.assign(q);
	return tokenizer.tokenize( ttVec, question.c_str() );
}

int Barz::classifyTokens( QLexParser& lexer )
{
	/// invalidating all higher order objects
	puVec.clear();
	ctVec.clear();

	return lexer.lex( ctVec, ttVec );
}

void Barz::clear()
{
	puVec.clear();
	ctVec.clear();
	ttVec.clear();
	question.clear();
}

int Barz::semanticParse( QSemanticParser& sem )
{
	/// invalidating all higher order objects
	puVec.clear();

	return sem.semanticize( puVec, ctVec );
}

} // barzer namespace 
