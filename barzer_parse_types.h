#ifndef BARZER_PARSE_TYPES_H
#define BARZER_PARSE_TYPES_H

#include <vector>
#include <map>
#include <iostream>
#include <stdio.h>

namespace barzer {
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
	TToken( const char* s ) : len( strlen(s) ), buf(s) {}
	// watch out for data consistency
	TToken( const char* s, short l ) : len(l), buf(s) {}

	std::ostream& print( FILE* fp ) const;
	std::ostream& print( std::ostream& fp ) const;
};

inline std::ostream& operator<<( std::ostream& fp, const TToken& t ) { return (t.print(fp)); }
// Original TToken and the position of this TToken in the original question
typedef std::pair< TToken, short > TTokenWithPos;
typedef std::vector< TTokenWithPos > TTWPVec;
inline std::ostream& operator<<( std::ostream& fp, const TTokenWithPos& t ) { 
	return ( fp << t.second << "{" << t.first << "}" );
}
std::ostream& operator<<( std::ostream& fp, const TTWPVec& v );

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

	enum {
		BIT_COMPOUNDED,
		BIT_FLUFF,
	};
	/// non-mutually exclusive binary properties
	int 		flags[2]; // bit mask of BIT_XXX

};

// linugistic information - part of speech, directionality etc
class TokenLinguisticInfo {
};

/// classified token
struct CToken {
	TTWPVec tVec;
	CTokenClassInfo cInfo;
	TokenLinguisticInfo ling; 
};

typedef std::pair< CToken, short > CTokenWithPos;
typedef std::vector< CTokenWithPos > CTWPVec;

struct PUnitClassInfo
{
	enum {
		PUCI_UNDEF, // undefined
		PUCI_FLUFF, // fluff words
		PUCI_GRAMMAR, // non-searcheable grammatical costruct
		PUCI_RANGE,   // range ("where clause")
		PUCI_TEXT     // searcheable text
	} puType;

	int puSubtype;
	PUnitClassInfo() :
		puType(PUCI_UNDEF),
		puSubtype(0) {}
};

/// generic QP (question parsing) error class 
/// tokenizer, lexparser and semantical parser all 
/// used types derived from this to store errors
struct QPError {
	int e;
	QPError(): e(0) {}
	virtual void clear() { e = 0; }
	virtual ~QPError() {}
};

///// parsing unit
struct PUnit  {
	CTWPVec cVec;
	PUnitClassInfo puClass;
};

typedef std::pair< PUnit, short > PUnitWithPos;
typedef std::vector< PUnitWithPos > PUWPVec;

class QTokenizer;
class LexParser;

// collection of punits and the original question
class Barz {
	/// copy of the original question
	/// all poistional info, pointers and offsets from everything
	/// contained in puVec refers to this string
	std::string question; 
	
	TTWPVec ttVec; 
	CTWPVec ctVec; 
	PUWPVec puVec;
	
	friend class QSemanticParser;
	friend class QLexParser;
	friend class QTokenizer;

	friend class QParser;
public:
	void clear();

	int tokenize( QTokenizer& , const char* q );
	int classifyTokens( QLexParser& );
	int semanticParse( QSemanticParser& );
};
} // barzer  namespace 
#endif // BARZER_PARSE_TYPES_H
