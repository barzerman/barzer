#ifndef BARZER_LEXER_H
#define BARZER_LEXER_H
#include <ay/ay_headers.h>
#include <barzer_parse_types.h>
#include <barzer_language.h>

namespace barzer {
class DtaIndex;
class StoredUniverse;

/// converts input const char* quesion 
/// into vector of TToken with position (token's number in the input)
/// blanks will be there as blank tokens
/// tokenizes on any and all punctuation 
class QTokenizer {
public:
	struct Error : public QPError { } err;
	int tokenize( TTWPVec& , const char*, const QuestionParm& );	
};

class BarzerHunspellInvoke;

//// Lexical parser - it classifies TTokens and 
//// manipulates the resulting vector of CTokens
class QLexParser {
	/// main purpose of lang lexer is to detect ranges 
	/// by running through some lang specific code
	QLangLexer langLexer;
	const DtaIndex* dtaIdx;
	const StoredUniverse& d_universe;

	void collapseCTokens( CTWPVec::iterator beg, CTWPVec::iterator end );

	enum { 
		// hunspell results with higher levenshteyn distance from original than this will be 
		/// discarded
		MAX_EDIT_DIST_FROM_SPELL = 2,  
		/// words shorter than this wont be spell corrected 
		MIN_SPELL_CORRECT_LEN = 4 
	};
	/// invoked from singleTokenClassify - tries to spell correct 
	/// fluffs the token if it's not correctable otherwise attempts to find something 
	/// returned by hunspell within the maximum Levenshteyn edit distance from t that resolves to 
	/// an actual domain token
	int trySpellCorrectAndClassify( CToken& ctok, TToken& ttok );

	/// resolves single tokens - this is not language specific
	int singleTokenClassify( CTWPVec& , TTWPVec&, const QuestionParm& );	
	/// multitoken non-language specific hardcoded stuff
	int advancedBasicClassify( CTWPVec& , const TTWPVec&, const QuestionParm& );	
	/// called from advancedBasicClassify
	/// tries to detect numbers with punctuation 1.5, -5.5 , 1,000,000 etc. 
	int advancedNumberClassify( CTWPVec& , const TTWPVec&, const QuestionParm& );	

	bool tryClassify_number( CToken&, const TToken&  ) const;
	bool tryClassify_integer( CToken&, const TToken&  ) const;
public:
	QLexParser( const StoredUniverse& u, const DtaIndex * di=0) : dtaIdx(di), d_universe(u) {}
	enum {
		QLPERR_NULL_IDX =1 // dtaIdx object is null
	}; 
	void setDtaIndex(const DtaIndex * di) { dtaIdx= di; }
	struct Error : public QPError { } err;
	virtual int lex( CTWPVec& , TTWPVec&, const QuestionParm& );
	virtual ~QLexParser() {}
};
}

#endif // BARZER_LEXER_H
