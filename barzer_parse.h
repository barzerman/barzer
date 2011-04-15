#ifndef BARZER_PARSE_H
#define BARZER_PARSE_H
#include <ay/ay_headers.h>
#include <barzer_lexer.h>
#include <barzer_el_matcher.h>

namespace barzer {
class StoredUniverse;
class QSemanticParser {
protected:
	const StoredUniverse& universe;
	BarzelMatcher barzelMatcher;
public:
	struct Error : public QPError { } err;

	virtual int semanticize( Barz&, const QuestionParm&  );
	QSemanticParser( const StoredUniverse& u ) : 
		universe(u),
		barzelMatcher(u)
	{}
	virtual ~QSemanticParser() {}
};

/// invokes tokenizer, lex parser and semantical parser 
/// to produce a valid Barze
class QParser {
protected:
	const StoredUniverse & universe;
	struct Error : public QPError { 
		int tokRc; // tokenizer rcode 
		int lexRc; // lexer rcode 
		int semRc; // semantic rcode 

		void clear() { 
			tokRc=lexRc=semRc=0;
			Error::clear();
		}
		Error() : tokRc(0), lexRc(0), semRc(0) {}
	} err;
public:
	QTokenizer tokenizer;
	QLexParser lexer;
	QSemanticParser semanticizer;
	
	QParser( const StoredUniverse& u );

	/// wipes out higher ctokens/punits and tokenizes q
	int tokenize_only( Barz& barz, const char* q, const QuestionParm& qparm );

	/// wipes out punits and repopulates ctokens
	int lex_only( Barz& barz, const QuestionParm& qparm );

	/// analyzes ctokens and repopulates punits
	int semanticize_only( Barz& barz, const QuestionParm& qparm );

	 
	/// tokenizes, classifies and semanticizes 
	int parse( Barz& barz, const char* q, const QuestionParm& qparm );
};

} // barzer namespace ends 

#endif // BARZER_PARSE_H
