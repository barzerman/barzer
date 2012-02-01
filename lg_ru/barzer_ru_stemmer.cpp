#include <barzer_ru_stemmer.h>
#include <ay_char.h>
#include <vector>
#include <stdint.h>

namespace barzer {

namespace {

inline bool is_vowel( const char* str ) 
{
    uint8_t b0 = (unsigned char)(str[0]),
        b1 = (unsigned char)(str[1]);
    return( 
        (b0 == 0xd0 && ( b1 == 0xb0 || b1 == 0xb5 || b1 == 0xb8 || b1 == 0xb9 || b1 == 0xbe)) ||
        (b0 == 0xd1 && 
            (b1 == 0x91 || b1 == 0x83 || b1 == 0x8b || b1 == 0x8c || b1 == 0x8d || b1 == 0x8e || b1 == 0x8f)) 
    );
}
inline bool is_consonant( const char* str ) 
{
    uint8_t b0 = (uint8_t)(str[0]), b1 = (uint8_t)(str[1]);
    return( 
        !(
        (b0==0xd0 && (b1==0xb0||b1==0xb5||b1==0xb8||b1==0xb9||b1==0xbe))||
        (b0==0xd1 && (b1==0x91||b1==0x83||b1==0x8b||b1==0x8c||b1==0x8d||b1==0x8e||b1==0x8f))
        )
    );
}

/// pure vowel 
inline bool is_truevowel( const char* s ) 
{
    uint8_t b0 = (uint8_t)(s[0]), b1 = (uint8_t)(s[1]);
    return( 
        (b0== 0xd0 && (b1==0xb0||b1==0xb5||b1==0xbe||b1==0xb8||b1==0x8b)) ||
        (b0== 0xd1 && (b1==0x91||b1==0x8f||b1==0x83))
    );
}

} // anonymous namespace 

///  russian morphology (stemmer) 

namespace suffix {
/// ATTENTION!! make sure suffix arrays are always terminated with 0

/// for _truevowel suffixes: one of these vowels is also chopped if its preceding the suffix, otherwise just the suffix is chopped
// 6 - ньк ( 6 or 7 characters to chop)
const char* adj_67_nk_truevowel[]   = {"нькими", "нького",0 };
// 5 == ньк (6 characters to chop)
const char* adj_56_nk_truevowel[]    = {"нький","нькое","нькой","нькие","ньким","ньких","нькая",0 };
// 4 = л (4 or 5 charcters to be chopped)
const char* verb_45_truevowel[]     = {"лось" ,"лась","лись","лися","лося","ться","тесь",0};

const char* verb_4[]        = {"айся","ейся","ойся","уйся"};
const char* noun_34_truevowel[]        = {"ами","ями",0};
const char* verb_34_truevowel[] = {"лся","тся",0 };

const char* adj_3_novowel[]     = {"ому","ого","ыми","ими","аму","аго","его",0};
const char* adj_3_vowel[]       = {"ями", 0 };
// 2 == л or т 
const char* verb_3_a[]  = { "ала" ,"али" ,"ало" ,"ать",0 }; 
const char* verb_3_e[]  = { "ела" ,"ели" ,"ело" ,"еть",0 };
const char* verb_3_i[]  = { "ила" ,"или" ,"ило" ,"ить",0 };
const char* verb_3_u[]  = { "ула" ,"ули" ,"уло" ,"уть",0 };
const char* verb_3_ja[] = { "яла" ,"яли" ,"яло" ,"ять",0 };
const char* verb_3_y[]  = { "ыла" ,"ыли" ,"ыло" ,"ыть",0 };
const char* verb_3[]        = {"ишь" ,"ись", "ешь" ,"ься",0};

const char* noun_23_truevowel[]        = {"ом","ой","ою","ах","ов","ам", "ем", "ей", "ью", "ию","ье","ья","ее", "ии", "ие","ия", "ая", "ую", 0};
const char* verb_2[]        = {"ил" ,"ят", "ит", "ал" ,"ол","ел","ыл","ул","ся","им","ет", "ут","ёт",  0};
const char* adj_2[]         = {"ий" ,"ое" , "ых", "ые","ый","ым","ой","ие","их","им","ая",0};

// noun suffixes 


}

namespace {
// this function is essentially a macro used by Russian::stem
inline bool chop_into_string( std::string& out, size_t toChop, const char* s, size_t s_len )
    { return( out.assign( s, (s_len-2*toChop) ), true ); }
} // anonymous namespace 
using namespace ay;
bool Russian::normalize( std::string& out, const char* s, size_t* len ) 
{
    using namespace suffix;
    size_t s_len = strlen( s );
    if( len ) 
        *len = s_len;
    size_t numLetters = s_len/2; // number of russian characters (one russian characters s 2 chars)
    enum { RUSSIAN_MIN_STEM_CHAR = 4, RUSSIAN_MIN_STEM_LEN=(2*RUSSIAN_MIN_STEM_CHAR) };

    if( s_len< RUSSIAN_MIN_STEM_LEN ) 
        return false;
    /// suffix stemming 
    Char2B_accessor l1( s+ s_len -2), // last character
        l2( l1.prev() ), // the one before last 
        l3( l2.prev() ), // 3 from last
        l4( l3.prev() );
    
    if( numLetters > 7 ) {
        Char2B_accessor l5( l4.prev()) , l6( l5.prev() );
        if( numLetters > 9 ) { // 67 suffix
            Char2B_accessor l7(l6.prev());
            if( l6(adj_67_nk_truevowel) ) 
                return( chop_into_string( out, (is_truevowel(l7.c_str()) ? 7:6 ), s, s_len ) );
            if( l5("ньк") && l5(adj_56_nk_truevowel) ) 
                return( chop_into_string( out, (is_truevowel(l6.c_str()) ? 6:5 ), s, s_len ) );
            if( l4(verb_45_truevowel) ) 
                return( chop_into_string( out, (is_truevowel(l5.c_str()) ? 5:4 ), s, s_len ) );
            if( l4(verb_4) ) 
                return( chop_into_string( out, 4, s, s_len ) );
        }
        if(l3(noun_34_truevowel) || l3(verb_34_truevowel)) 
            return( chop_into_string( out, (is_truevowel(l4.c_str()) ? 4:3 ), s, s_len ) );
    }
    if( 
        numLetters > 5 && (
        l3(adj_3_novowel) ||
        l3(adj_3_vowel) ||
        // 2 == л   (а е и ю я ы)
        (
            (l2("т") || l2("л")) && (
                (l3("а")&& l3(verb_3_a)) ||
                (l3("е")&& l3(verb_3_e)) ||
                (l3("и")&& l3(verb_3_i)) ||
                (l3("ю")&& l3(verb_3_u)) ||
                (l3("я")&& l3(verb_3_ja)) ||
                (l3("ы")&& l3(verb_3_y)) 
            )
        ) ||
        l3(verb_3) )
    ) {
        return( chop_into_string( out, 3, s, s_len ) );
    } else {
        if(numLetters > 4 && l2(noun_23_truevowel))
            return( chop_into_string( out, (is_truevowel(l3.c_str()) ? 3:2 ), s, s_len ) );
        else if(numLetters > 3 &&(l2(verb_2) || l2(adj_2)) )
            return( chop_into_string( out, 2, s, s_len ) );
        else if( is_vowel(l1.c_str() ) )  
            return( chop_into_string( out, 1, s, s_len ) ); // generic terminal vowel chop
    }
    return false;
} // Russian::stem 

} // namespace barzer
