
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#pragma once
#include <ay/ay_headers.h>
#include <vector>
#include <map>
#include <list>
#include <iostream>
#include <stdio.h>
#include <barzer_token.h>
#include <barzer_number.h>
#include <barzer_entity.h>
#include <barzer_storage_types.h>
#include <barzer_question_parm.h>

namespace barzer {
struct StoredToken;
struct StoredEntity;
class StoredUniverse;
struct TToken ;
struct CToken;
struct PUnit ;
class Barz ;

struct TToken {
	uint16_t  d_utf8beg, d_utf8len; // starting glyphs and num of following glyphs for utf8  the string assumed to be elsewhere
    uint32_t  d_origOffset, d_origLength;  // offset and length in the original question 

	std::string buf;
    std::string normBuf; /// normalized buf (converted to lower case, de-asciied etc)
    	
    bool glyphsAreValid() const { return ( d_utf8beg!= 0xffff && d_utf8len != 0xffff ); }  

    size_t getFirstGlyph() const { return d_utf8beg; }
    size_t getNumGlyphs() const { return d_utf8len; }
    
    bool equal( const TToken& o ) const
    {
        return( d_origOffset == o.d_origOffset  && d_origLength == o.d_origLength );
    }
	TToken( ) : d_utf8beg(0xffff), d_utf8len(0xffff), d_origOffset(0), d_origLength(0), buf("") {}
	TToken( const char* s ) : d_utf8beg(0xffff), d_utf8len(0xffff), d_origOffset(0), d_origLength(0), buf(s) {}
	// watch out for data consistency
	TToken( const char* s, short l ) : d_utf8beg(0xffff), d_utf8len(0xffff), d_origOffset(0), d_origLength(0), buf(s,l) {}
	TToken( const char* s, short l, size_t bg, size_t ng, size_t o ) : 
        d_utf8beg(bg), d_utf8len(ng), d_origOffset(o), d_origLength(l), buf(s,l) 
    {}
	
	int getPunct() const
		{ return( buf.length() ? buf[0] : ((int)'|') ); }

    char isChar( ) { return ( buf.length() ==1 ? buf[0]:0 ); }

    TToken& setOrigOffsetAndLength( uint32_t offset, uint32_t length ) { return( d_origOffset=offset, d_origLength=length, *this ); }
    TToken& setOrigOffset( uint32_t offset) { return( d_origOffset=offset, *this ); }
    std::pair< uint32_t , uint32_t > getOrigOffsetAndLength() const { return std::make_pair(d_origOffset,d_origLength); }
    uint32_t getOrigOffset() const { return d_origOffset; }
    uint32_t getOrigLength() const { return d_origLength; }

    size_t getLen() const { return buf.length(); }

    bool is3Digits() const { return (buf.length() ==3 && isdigit(buf[0]) && isdigit(buf[1]) && isdigit(buf[2]) ); }
    bool is2Digits() const { return (buf.length() ==2 && isdigit(buf[0]) && isdigit(buf[1]) ); }
    bool is1Digit() const { return (buf.length() ==1 && isdigit(buf[0])); }

    bool is3OrLessDigits() const {  return( is3Digits() || is2Digits() || is1Digit() ); }
    bool isAllDigits() const {  
        for( const auto& i : buf ) { 
            if( !isdigit(i) ) return false; 
        }
        return true;
    }
    bool isSpace() const { return (buf.length() == 1&& isspace(buf[0]) ); }
    const char*  getBuf() const { return buf.c_str(); }
	std::ostream& print( FILE* fp ) const;
	std::ostream& print( std::ostream& fp ) const;

	enum {
		IS_ASCII,
		IS_16BYTE, /// alphabet has 16 byte chars
	};
	/// returns true if every character in the token is ASCII
	bool isAscii() const
		{ 
			for( const char* s = buf.c_str(); *s; ++s ) 
				if( !isascii(*s) ) return false; 

			return true;
		}
    
    size_t getNumChar( size_t bytesPerChar = 1 ) const 
    {
        return ( bytesPerChar<=1 ? buf.length(): (buf.length()/bytesPerChar) );
    }
};

inline std::ostream& operator<<( std::ostream& fp, const TToken& t ) { return (t.print(fp)); }
// Original TToken and the position of this TToken in the original question
typedef std::pair< TToken, uint16_t > TTokenWithPos;
typedef std::vector< TTokenWithPos > TTWPVec;
inline std::ostream& operator<<( std::ostream& fp, const TTokenWithPos& t ) { 
	return ( fp << t.second << "{" << t.first << "}" );
}
std::ostream& operator<<( std::ostream& fp, const TTWPVec& v );

/// classified token
struct CToken {
	TTWPVec qtVec; // input question tokens corresponding to this CToken 
	CTokenClassInfo cInfo;
	TokenLinguisticInfo ling; 

	typedef std::pair< std::string, std::string > StringPair;
	typedef std::vector< StringPair > SpellCorrections;

	BarzerNumber bNum; // in cse this classifies as number

	BarzerNumber& number() { return bNum; }
	const BarzerNumber& number() const { return bNum; }
	/// before:after corrections are stored here 
	SpellCorrections spellCorrections;

	const StoredToken* storedTok;
	// this can be 0. token resulting from stemming of stroedTok
	const StoredToken* stemTok; 
    
    std::string stem; 
	/// this may be blank 
	std::string correctedStr;

    bool equal( const CToken& o ) const
    {
        if( qtVec.size() == o.qtVec.size() )  {
            for( size_t i = 0; i< qtVec.size(); ++i ) {
                if( ! qtVec[i].first.equal( o.qtVec[i].first ) ) 
                    return false;
            }
            return true;
        }
        return false;
    }
    const StoredToken*  getStemTok() const { return stemTok; }
    void                setStemTok(const StoredToken* t) { stemTok=t; }
    bool                stemTokSameAsStored() const { return (storedTok == stemTok); }
    void                syncStemAndStoredTok(const StoredUniverse& u) ;
    uint32_t            getStemTokStringId() const { return ( stemTok ? stemTok->stringId : 0xffffffff ); }

	void clear() {
		qtVec.clear();
		cInfo.clear();
		ling.clear();
		bNum.clear();
        stem.clear();
        stemTok = storedTok= 0;
	}

	const SpellCorrections& getSpellCorrections() const { return spellCorrections; }

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
		{ cInfo.theClass = (int16_t)(c); } 

	CToken( ) : storedTok(0), stemTok(0) {}

    // this is for autocomplete only. do not use it 
    void setStoredTok_raw( const StoredToken* st ) 
        { 
            storedTok = st;
            stemTok = 0;
            cInfo.theClass = CTokenClassInfo::CLASS_WORD;
        }
	std::ostream& print( std::ostream& fp ) const;
	std::ostream& printQtVec( std::ostream& fp ) const;

	int getCTokenClass( ) const { return cInfo.theClass; }

    bool mayBeANumber() const 
        { return (cInfo.theClass == CTokenClassInfo::CLASS_NUMBER|| (qtVec.size() == 1 && qtVec.front().first.is3OrLessDigits())); }
	bool isBlank() const { return cInfo.theClass == CTokenClassInfo::CLASS_BLANK; }
	bool isNumber() const { return cInfo.theClass == CTokenClassInfo::CLASS_NUMBER; }
	bool isMysteryWord() const { return cInfo.theClass == CTokenClassInfo::CLASS_MYSTERY_WORD; }
	bool isWord() const { return cInfo.theClass == CTokenClassInfo::CLASS_WORD; }
	bool isPunct() const { return cInfo.theClass == CTokenClassInfo::CLASS_PUNCTUATION; }

    bool isString() const {
        return( 
            cInfo.theClass == CTokenClassInfo::CLASS_WORD ||
            cInfo.theClass == CTokenClassInfo::CLASS_MYSTERY_WORD
        );
    }
	bool isPunct(const char* s) const { return ( isPunct() && qtVec.size() == 1 && strchr(s,qtVec[0].first.buf[0]) ); }
    char  getPunct() { return ( (isPunct() && qtVec.size() == 1) ? qtVec[0].first.buf[0] : 0 ) ; }

	bool isSpace() const { return cInfo.theClass == CTokenClassInfo::CLASS_SPACE; }
	bool isPunct(char c) const { return ( (isPunct() ||(c==' ' && isSpace()) )  && qtVec.size() == 1 && qtVec[0].first.buf[0] == c ); }
    char isChar() const { return ( qtVec.size() == 1 && qtVec[0].first.buf.size() == 1 ? qtVec[0].first.buf[0] : 0 ); }
	
	void setSpellCorrected( bool v = true ) { cInfo.setSpellCorrected(v); }
	void addSpellingCorrection(const char* wrong, const char*  correct, const StoredUniverse& uni);

	void setStemmed( bool v = true ) { cInfo.setStemmed(v); }
	bool isSpellCorrected( ) const { return cInfo.isSpellCorrected(); }
	bool isStemmed( ) const { return cInfo.isStemmed(); }

	void setNumber( const BarzerNumber& n ) {
		cInfo.theClass = CTokenClassInfo::CLASS_NUMBER;
		bNum = n;
	}
	void setNumber( int i ) {
		cInfo.theClass = CTokenClassInfo::CLASS_NUMBER;
		bNum.set(i);
	}
	void setNumber( double x ) {
		cInfo.theClass = CTokenClassInfo::CLASS_NUMBER;
		bNum.set(x);
	}

    size_t isAllDigits() const { 
        return ( cInfo.theClass==CTokenClassInfo::CLASS_NUMBER || (qtVec.size() ==1 && qtVec.front().first.isAllDigits() ) ); 
    }
    bool is3Digits() const { return ( (qtVec.size() ==1 && qtVec.front().first.is3Digits() ) ); }
    bool trySetBNum( ) { 
        if( cInfo.theClass != CTokenClassInfo::CLASS_NUMBER ) {
            if( qtVec.size() ==1 && qtVec.front().first.isAllDigits() ) {
                bNum.set( atoi(qtVec.front().first.buf.c_str()) ); 
                bNum.setAsciiLen( qtVec.front().first.buf.length() );
                return true;
            }
            return false;
        } else 
            return true;
    }
    void setBNum_int( const char* x ) { bNum.set( atoi(x) ); }

	const StoredToken* getStoredTok() const { return storedTok; }
	/// 
	/*
	uint32_t getStringId() const 	
		{ return( isWord() && storedTok ? storedTok->getSingleTokStringId() : 0xffffffff ) ; }
	*/	
	const TToken*  getFirstTToken() const { return ( qtVec.size() ? &(qtVec.front().first) : 0 ); }
	int   getPunct( ) const
		{ const TToken* t = getFirstTToken(); return ( t ? t->getPunct() : 0 ); }
	const TTWPVec& getTTokens() const { return qtVec; }
	const BarzerNumber& getNumber() const { return bNum; }
};

typedef std::pair< CToken, uint16_t > CTokenWithPos;
typedef std::vector< CTokenWithPos > CTWPVec;

inline std::ostream& CTWPVec_origTok_print( std::ostream& os,  const CTWPVec& ctoks )
{
	for( CTWPVec::const_iterator ci = ctoks.begin(); ci != ctoks.end(); ++ci ) {
		const TTWPVec& ttv = ci->first.getTTokens();
		for( TTWPVec::const_iterator ti = ttv.begin(); ti!= ttv.end() ; ++ti ) {
			const TToken& ttok = ti->first;
			if( ttok.buf.length() ) {
				os.write( ttok.buf.c_str(), ttok.buf.length() );
			}
		}
	}	
	return os << ' ';
}

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

/// non constant string - it's different from barzer literal as the value may not be 
/// among the permanently stored but rather something constructed by barzel
class BarzerString {
	std::string str, d_stemStr;
    uint32_t d_stemStringId;
    enum {
        T_NORMAL,
        T_FLUFF
    };
    int d_type;
public:
    bool isEqual( const BarzerString& o ) const { return (d_type == o.d_type && o.d_stemStr == o.d_stemStr); }
    bool isLessThan( const BarzerString& o ) const { return (d_type < o.d_type && o.d_stemStr < o.d_stemStr); }

	BarzerString() : d_stemStringId(0xffffffff), d_type(T_NORMAL) {}
	BarzerString(const char* s) : str(s), d_stemStringId(0xffffffff), d_type(T_NORMAL)  {}
	BarzerString(const char* s,size_t s_len) : str(s,s_len), d_stemStringId(0xffffffff), d_type(T_NORMAL)  {}
    BarzerString( const std::string& s ) : str(s), d_stemStringId(0xffffffff), d_type(T_NORMAL) {}

	BarzerString(const char* s,size_t s_len, bool isFluff ) : str(s,s_len), d_stemStringId(0xffffffff), d_type(isFluff? T_NORMAL:T_FLUFF )  {}

    void clear() { d_type= T_NORMAL; d_stemStringId=0xffffffff; str.clear(); }
    bool isFluff() const { return (d_type == T_FLUFF); }
    BarzerString& setFluff()     { d_type = T_FLUFF; return *this; }
    void setNormal()    { d_type = T_NORMAL; }

	void setFromTTokens( const TTWPVec& v );

	const std::string& stemStr() const { return d_stemStr; }
    std::string& stemStr() { return d_stemStr; }

	const std::string& getStr() const { return str; }
    std::string& getStr() { return str; }
    char operator[]( const size_t i ) const { return ( (i<str.length()) ? str[i]: 0 ); }

    const char* c_str() const { return str.c_str(); }
    size_t length() const { return str.length(); }
	void setStr(const std::string &s) {	str = s; }
	void setStr(const char *s) { str = s; }
	void setStr(const char *s,size_t s_len) { str.assign(s,s_len); }
    BarzerString& assign( const char* s ) 
        { return ( str.assign(s), *this ); }
    BarzerString& operator =( const char* s ) { return assign(s); }
    BarzerString& operator =( const std::string& s ) { return ( str=s,*this); }

    const char* find_char( char c ) const {
        auto x = str.find(c);
        return( x == std::string::npos ? 0: str.c_str()+x );
    }
	std::ostream& print( std::ostream& fp ) const
		{ return ( fp << "\"" <<str<< "\"" ); }
    void setStemStringId( uint32_t i ) { d_stemStringId = i; }
    uint32_t getStemStringId() const { return d_stemStringId; }
    
    
};
inline bool operator<( const BarzerString& l, const BarzerString& r )
    { return l.isLessThan(r); }
inline bool operator==( const BarzerString& l, const BarzerString& r )
    { return l.isEqual(r); }

inline std::ostream& operator <<( std::ostream& fp, const BarzerString& x )
	{ return( x.print(fp) ); }
} // barzer  namespace 
