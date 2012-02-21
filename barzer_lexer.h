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
	enum { MAX_QUERY_LEN = 1024, MAX_NUM_TOKENS = 96 };
	struct Error : public QPError { } err;
	int tokenize( TTWPVec& , const char*, const QuestionParm& );
};

class BarzerHunspellInvoke;

template<typename T>
struct PosedVec {
	T& d_vec;
    typedef typename T::value_type value_type;

	size_t d_pos;

	PosedVec (T& vec, size_t pos)
	    : d_vec(vec), d_pos(pos)
	{}

    size_t operator ++() { return(++d_pos); }

    size_t  pos() const { return 0; }

    value_type&             element() { return d_vec[d_pos]; } 
    const value_type&       element() const { return d_vec[d_pos]; } 

    typename T::iterator posIterator() { return (d_vec.begin() + d_pos); }
    T& vec() { return d_vec; }
    const T& vec() const { return d_vec; }
};

/*
template typename<>
inline PosedVec<CTWPVec>::pos() const { return d_pos; }
*/

struct SpellCorrectResult;

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
		MIN_SPELL_CORRECT_LEN = 4 ,

		MAX_CTOKENS_PER_QUERY = 64
	};

	/// invoked from singleTokenClassify - tries to spell correct
	/// fluffs the token if it's not correctable otherwise attempts to find something
	/// returned by hunspell within the maximum Levenshteyn edit distance from t that resolves to
	/// an actual domain token
	SpellCorrectResult trySpellCorrectAndClassify (PosedVec<CTWPVec>, PosedVec<TTWPVec>, const QuestionParm& qparm);

	/// resolves single tokens - this is not language specific
	int singleTokenClassify( Barz&, const QuestionParm& );	
	/// multitoken non-language specific hardcoded stuff
	int advancedBasicClassify( Barz&, const QuestionParm& );	
	/// called from advancedBasicClassify
	/// tries to detect numbers with punctuation 1.5, -5.5 , 1,000,000 etc. 
	int advancedNumberClassify( Barz&, const QuestionParm& );	

	bool tryClassify_number( CToken&, const TToken&  ) const;
	bool tryClassify_integer( CToken&, const TToken&  ) const;
public:
	QLexParser( const StoredUniverse& u, const DtaIndex * di=0) : dtaIdx(di), d_universe(u) {}
	enum {
		QLPERR_NULL_IDX =1 // dtaIdx object is null
	};
	void setDtaIndex(const DtaIndex * di) { dtaIdx= di; }
	struct Error : public QPError { } err;
	virtual int lex( Barz&, const QuestionParm& );
	virtual ~QLexParser() {}
};
}

#endif // BARZER_LEXER_H
