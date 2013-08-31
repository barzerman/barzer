
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#include <barzer_language.h>

#include <numeric>
#include <lg_ru/barzer_ru_lex.h>
#include <lg_en/barzer_en_lex.h>
#include <ay/ay_utf8.h>
#include <ay/ay_levenshtein.h>
#include "barzer_universe.h"

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

bool Lang::hasTwoByteDiacritics( const char* str, size_t str_len, int lang )
{
    if( lang == LANG_RUSSIAN ) {
        for( const char* s = str, *s_end = str+str_len; s< s_end; s+=2 ) {
            if( (uint8_t)(s[0]) == 0xd0 ) {
                if( (uint8_t)(s[1]) == 0x81 ) {// Ё
                    return true;
                }
            } else if((uint8_t)(s[0]) == 0xd1 ) { 
                if( (uint8_t)(s[1]) == 0x91 ) { // ё
                    return true;
                }
            }
        }
    } 
    return false;
}
bool Lang::twoByteStripDiacritics( std::string& dest, const char* str, size_t str_len, int lang )
{
    dest.clear();
    dest.reserve( str_len );
    bool hasDiacritics = false;
    if( lang == LANG_RUSSIAN ) {
        for( const char* s = str, *s_end = str+str_len; s< s_end; s+=2 ) {
            if( (uint8_t)(s[0]) == 0xd0 ) {
                if( (uint8_t)(s[1]) == 0x81 ) {// Ё
                    dest.push_back( (char)(0xd0) );
                    dest.push_back( (char)(0x95) );
                    if( !hasDiacritics )
                        hasDiacritics= true;
                    continue;
                }
            } else if((uint8_t)(s[0]) == 0xd1 ) { 
                if( (uint8_t)(s[1]) == 0x91 ) { // ё
                    dest.push_back( (char)(0xd0) );
                    dest.push_back( (char)(0xb5) );
                    if( !hasDiacritics )
                        hasDiacritics= true;
                    continue;
                } else if( (uint8_t)(s[1]) == 0x8a ) { // ъ
                    dest.push_back( (char)(0xd1) );
                    dest.push_back( (char)(0x8c) );
                    if( !hasDiacritics )
                        hasDiacritics= true;
                }
            }

            dest.push_back( s[0] );
            dest.push_back( s[1] );
        }
    }
    return hasDiacritics;
}
bool Lang::convertTwoByteToLower( char* s, size_t s_len, int lang )
{
    bool hasUpperCase = false;
    if( lang == LANG_RUSSIAN ) {
        // all russian uppercase letters are 0xd0
        char* s_end = s+s_len;
        for( char* ss=s; ss< s_end; ss+=2 ) {
            uint8_t b1= (uint8_t)(ss[1]);
            if( (uint8_t)(ss[0]) == 0xd0 ) {
                if( b1 == 0x81 ) {// Ё
                    ss[0] = 0xd1;ss[1] = 0x91; continue;
                } else
                if( b1 == 0xac ) {  // Ь
                    ss[0] = 0xd1; ss[1] = 0x8c; continue;
                } else
                if( b1 == 0xac ) {  // Ъ
                    ss[0] = 0xd1; ss[1] = 0x8a; continue;
                } else
                if( b1 == 0x99 ) {  // Й
                    ss[0] = 0xd0; ss[1] = 0xb9; continue;
                }
            } 
            
            if( !((uint8_t)(ss[0]) == 0xd1 && b1==0x91) && (b1 >=0x90 && b1 <=0xaf ) ) {
                const char* lc = rus_letter[ (b1-0x90) ];
                hasUpperCase = true;
                ss[0] = lc[0];
                ss[1] = lc[1];
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
size_t Lang::getLevenshteinDistance( ay::LevenshteinEditDistance& lev, const char* s1, size_t s1_len, const char* s2, size_t s2_len )
{
    int lang1 = getLangNoUniverse( s1, s1_len );
    int lang2 = getLangNoUniverse( s2, s2_len );
    
    if( lang1 != lang2 ) {
	    ay::StrUTF8 u1(s1, s1_len);
	    ay::StrUTF8 u2(s2, s2_len);

        return  lev.utf8( u1, u2 );
    } else if( isTwoByteLang(lang1) ) {
        return lev.twoByte(s1, s1_len / 2, s2, s2_len / 2);
    } else 
        return lev.ascii_no_case(s1, s2);
}

void Lang::lowLevelNormalization( char* d, size_t d_len, const char* s, size_t s_sz )
{
    const char* d_end = d+d_len;
    for( const char* ss = s, *ss_end = s+s_sz; ss < ss_end && d< d_end; ++ss ) {
        if( static_cast<uint8_t>(ss[0])== 0xe2 ) {
            if( 0x80 == static_cast<uint8_t>(ss[1]) && 0x94 == static_cast<uint8_t>(ss[2])) { // wikipedia hyphen
                *d='-';
                ++d;
                ss+=2;
                continue;
            } 
        } 

        *d++=*ss;
    }
    if( d< d_end )
        *d=0;
}

int Lang::stringNoCaseCmp( const std::string& l, const std::string& r )
{
    std::vector<char> buf;
    std::string ll, lr;

    stringToLower( buf, ll, l );
    stringToLower( buf, lr, r );
    return strcmp( ll.c_str(), lr.c_str() );
}

bool Lang::stringToLower( std::string& dest, const std::string& src )
{
    std::vector<char> buf;
    return stringToLower(buf,dest,src);
}
bool Lang::stringToLower( std::vector<char>& buf, std::string& dest, const std::string& src )
{
    size_t buf_sz = src.length();
    buf.resize( buf_sz + 1) ;
    memcpy( &(buf[0]), src.c_str(), buf_sz );
    buf[ buf_sz ] = 0;
    int lang = getLangNoUniverse( &(buf[0]), buf_sz );

    if( bool hasLowerCase = stringToLower( &(buf[0]), buf_sz, lang ) ) {
        dest.assign( &(buf[0]) );
        return true;
    } else {
        dest = src;
        return false;
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


int Lang::getLangNoUniverse( const char* str, size_t s_len )
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
size_t Lang::getNumGlyphs(int lang, const char* str, size_t s_len )
{
    return( lang == LANG_ENGLISH ? s_len : ( Lang::isTwoByteLang(lang) ? s_len/2 : ay::StrUTF8::glyphCount(str,str+s_len)));
}
int Lang::getLangAndLengthNoUniverse( size_t& numGlyphs, const char* str, size_t s_len )
{
    int lang = getLangNoUniverse(str,s_len);
    if(lang == LANG_ENGLISH) {
        return ( numGlyphs=s_len, lang );
    } else if( Lang::isTwoByteLang(lang)) {
        return ( numGlyphs=s_len/2, lang );
    } else { // utf8
        return ( numGlyphs= ay::StrUTF8::glyphCount(str,str+s_len), lang );
    }
}

namespace
{
	std::pair<int, double> accMaxPair(const std::pair<int, double>& left, const std::pair<int, double>& right)
	{
		return left.second > right.second ? left : right;
	}
}

int Lang::getLang( const StoredUniverse& universe, const char *str, size_t s_len )
{
	const char *s_end = str + s_len;

    
    int curLang = LANG_UNKNOWN;
    if( !(s_len&1) ) { // for even s_len determine whether this is Russian
	    for (const char *s = str; s < s_end; s+=2) {
            unsigned char 
                c0 = static_cast<unsigned char>(*s), 
                c1 = static_cast<unsigned char>(s[1]);
            if( curLang == LANG_UNKNOWN ) {
		        if (utf8_2byte_isRussian(c0,c1))
			        curLang= LANG_RUSSIAN;
                else
                    break;
            } else if( curLang != LANG_RUSSIAN || !utf8_2byte_isRussian(c0,c1)) 
                if( curLang == LANG_RUSSIAN )
                    curLang = LANG_UNKNOWN;
                break;
        }
        if( curLang == LANG_RUSSIAN )
            return LANG_RUSSIAN;
    }
    if( universe.getBarzHints().getUtf8Languages().size() ) {
	    GlobalPools& gp = universe.gp;
	    ay::ASCIITopicModelMgr *ascii = gp.getASCIILangMgr();
	    ay::UTF8TopicModelMgr *utf8 = gp.getUTF8LangMgr();
	    if (ascii->getNumTopics() == 1 && utf8->getNumTopics() == 1)
	    {
		    std::vector<int> asciiLang, utf8Lang;
		    ascii->getAvailableTopics(asciiLang, universe.getBarzHints().getUtf8Languages() );
		    utf8->getAvailableTopics(utf8Lang, universe.getBarzHints().getUtf8Languages() );
    
            if( !utf8Lang.size() )
                return LANG_ENGLISH;
            else if( !asciiLang.size() )
                return ( utf8Lang.size() ? fromAyLang(utf8Lang[0]) : LANG_ENGLISH );
            else {
		        double utf8Score = 0, asciiScore = 0;

		        ay::getScores (utf8->getModel(utf8Lang[0]), ascii->getModel(asciiLang[0]), str, s_len, utf8Score, asciiScore);
		        return fromAyLang(utf8Score > asciiScore ? utf8Lang[0] : asciiLang[0]);
            }
	    }
    
	    std::vector<std::pair<int, double> > probs;
	    ay::evalAllLangs(utf8, ascii, str, probs, true);
	    return fromAyLang(std::accumulate(probs.begin(), probs.end(), std::make_pair(0, 0.0), accMaxPair).first);
    } 
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
