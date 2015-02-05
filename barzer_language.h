
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#pragma once
#include <arch/barzer_arch.h>
#include <ay/ay_headers.h>
#include <ay/ay_char.h>
#include <ay_snowball.h>
#include <barzer_parse_types.h>

/// language specific logic 
namespace ay { class LevenshteinEditDistance; }
namespace barzer {

#undef LANG_ENGLISH
#undef LANG_RUSSIAN
#undef LANG_SPANISH
#undef LANG_FRENCH

enum BarzerLangEnum
{
	LANG_UNKNOWN=-1,
	LANG_ENGLISH,
	LANG_RUSSIAN,

	LANG_UNKNOWN_UTF8,
	LANG_SPANISH,
	LANG_FRENCH,
	/// add new language after this only
    LANG_JAPANESE,
    LANG_CHINESE, // simplified

	LANG_MAX
};

inline BarzerLangEnum fromAyLang (int lang)
{
	switch (lang)
	{
	case ay::StemWrapper::LG_ENGLISH:
		return LANG_ENGLISH;
	case ay::StemWrapper::LG_SPANISH:
		return LANG_SPANISH;
	case ay::StemWrapper::LG_FRENCH:
		return LANG_FRENCH;
	default:
		return LANG_UNKNOWN_UTF8;
	}
}

struct WordLangInfo {
    bool hasNonLowerCase;
    WordLangInfo() : hasNonLowerCase(false) {}
};
class StoredUniverse;
/// Lang class determines language of the character/string see getLang function. all methods in it should be static  and inline when possible if they deal with strings 
struct Lang {
    static const char* getLangName( int xx ) ;

    inline static bool utf8_2byte_isRussian( unsigned char u0, unsigned char u1  )
    { return ( (u0 == 0xd0 && (u1== 0x81 ||(u1 >= 0x90 && u1<= 0xbf) ))|| (u0 == 0xd1 && (u1 >= 0x80 && u1<= 0x91 )) ); }

    inline static int getLangUtf8(  unsigned char u0, unsigned char u1 ) 
    {
        if( utf8_2byte_isRussian(u0,u1) )
            return LANG_RUSSIAN;
        else 
            return LANG_UNKNOWN_UTF8;
    }
    inline static bool isTwoByteLang(int lang) { return ( lang == LANG_RUSSIAN ); }
    inline static bool isUtf8Lang(int lang) { return ( lang == LANG_UNKNOWN_UTF8 ); }
    inline static bool isEnglish(int lang) { return (lang == LANG_ENGLISH); }

    static int getLangNoUniverse( const char* str, size_t s_len );
    static int getLangAndLengthNoUniverse( size_t& numGlyphs, const char* str, size_t s_len );
    static size_t getNumGlyphs( int lang, const char* str, size_t s_len );
    static int getLang( const StoredUniverse&, const char* str, size_t s_len );
    static bool convertTwoByteToLower( char* s, size_t s_len, int lang );
    /// returns true if diacritics were found
    /// only call it if s_len is an even number
    static bool hasTwoByteDiacritics( const char*, size_t s_len, int lang );
    static bool twoByteStripDiacritics( std::string& dest, const char*, size_t s_len, int lang );
    static bool convertUtf8ToLower( char* s, size_t s_len, int lang );

    static bool hasTwoByteUpperCase( const char* s, size_t s_len, int lang );
    static bool hasUpperCase( const char* s, size_t s_len, int lang );
    static bool stringToLower( std::vector<char>& buf, std::string& dest, const std::string& src );

    static bool stringToLower( std::string& dest, const std::string& src );

    /// converts both to lower case and returns strcmp
    static int stringNoCaseCmp( const std::string& l, const std::string& r );

    static bool stringToLower( char* s, size_t s_len, int lang );
    static bool convertToLower( char* s, size_t s_len, int lang )
        { return stringToLower(s,s_len,lang); }
    
    static size_t getNumChars( const char* s, size_t s_len, int lang );
    static void   lowLevelNormalization( char* d, size_t d_len, const char* s, size_t s_sz );

    static size_t getLevenshteinDistance( ay::LevenshteinEditDistance& lev, const char* s1, size_t s1_len, const char* s2, size_t s2_len ) ;
    static size_t getLevenshteinDistance( ay::LevenshteinEditDistance& lev, const std::string& s1, const std::string& s2 ) 
    {
        return getLevenshteinDistance( lev, s1.c_str(), s1.length(), s2.c_str(), s2.length() );
    }
    static BarzerLangEnum fromStringCode(const std::string&);
};

class QSingleLangLexer {
protected:
	int lang;
public:
	virtual int lex( CTWPVec& , const TTWPVec&, const QuestionParm& ) = 0;

	QSingleLangLexer( int lg ) : lang(lg) {}
	virtual ~QSingleLangLexer( ) {}

	struct Error : public QPError { } err;

	/// the factory method
	static QSingleLangLexer* 	mkLexer( int lg ); 
};

class QLangLexer {
	typedef std::vector< QSingleLangLexer* > LangLexVec;

	LangLexVec llVec;
public:
	QLangLexer() : llVec( LANG_MAX, 0 ) {}
	~QLangLexer();
	QSingleLangLexer* addLang( int lang );

	QSingleLangLexer* getLang( int lang ) {
		if (lang == LANG_UNKNOWN) lang = LANG_ENGLISH;
		return( lang >= 0 && lang<(int)llVec.size() ? llVec[lang] : 0 );
	}

	// routes lexing to an appropriate language specific lexer and lexes
	int lex( CTWPVec& , const TTWPVec&, const QuestionParm& );
};
///  
struct LangInfo {
    uint32_t counter;

    LangInfo() : counter(0) {}

    uint32_t  counterIncrement() { return ++counter; }
    void  counterClear() { counter = 0; }
    void clear() {
        counterClear();
    }
    uint32_t getCounter() const { return counter; }
};

struct LangInfoArray {
    LangInfo langInfo[ LANG_MAX +1 ];
    int defaultLang;
public:
    size_t getMaxLang() const { return sizeof(langInfo)/(sizeof(langInfo[0])); } 
    LangInfoArray() : defaultLang(LANG_UNKNOWN)
        { new(langInfo)(LangInfo[ LANG_MAX+1 ]); }
    bool isLangValid( int i ) const { return (i > LANG_UNKNOWN && i < LANG_MAX); }

    uint32_t incrementLangCounter( int16_t i )
        { return( isLangValid(i) ? langInfo[(i+1)].counterIncrement() : 0 ); }
    uint32_t getLangCounter(int i) const { return(isLangValid(i)? langInfo[i+1].getCounter() : 0) ; }

    int16_t getDominantLanguage() const;
    std::ostream& print(std::ostream&) const;

    bool hasLang( int16_t i ) const { return (getLangCounter(i)> 0); }
    void clear() 
    {
        for( LangInfo* i= langInfo, *i_end = i+getMaxLang(); i< i_end; ++i ) 
            i->clear();
    }
    void setDefaultLang(int lang) {
        defaultLang = lang;
    }
    int getDefaultLang() const { return defaultLang; }
};
std::ostream& operator<< ( std::ostream& fp, const LangInfo& );

class QSingleLangLexer_UTF8 : public QSingleLangLexer {
public:
QSingleLangLexer_UTF8() : QSingleLangLexer (LANG_UNKNOWN_UTF8) {}
int lex( CTWPVec& , const TTWPVec&, const QuestionParm& );
};

} // namespace barzer
