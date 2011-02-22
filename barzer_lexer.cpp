#include <barzer_lexer.h>
#include <ctype.h>

namespace barzer {

int QLexParser::lex( CTWPVec& cVec, const TTWPVec& tVec, const QuestionParm& qparm )
{
	err.clear();
	langLexer.lex( cVec, tVec, qparm );
	return 0;
}

int QTokenizer::tokenize( TTWPVec& ttwp, const char* q, const QuestionParm& qparm  )
{
	err.clear();

	size_t origSize = ttwp.size(); 
	char lc = 0;
	const char* tok = 0;
	const char* s = q;
	for( s = q; *s; ++s ) {
		char c = *s;
		if( !isascii(c) ) {
			if( !tok ) 
				tok = s;
		} else
		if( isspace(c) ) {
			if( !lc || lc != c ) {
				if( tok ) {
					ttwp.push_back( TTWPVec::value_type( TToken(tok,s-tok), ttwp.size() ));
					tok = 0;
				}
				ttwp.push_back( 
						TTWPVec::value_type(
							TToken(s,1),
							ttwp.size() 
						)
				);
			}
		} else if( ispunct(c) ) {
			if( tok ) {
				ttwp.push_back( TTWPVec::value_type( TToken(tok,s-tok), ttwp.size() ));
				tok = 0;
			}
			ttwp.push_back( TTWPVec::value_type( TToken(s,1), ttwp.size() ));
		} else if( isalnum(c) ) {
			if( !tok ) 
				tok = s;
		}
	    lc=c;
	}
	if( tok ) {
		ttwp.push_back( TTWPVec::value_type( TToken(tok,s-tok), ttwp.size() ));
		tok = 0;
	}
	return ( ttwp.size() - origSize );
}

} // barzer namespace 
