#include <lg_ru/barzer_ru_lex.h>
#include <barzer_bzspell.h>
namespace barzer {
int QSingleLangLexer_RU::lex( CTWPVec& , const TTWPVec&, const QuestionParm& )
{
	return 0;
}
namespace {

inline bool is_vowel( const char* str ) 
{
uint8_t b0 = (unsigned char)(str[0]),
        b1 = (unsigned char)(str[1]);
return( 
    (b0 == 0xd0 && ( b1 == 0xb0 || b1 == 0xb5 || b1 == 0xb8 || b1 == 0xb9 || b1 == 0xbe)) ||
    (b0 == 0xd1 && (b1 == 0x91 || b1 == 0x83 || b1 == 0x8b || b1 == 0x8d || b1 == 0x8e || b1 == 0x8f)) 
);

}

}
/// russian morphology (stemmer) 
// here we already assume that the string has an even number of chars (2 per each russian letter)
uint32_t Russian_Stemmer::getStemCorrection( std::string& out, const BZSpell& bzspell, const char* s, size_t s_len )
{
    if( s_len < 1 || s_len>= BZSpell::MAX_WORD_LEN ) 
        return 0xffffffff;

    char buf[ BZSpell::MAX_WORD_LEN ];
    memcpy( buf, s, s_len );
    buf[s_len-1]= 0;
    char* beg = buf , *end = beg+s_len;
    size_t buf_len = s_len;
    for( ;buf_len > 4; buf_len=end-beg) { // word is longer than 3 letters
        
        for( ; end>= beg ; end-=2 ) {
            if( is_vowel(x) ) {
                *end = 0;
                uint32_t id= 0xffffffff;
                if( bzspell.isUsersWord( id, str ) ) 
                    return ( out.assign(beg) , id );
            } else 
                break;
        }
    }
    return 0xffffffff;
}

}
