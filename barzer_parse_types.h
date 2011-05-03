#ifndef BARZER_PARSE_TYPES_H
#define BARZER_PARSE_TYPES_H

#include <ay/ay_headers.h>
#include <vector>
#include <map>
#include <list>
#include <iostream>
#include <stdio.h>
#include <barzer_token.h>
#include <barzer_basic_types.h>
#include <barzer_storage_types.h>

namespace barzer {
struct TToken ;
struct CToken;
struct PUnit ;
class Barz ;

// global constnts 
enum {
	BGC_MAX_TOKLEN = 1024, // maximum length of token
	BGC_MAX_NUMTOK = 1024 // maximum number of tokens in the question
}; 

struct TToken {
	uint16_t  len; 
	const char* buf;
			
	TToken( ) : len(0), buf("") {}
	TToken( const char* s ) : len( strlen(s) ), buf(s) {}
	// watch out for data consistency
	TToken( const char* s, short l ) : len(l), buf(s) {}
	
	int getPunct() const
		{ return( buf && len ? buf[0] : ((int)'|') ); }

	std::ostream& print( FILE* fp ) const;
	std::ostream& print( std::ostream& fp ) const;
};

inline std::ostream& operator<<( std::ostream& fp, const TToken& t ) { return (t.print(fp)); }
// Original TToken and the position of this TToken in the original question
typedef std::pair< TToken, uint16_t > TTokenWithPos;
typedef std::vector< TTokenWithPos > TTWPVec;
inline std::ostream& operator<<( std::ostream& fp, const TTokenWithPos& t ) { 
	return ( fp << t.second << "{" << t.first << "}" );
}
std::ostream& operator<<( std::ostream& fp, const TTWPVec& v );
		struct StoredToken;

/// classified token
struct CToken {
	TTWPVec qtVec; // input question tokens corresponding to this CToken 
	CTokenClassInfo cInfo;
	TokenLinguisticInfo ling; 

	BarzerNumber bNum; // in cse this classifies as number

	const StoredToken* storedTok;

	void clear() {
		qtVec.clear();
		cInfo.clear();
		ling.clear();
		bNum.clear();
	}

	size_t getTokenSpan() const
	{
		uint16_t minPos = 0, maxPos= 0;
		for( TTWPVec::const_iterator i = qtVec.begin(); i!= qtVec.end(); ++i ) {
			if( i->second < minPos ) 
				minPos = i->second;
			if( i->second > maxPos ) 
				maxPos = i->second;
		}
		return( ( maxPos >= minPos )  ? (maxPos-minPos+1) : 0 );
	}

	void setCInfoBit( size_t b ) { cInfo.bitFlags.set(b); }

	/// sets qtVec to a single TToken
	inline void  setTToken( const TToken& tt, uint16_t pos ) {
		qtVec.reserve(1);
		qtVec.resize(1);
		qtVec[0].first = tt;
		qtVec[0].second = pos;
	}
	
	/// updates info from currently attached saved token
	void syncClassInfoFromSavedTok();
	void setClass(CTokenClassInfo::TokenClass_t c)
		{ cInfo.theClass = (typeof(cInfo.theClass))(c); } 
	CToken( ) : 
		storedTok(0) {}
	std::ostream& print( std::ostream& fp ) const;
	
	std::ostream& printQtVec( std::ostream& fp ) const;

	int getCTokenClass( ) const { return cInfo.theClass; }

	bool isBlank() const { return cInfo.theClass == CTokenClassInfo::CLASS_BLANK; }
	bool isNumber() const { return cInfo.theClass == CTokenClassInfo::CLASS_NUMBER; }
	bool isMysteryWord() const { return cInfo.theClass == CTokenClassInfo::CLASS_MYSTERY_WORD; }
	bool isWord() const { return cInfo.theClass == CTokenClassInfo::CLASS_WORD; }
	bool isPunct() const { return cInfo.theClass == CTokenClassInfo::CLASS_PUNCTUATION; }
	bool isPunct(char c) const { 
		return ( isPunct() && qtVec.size() == 1 && qtVec[0].first.buf[0] == c );
	}
	bool isSpace() const { return cInfo.theClass == CTokenClassInfo::CLASS_SPACE; }
	
	void setNumber( int i ) {
		cInfo.theClass = CTokenClassInfo::CLASS_NUMBER;
		bNum.set(i);
	}
	void setNumber( double x ) {
		cInfo.theClass = CTokenClassInfo::CLASS_NUMBER;
		bNum.set(x);
	}
	/// 
	uint32_t getStringId() const 	
		{ return( isWord() && storedTok ? storedTok->getSingleTokStringId() : 0xffffffff ) ; }
	
	const TToken*  getFirstTToken() const { return ( qtVec.size() ? &(qtVec.front().first) : 0 ); }
	int   getPunct( ) const
		{ const TToken* t = getFirstTToken(); return ( t ? t->getPunct() : 0 ); }
	const TTWPVec& getTTokens() const { return qtVec; }
	const BarzerNumber& getNumber() const { return bNum; }
};

typedef std::pair< CToken, uint16_t > CTokenWithPos;
typedef std::vector< CTokenWithPos > CTWPVec;

std::ostream& operator<<( std::ostream & , const CTWPVec& );
inline std::ostream& operator<<( std::ostream& fp, const CToken& t)
	{ return t.print( fp ); }

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

//// comes with every question and is passed to lexers and semanticizers 
struct QuestionParm {
	int lang;  // a valid LANG ID see barzer_language.h
	QuestionParm() : lang(0) {}
};

/// non constant string - it's different from barzer literal as the value may not be 
/// among the permanently stored but rather something constructed by barzel
class BarzerString {
	std::string str;
public:
	void setFromTTokens( const TTWPVec& v );

	const std::string& getStr() const { return str; }

	void setStr(const std::string &s) {	str = s; }
	void setStr(const char *s) { str = s; }

	std::ostream& print( std::ostream& fp ) const
		{ return ( fp << "\"" <<str<< "\"" ); }
};
inline std::ostream& operator <<( std::ostream& fp, const BarzerString& x )
	{ return( x.print(fp) ); }
} // barzer  namespace 
#endif // BARZER_PARSE_TYPES_H
