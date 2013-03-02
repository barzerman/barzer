
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#pragma once
#include <ay/ay_headers.h>
#include <ay/ay_utf8.h>
#include <barzer_parse_types.h>
#include <barzer_language.h>
#include <barzer_tokenizer.h>

namespace barzer {
class DtaIndex;
class StoredUniverse;

template<typename T>
struct PosedVec {
	T& d_vec;
    typedef typename T::value_type value_type;

	size_t d_pos;

	PosedVec (T& vec, size_t pos)
	    : d_vec(vec), d_pos(pos)
	{}

    size_t operator ++() { return(++d_pos); }

    size_t  pos() const { return d_pos; }

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

// internal parameters 
struct QLexParser_LocalParms;

//// Lexical parser - it classifies TTokens and
//// manipulates the resulting vector of CTokens
class QLexParser {
	/// main purpose of lang lexer is to detect ranges
	/// by running through some lang specific code
	QLangLexer langLexer;
	const DtaIndex* dtaIdx;
	const StoredUniverse& d_universe;
	
	bool m_dontSpell;

	void collapseCTokens( CTWPVec::iterator beg, CTWPVec::iterator end );
public:
	enum {
		/// discarded
		MAX_EDIT_DIST_FROM_SPELL = 2,
		/// words equal or shorter than this wont be spell corrected
		MIN_SPELL_CORRECT_LEN = 3,

		MAX_CTOKENS_PER_QUERY = 64*4
	};
    size_t d_maxCtokensPerQuery;
private:
    const StoredToken* getStoredToken(uint32_t& strId, const char* str) const;
    const StoredToken* getStoredToken(const char* str) const
    {
        uint32_t tmpId = 0xffffffff;
        return getStoredToken(tmpId, str );
    }
    // inlined in the cpp
    bool trySplitCorrect(
        SpellCorrectResult& corrResult, 
        QLexParser_LocalParms& parms ,
        bool isAutoc
    );
    /// 
    bool trySplitCorrectUTF8(
        SpellCorrectResult& corrResult, 
        QLexParser_LocalParms& parms ,
        bool isAutoc
    );

	/// invoked from singleTokenClassify - tries to spell correct
	/// fluffs the token if it's not correctable otherwise attempts to find something
	/// returned within the maximum Levenshteyn edit distance from t that resolves to
	/// an actual domain token
	SpellCorrectResult trySpellCorrectAndClassify (PosedVec<CTWPVec>, PosedVec<TTWPVec>, const QuestionParm& qparm);

	/// resolves single tokens - this is not language specific
	size_t retokenize( Barz&, const QuestionParm& );
	int singleTokenClassify( Barz&, const QuestionParm& );
	int singleTokenClassify_space( Barz&, const QuestionParm& );
	/// multitoken non-language specific hardcoded stuff
	int advancedBasicClassify( Barz&, const QuestionParm& );
	/// called from advancedBasicClassify
	/// tries to detect numbers with punctuation 1.5, -5.5 , 1,000,000 etc.
	int advancedNumberClassify( Barz&, const QuestionParm& );
    /// extra separator logic
	int advancedNumberClassify_separator( Barz&, const QuestionParm& );

    int separatorNumberGuess (Barz&, const QuestionParm&);

	bool tryClassify_number( CToken&, const TToken&  ) const;
	bool tryClassify_integer( CToken&, const TToken&  ) const;
	
public:
	void setDontSpell(bool dont) { m_dontSpell = dont; }
	
	QLexParser( const StoredUniverse& u, const DtaIndex * di=0) : dtaIdx(di), d_universe(u), m_dontSpell(false), d_maxCtokensPerQuery(MAX_CTOKENS_PER_QUERY) {}
    void setMaxCTokensPerQuery( size_t sz ) { d_maxCtokensPerQuery= sz; }

	enum {
		QLPERR_NULL_IDX =1 // dtaIdx object is null
	};
	void setDtaIndex(const DtaIndex * di) { dtaIdx= di; }
	struct Error : public QPError { } err;
	int lex( Barz&, const QuestionParm& );

	int lex( Barz& barz, const TokenizerStrategy& strat, QTokenizer& tokenizer, const char* q, const QuestionParm& qparm );
	virtual ~QLexParser() {}
};
} // namespace barzer
