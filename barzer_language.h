#ifndef BARZER_LANGUAGE_H  
#define BARZER_LANGUAGE_H  

#include <ay/ay_headers.h>
#include <barzer_parse_types.h>

/// language specific logic 
namespace barzer {

enum {
    LANG_UNKNOWN=-1,
	LANG_ENGLISH,
	LANG_RUSSIAN,
	LANG_SPANISH,
	LANG_FRENCH,

	LANG_MAX
};

/// Lang class determines language of the character/string see getLang function. all methods in it should be static  and inline when possible if they deal with strings 
struct Lang {
    static inline const char* getLangName( int xx ) 
        {
            if( xx == LANG_ENGLISH ) return "ENGLISH"; 
            else if( xx == LANG_RUSSIAN ) return "RUSSIAN";
            else
                return "UNKNOWN";
        }

    inline static bool utf8_2byte_isRussian( unsigned char u0, unsigned char u1  )
    { return ( (u0 == 0xd0 && (u1 >= 0x90 && u1<= 0xbf ))|| (u0 == 0xd1 && (u1 >= 0x80 && u1<= 0x8f )) ); }

    inline static int getLang2Byte(  unsigned char u0, unsigned char u1 ) 
    {
        if( utf8_2byte_isRussian(u0,u1) )
            return LANG_RUSSIAN;
        else 
            return LANG_UNKNOWN;
    }

    static int getLang( const char* str, size_t s_len )
    {
        const char* s_end = str+s_len, *s_end_1 = s_end + s_len-1;
        int lang = LANG_UNKNOWN;
        for( const char* s= str; *s && s< s_end; ++s ) {
            if( isascii(*s) ) {
                if( lang>LANG_ENGLISH )  // ascii character and lang was non english
                    return LANG_UNKNOWN;
                else  if( lang == LANG_UNKNOWN )
                    lang = LANG_ENGLISH;
            } else {
                if( lang == LANG_ENGLISH ) {
                    return LANG_UNKNOWN;
                } else if(s< s_end_1) { // at least theres at least 1 char beore the end
                    int tmpLang =  getLang2Byte( (unsigned char)(s[0]), (unsigned char)(s[1]) );
                    if( tmpLang == LANG_UNKNOWN )  // unknown 2 byte utf8 character
                        return LANG_UNKNOWN;
                    else if (lang != tmpLang) { // known language character, different from lang
                        if( lang == LANG_UNKNOWN )  {// if lang was previously unknown 
                            lang = tmpLang;
                        } else                        // if this character is from a diff language than lang
                            return LANG_UNKNOWN;
                    }
                    ++s;
                } else // character is non ascii and this is the last character 
                    return LANG_UNKNOWN;
            }
        }
        return lang;
    }
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

}
#endif // BARZER_LANGUAGE_H  
