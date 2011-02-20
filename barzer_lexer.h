#ifndef BARZER_LEXER_H
#define BARZER_LEXER_H
#include <barzer_parse_types.h>

namespace barzer {

/// converts input const char* quesion 
/// into vector of TToken with position (token's number in the input)
/// blanks will be there as blank tokens
/// tokenizes on any and all punctuation 
class QTokenizer {
public:
	struct Error : public QPError { } err;
	int tokenize( TTWPVec& , const char* );	
};

//// Lexical parser - it classifies TTokens and 
//// manipulates the resulting vector of CTokens
class QLexParser {
public:
	struct Error : public QPError { } err;
	virtual int lex( CTWPVec& , const TTWPVec& );
};
}

#endif // BARZER_LEXER_H
