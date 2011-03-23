#ifndef BARZER_PARSE_H
#define BARZER_PARSE_H
#include <ay/ay_headers.h>
#include <barzer_lexer.h>

namespace barzer {

class QSemanticParser {
public:
	struct Error : public QPError { } err;

	virtual int semanticize( PUWPVec& , const CTWPVec&, const QuestionParm&  );
	virtual ~QSemanticParser() {}
};

/// invokes tokenizer, lex parser and semantical parser 
/// to produce a valid Barze
class QParser {
private:
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
