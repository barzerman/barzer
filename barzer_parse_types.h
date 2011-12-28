#ifndef BARZER_PARSE_TYPES_H
#define BARZER_PARSE_TYPES_H

#include <ay/ay_headers.h>
#include <vector>
#include <map>
#include <list>
#include <iostream>
#include <stdio.h>
#include <barzer_token.h>
#include <barzer_number.h>
#include <barzer_entity.h>

namespace barzer {
struct StoredToken;
struct StoredEntity;
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

	enum {
		IS_ASCII,
		IS_16BYTE, /// alphabet has 16 byte chars
	};
	/// returns true if every character in the token is ASCII
	bool isAscii() const
		{ 
			for( const char* s = buf; *s; ++s ) 
				if( !isascii(*s) ) return false; 

			return true;
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

	/// this may be blank 
	std::string correctedStr;

	void clear() {
		qtVec.clear();
		cInfo.clear();
		ling.clear();
		bNum.clear();
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

	bool isBlank() const { return cInfo.theClass == CTokenClassInfo::CLASS_BLANK; }
	bool isNumber() const { return cInfo.theClass == CTokenClassInfo::CLASS_NUMBER; }
	bool isMysteryWord() const { return cInfo.theClass == CTokenClassInfo::CLASS_MYSTERY_WORD; }
	bool isWord() const { return cInfo.theClass == CTokenClassInfo::CLASS_WORD; }
	bool isPunct() const { return cInfo.theClass == CTokenClassInfo::CLASS_PUNCTUATION; }
	bool isPunct(char c) const { return ( isPunct() && qtVec.size() == 1 && qtVec[0].first.buf[0] == c ); }

    bool isString() const {
        return( 
            cInfo.theClass == CTokenClassInfo::CLASS_WORD ||
            cInfo.theClass == CTokenClassInfo::CLASS_MYSTERY_WORD
        );
    }
	bool isPunct(const char* s) const { return ( isPunct() && qtVec.size() == 1 && strchr(s,qtVec[0].first.buf[0]) ); }
    char  getPunct() { return ( (isPunct() && qtVec.size() == 1) ? qtVec[0].first.buf[0] : 0 ) ; }

	bool isSpace() const { return cInfo.theClass == CTokenClassInfo::CLASS_SPACE; }
	
	void setSpellCorrected( bool v = true ) { cInfo.setSpellCorrected(v); }
	void addSpellingCorrection( const char* wrong, const char*  correct ) 
	{ 
		setSpellCorrected();
		spellCorrections.resize( spellCorrections.size() +1 ) ;
		spellCorrections.back().first.assign(wrong);
		spellCorrections.back().second.assign(correct);
		if( isMysteryWord() ) {
			correctedStr.assign( correct );
		}
	}
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
			if( ttok.len && ttok.buf ) {
				os.write( ttok.buf, ttok.len );
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

//// comes with every question and is passed to lexers and semanticizers 
struct QuestionParm {
	int lang;  // a valid LANG ID see barzer_language.h
    struct AutocParm {
        uint32_t trieClass, trieId;
        
        enum {
            TOPICMODE_RULES_ONLY,
            TOPICMODE_TOPICS_ONLY,
            TOPICMODE_RULES_AND_TOPICS
        };
        uint32_t topicMode;

        /// when this is not empty only entities whose class/subclass match anything in the vector, will 
        /// be reported by autocomplete - this vector will be very small 
        std::vector< StoredEntityClass > ecVec;

        bool hasSpecificTrie() const { return( trieClass != 0xffffffff && trieId != 0xffffffff ); }
        bool needOnlyTopic() const { return ( topicMode == TOPICMODE_TOPICS_ONLY ); }
        bool needOnlyRules() const { return ( topicMode == TOPICMODE_RULES_ONLY ); }
        bool needTopic() const { return ( topicMode == TOPICMODE_TOPICS_ONLY || topicMode == TOPICMODE_RULES_AND_TOPICS ); }
        bool needRules() const { return ( topicMode == TOPICMODE_RULES_ONLY || topicMode == TOPICMODE_RULES_AND_TOPICS ); }

        bool needBoth() const { return ( topicMode == TOPICMODE_RULES_AND_TOPICS ); }

        void clear() { trieClass= 0xffffffff; trieId= 0xffffffff;  topicMode=TOPICMODE_RULES_ONLY; }

        bool entFilter( const StoredEntityUniqId& euid ) const 
            {
              if( ecVec.size() ) {
                for( std::vector< StoredEntityClass >::const_iterator i = ecVec.begin(); i!= ecVec.end(); ++i ) { 
                    if( i->matchOther(euid.eclass) )
                        return true;
                }
                return false;
              } else 
                return true; 
            }
        /// parses a string "c1,s1|..."
        void parseEntClassList( const char* s)
        {
            const char* b = s, *pipe = strchr(b,'|'), *b_end = b+ strlen(b);
            StoredEntityClass ec;
            for( ; b &&*b; b= (pipe?pipe+1:0), (pipe = (b? strchr(b,'|'):0)) ) {
                ec.ec = atoi(b);
                const char *find_end = (pipe?pipe: b_end), *comma = std::find( b, find_end, ',' );
                ec.subclass = ( comma!= find_end ? atoi(comma+1): 0 );
                if( ec.isValid() ) ecVec.push_back( ec );
            }
        }

        AutocParm() : trieClass(0xffffffff), trieId(0xffffffff),  topicMode(TOPICMODE_RULES_ONLY)  {}
    } autoc;

    void clear() { autoc.clear(); }
	QuestionParm() : lang(0) {}
};

/// non constant string - it's different from barzer literal as the value may not be 
/// among the permanently stored but rather something constructed by barzel
class BarzerString {
	std::string str;
public:
	BarzerString() {}
	BarzerString(const char* s) : str(s) {}

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
