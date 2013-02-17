
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <stdint.h>

namespace barzer {
    
struct Lang {
    enum {
        LANG_UNKNOWN=-1,
        LANG_ENGLISH=0,
        LANG_RUSSIAN
    };
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

} // barzer namespace ends
using namespace barzer;
int main( int argc, char* argv[] ) 
{
    char buf[ 256 ];
    const char* rus="абвгдеёжзийклмнопрстуфхцчшщьыъэюяАБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЬЫЪЭЮЯ";
        char c[3]= {0};
    for( const char* s= rus; *s; s+=2 ) {
        c[0] = *s;
        c[1] = s[1];
        unsigned c1 = c[0], c2 = c[2] ;
        uint16_t x = *(const uint16_t*)( c );
        printf( "%x,%x // %s %x %x\n", (unsigned char)(c[0]), (unsigned char)c[1], c, (unsigned char)c[1],x );
    }
    if( argc > 1 ) {
    while( fgets(buf,sizeof(buf),stdin) ) {
        buf[ strlen(buf)-1 ] =0;
        const char* langName = Lang::getLangName( Lang::getLang( buf, strlen(buf) ) );
        std::cerr << langName << std::endl;
    }
    }
}
