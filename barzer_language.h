#ifndef BARZER_LANGUAGE_H  
#define BARZER_LANGUAGE_H  

#include <arch/barzer_arch.h>
#include <ay/ay_headers.h>
#include <barzer_parse_types.h>

/// language specific logic 
namespace barzer {

#undef LANG_ENGLISH
#undef LANG_RUSSIAN
#undef LANG_SPANISH
#undef LANG_FRENCH

enum {
    LANG_UNKNOWN=-1,
	LANG_ENGLISH,
	LANG_RUSSIAN,
	LANG_SPANISH,
	LANG_FRENCH,
    /// add new language after this only
	LANG_MAX
};

/// Lang class determines language of the character/string see getLang function. all methods in it should be static  and inline when possible if they deal with strings 
struct Lang {
    static const char* getLangName( int xx ) ;

    inline static bool utf8_2byte_isRussian( unsigned char u0, unsigned char u1  )
    { return ( (u0 == 0xd0 && (u1== 0x81 ||(u1 >= 0x90 && u1<= 0xbf) ))|| (u0 == 0xd1 && (u1 >= 0x80 && u1<= 0x91 )) ); }

    inline static int getLang2Byte(  unsigned char u0, unsigned char u1 ) 
    {
        if( utf8_2byte_isRussian(u0,u1) )
            return LANG_RUSSIAN;
        else 
            return LANG_UNKNOWN;
    }
    inline static bool isTwoByteLang(int lang) { return ( lang == LANG_RUSSIAN ); }
    static int getLang( const char* str, size_t s_len );
    static bool convertTwoByteToLower( char* s, size_t s_len, int lang );
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
	QSingleLangLexer* addLang( size_t lang );

	QSingleLangLexer* getLang( size_t lang )
		{ return( lang<llVec.size() ? llVec[lang] : 0 ); }

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
public:
    size_t getMaxLang() const { return sizeof(langInfo)/(sizeof(langInfo[0])); } 
    LangInfoArray() 
        { new(langInfo)(LangInfo[ LANG_MAX+1 ]); }
    uint32_t incrementLangCounter( int16_t i )
        { return( i>= LANG_UNKNOWN && i< LANG_MAX ?  return langInfo[(i+1)].counterIncrement() : 0 ); }

    int16_t getDominantLanguage() 
    {
        for( int i = 0; i< getMaxLang(); ++i ) {
            
        }
    }
    std::ostream& print() const;
};
std::ostream& operator<< ( std::ostream& fp, const LangInfo& );

}
#endif // BARZER_LANGUAGE_H  
