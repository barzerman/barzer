#include <barzer_language.h>

#include <lg_ru/barzer_ru_lex.h>
#include <lg_en/barzer_en_lex.h>
#include <ay/ay_utf8.h>

namespace barzer {

namespace {
const char * rus_letter[] =  {
    "а","б","в","г","д","е","ж","з","и","й","к","л","м","н",
    "о","п","р","с","т","у","ф","х","ц","ч","ш","щ","ь","ы","ъ","э","ю","я"
};

inline bool russian_is_upper( const char* ss ) {
    uint8_t b1= (uint8_t)(ss[1]);
    return( (b1 >=0x90 && b1 <=0xaf) || ((uint8_t)(ss[0]) && b1 == 0x81) );
}

}

bool Lang::convertUtf8ToLower( char* s, size_t s_len, int lang )
{
	ay::StrUTF8 utf(s, s_len);
	if (!utf.toLower())
		return false;

	std::memcpy(s, utf.c_str(), s_len);
	return true;
}

bool Lang::convertTwoByteToLower( char* s, size_t s_len, int lang )
{
    bool hasUpperCase = false;
    if( lang == LANG_RUSSIAN ) {
        // all russian uppercase letters are 0xd0
        char* s_end = s+s_len;
        for( char* ss=s; ss< s_end; ss+=2 ) {
            uint8_t b1= (uint8_t)(ss[1]);
            if( (b1 >=0x90 && b1 <=0xaf) ) {
                const char* lc = rus_letter[ (b1-0x90) ];
                hasUpperCase = true;
                ss[0] = lc[0];
                ss[1] = lc[1];
            } else if( (uint8_t)(ss[0]) == 0xd0 && b1 == 0x81 ) {
                ss[0] = 0xd1;
                ss[1] = 0x91;
            }
        }

        return hasUpperCase;
    } else 
        return false;
}
size_t Lang::getNumChars( const char* s, size_t s_len, int lang )
{
    if( lang == LANG_ENGLISH) 
        return s_len;
    else if( lang == LANG_RUSSIAN ) 
        return (s_len/2);
    else {
        return ay::StrUTF8::glyphCount(s,s+s_len);
    }
}
bool Lang::stringToLower( char* s, size_t s_len, int lang )
{
    if( lang == LANG_ENGLISH ) {
        const char* s_end = s+s_len;
        bool hasUpperCase = false;
        for( char* i = s; i< s_end; ++i ) {
            if( isupper(*i) ) {
                if( !hasUpperCase ) 
                    hasUpperCase = true;
                *i = tolower(*i);
            }
        }
        return hasUpperCase;
    } else if( lang == LANG_UNKNOWN_UTF8 ) {
        return convertUtf8ToLower(s,s_len,lang);
    } else if( lang == LANG_RUSSIAN ) 
        return convertTwoByteToLower(s,s_len,lang);

    return false;
}

bool Lang::hasTwoByteUpperCase( const char* s, size_t s_len, int lang )
{
    if( lang == LANG_RUSSIAN ) {
        const  char* s_end = s+s_len;
        for( const char* ss=s; ss< s_end; ss+=2 ) {
            if( russian_is_upper(ss) )
                return true;
        } 
        return false;
    } else
        return false;
}
bool Lang::hasUpperCase( const char* s, size_t s_len, int lang )
{
	if( lang == LANG_ENGLISH ) {
		const char* s_end = s+s_len;
		for( ; s< s_end; ++s ) {
			if( isupper(*s) ) 
				return true;
		}
		return false;
	} else if( lang == LANG_RUSSIAN ) 
		return hasTwoByteUpperCase( s, s_len, lang );
	else
		return ay::StrUTF8::hasUpper(s, s_len);

	return false;
}


int Lang::getLang( const char* str, size_t s_len )
{
    const char* s_end = str+s_len, *s_end_1 = s_end + s_len-1;
    int lang = LANG_UNKNOWN;
    for( const char* s= str; *s && s< s_end; ++s ) {
        if( isascii(*s) ) {
            if( lang>LANG_ENGLISH )  // ascii character and lang was non english
                return LANG_UNKNOWN_UTF8;
            else  if( lang == LANG_UNKNOWN )
                lang = LANG_ENGLISH;
        } else {
            if( lang == LANG_ENGLISH ) {
                return LANG_UNKNOWN_UTF8;
            } else if(s< s_end_1) { // at least theres at least 1 char beore the end
                int tmpLang =  getLangUtf8( (unsigned char)(s[0]), (unsigned char)(s[1]) );
                if( tmpLang == LANG_RUSSIAN )  {
                    if( lang == LANG_UNKNOWN ) 
                        lang = tmpLang;
                    else if( lang != LANG_RUSSIAN ) 
                        return LANG_UNKNOWN_UTF8;
                } else // utf8 character
                    return LANG_UNKNOWN_UTF8;
                ++s;
            } else // character is non ascii and this is the last character 
                return LANG_UNKNOWN_UTF8;
        }
    }
    return ( lang == LANG_UNKNOWN ? LANG_UNKNOWN_UTF8 : lang );
}

const char* Lang::getLangName( int xx ) 
{
    if( xx == LANG_ENGLISH ) return "ENGLISH"; 
    else if( xx == LANG_RUSSIAN ) return "RUSSIAN";
    else if( xx == LANG_UNKNOWN_UTF8 ) return "UTF8";
    else
        return "UNKNOWN";
}
int QSingleLangLexer_UTF8::lex( CTWPVec& , const TTWPVec&, const QuestionParm& )
{
    return 0;
}

/// the factory method
QSingleLangLexer* 	QSingleLangLexer::mkLexer( int lg )
{
	switch( lg ) {
	case LANG_ENGLISH:
		return new QSingleLangLexer_EN();
	case LANG_RUSSIAN:
		return new QSingleLangLexer_RU();
	case LANG_UNKNOWN_UTF8:
		return new QSingleLangLexer_UTF8();
	}
	return 0;
}

QLangLexer::~QLangLexer()
{
	for( LangLexVec::iterator i = llVec.begin(); i!= llVec.end(); ++i ) { 
		if( *i )  {
			delete *i;
			*i = 0;
		}
	}
}

QSingleLangLexer* QLangLexer::addLang( size_t lang )
{
	if( lang < llVec.size() ) {
		if( !llVec[ lang ] )
			llVec[ lang ] = QSingleLangLexer::mkLexer( lang );
		return llVec[ lang ];
	}
	return 0;
}

int QLangLexer::lex( CTWPVec& cv, const TTWPVec& tv, const QuestionParm& qparm )
{
	QSingleLangLexer* lexer = getLang( qparm.lang );
	if( !lexer ) {
		if( qparm.lang) {
			std::cerr << "QLangLexer cant find lexer for lang " << qparm.lang << std::endl;
			return 666;
		} 
		else return 0;
	}
	return lexer->lex( cv, tv, qparm );
}

std::ostream& LangInfoArray::print( std::ostream& fp ) const
{
	for( size_t i =0; i< getMaxLang(); ++i )
		fp << Lang::getLangName( i-1 ) << ": " << langInfo[i] << std::endl;
	return fp;
}

std::ostream& operator<< ( std::ostream& fp, const LangInfo& li )
{
	return fp << li.counter << " times";
}

int16_t LangInfoArray::getDominantLanguage() const
{
    uint32_t maxCnt = 0;
	uint32_t totalCnt = 0;
    int16_t  bestLang = LANG_ENGLISH;
    for( size_t i = 0; i< sizeof(langInfo)/(sizeof(langInfo[0])); ++i )
	{
		const uint32_t cnt = langInfo[i].getCounter();
		totalCnt += cnt;
        if( cnt > maxCnt && i - 1 != LANG_ENGLISH)
		{
            maxCnt = cnt;
            bestLang = (i-1); /// lang+1 is always i - see incrementLangCounter
        }
    }
    const uint32_t threshold = 5;
    if (totalCnt && 100 * maxCnt / totalCnt < threshold)
	{
		AYLOG(WARNING) << "detected garbage language, falling back to English";
		bestLang = LANG_ENGLISH;
	}
	// AYLOG(DEBUG) << "detected dominant language: " << bestLang;
    return bestLang;
}
} // barzer namespace 
