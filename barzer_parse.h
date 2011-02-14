
struct TToken ;
struct CToken;
struct PUnit ;
struct Barz ;

// global constnts 
enum {
	BGC_MAX_TOKLEN = 1024, // maximum length of token
	BGC_MAX_NUMTOK = 1024 // maximum number of tokens in the question
}; 

enum {
};

struct TToken {
	short  len; 
	const char* buf;
	
	TToken( ) : len(0), buf("") {}
	TToken( const char* s ) : 
		len( strlen(s) ), buf(s) {}
	// watch out for data consistency
	TToken( const char* s, short l ) : 
		len(l), buf(s) {}
};

// Original TToken and the position of this TToken in the original question
typedef std::pair< TToken, short > TTokenWithPos;
typedef std::vector< TTokenWithPos > TTWPVec;

class CTokenClassInfo {
	/// class bits
	typedef enum {
		CLASS_UNCLASSIFIED,
		CLASS_WORD,
		CLASS_NUMBER,
		CLASS_PUNCTUATION,
		CLASS_SPACE
	} MainClass_t;
	MainClass_t theClass;
	int         subclass; 

	typedef {
		BIT_COMPOUNDED,
		BIT_FLUFF,
	};
	/// non-mutually exclusive binary properties
	int 		flags[2]; // bit mask of BIT_XXX

};

// linugistic information - part of speech, directionality etc
class TokenLinguisticInfo {
};

class CToken {
	TTWPVec tVec;
	CTokenClassInfo cInfo;
	TokenLinguisticInfo ling; 
};
/////
class PUnit  {
};
// collection of punits
class Barz {
};
