#include <barzer_language.h>

#include <lg_ru/barzer_ru_lex.h>
#include <lg_en/barzer_en_lex.h>

namespace barzer {

namespace {
const char * rus_letter[] =  {
    "а","б","в","г","д","е","ж","з","и","й","к","л","м","н",
    "о","п","р","с","т","у","ф","х","ц","ч","ш","щ","ь","ы","ъ","э","ю","я"
};

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
            } else if( b1 == 0x81 ) {
                ss[0] = 0xd1;
                ss[1] = 0x91;
            }
        }

        return hasUpperCase;
    } else 
        return false;
}

int Lang::getLang( const char* str, size_t s_len )
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

const char* Lang::getLangName( int xx ) 
{
    if( xx == LANG_ENGLISH ) return "ENGLISH"; 
    else if( xx == LANG_RUSSIAN ) return "RUSSIAN";
    else
        return "UNKNOWN";
}
/// the factory method
QSingleLangLexer* 	QSingleLangLexer::mkLexer( int lg )
{
	switch( lg ) {
	case LANG_ENGLISH:
		return new QSingleLangLexer_EN();
	case LANG_RUSSIAN:
		return new QSingleLangLexer_RU();
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
    return bestLang;
}
} // barzer namespace 
