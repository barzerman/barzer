#include <barzer_tokenizer.h>

namespace barzer {

////////////// TOKENIZE
///////////////////////////////////

int QTokenizer::tokenize( const TokenizerStrategy& , TTWPVec& , CTWPVec&, const char*, const QuestionParm& )
{
    return 0;
}
int QTokenizer::tokenize( TTWPVec& ttwp, const char* q, const QuestionParm& qparm  )
{
	err.clear();

	size_t origSize = ttwp.size();
	char lc = 0;
	const char* tok = 0;
	enum {
		CHAR_UNKNOWN=0,
		CHAR_ALPHA,
		CHAR_DIGIT,
		CHAR_UTF8
	};
	int prevChar = CHAR_UNKNOWN;
#define PREVCHAR_NOT( t ) ( prevChar && (CHAR_##t != prevChar) )


	size_t q_len = strlen(q);
	if( q_len > MAX_QUERY_LEN ) {
		q_len = MAX_QUERY_LEN;
	}
	const char* s_end = q+ q_len;
	const char* s = q;
	for( ; s!= s_end; ++s ) {

		char c = *s;
		if( !isascii(c) ) {
			if( CHAR_DIGIT == prevChar || (!ay::is_diacritic(s) && PREVCHAR_NOT(UTF8)) ) {
				if( tok ) {
					ttwp.push_back( TTWPVec::value_type( TToken(tok,s-tok), ttwp.size() ));
					tok = 0;
				}
				--s;
			} else {
				if( !tok )
					tok = s;
			}
			prevChar = CHAR_UTF8;
		} else
		if( isspace(c) ) {
			if( !lc || lc != c ) {
				if( tok ) {
					ttwp.push_back( TTWPVec::value_type( TToken(tok,s-tok), ttwp.size() ));
					tok = 0;
				}
				ttwp.push_back( TTWPVec::value_type( TToken(s,1), ttwp.size() ));
			}
			prevChar = CHAR_UNKNOWN;
		} else if( ispunct(c) ) {
			if( tok ) {
				ttwp.push_back( TTWPVec::value_type( TToken(tok,s-tok), ttwp.size() ));
				tok = 0;
			}
			ttwp.push_back( TTWPVec::value_type( TToken(s,1), ttwp.size() ));
			prevChar = CHAR_UNKNOWN;
		} else if( isalnum(c) ) {
			if( (isalpha(c) && (PREVCHAR_NOT(ALPHA)&&PREVCHAR_NOT(UTF8)) ) ||
				(isdigit(c) && PREVCHAR_NOT(DIGIT) )
			) {
				if( tok ) {
					ttwp.push_back( TTWPVec::value_type( TToken(tok,s-tok), ttwp.size() ));
					tok = 0;
				}
				--s;
			} else  {
				if( !tok ) tok = s;
			}
			if( isdigit(c) ) {
				prevChar = CHAR_DIGIT;
			} else {
				prevChar =CHAR_ALPHA;
			}
		} else
			prevChar = CHAR_ALPHA;
	    lc=c;
	}
	if( tok ) {
		ttwp.push_back( TTWPVec::value_type( TToken(tok,s-tok), ttwp.size() ));
		tok = 0;
	}

	if( ttwp.size()  > MAX_NUM_TOKENS ) {
		ttwp.resize(MAX_NUM_TOKENS);
	}

	return ( ttwp.size() - origSize );
#undef PREVCHAR_NOT
}
} // namespace barzer 
