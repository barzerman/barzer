
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#include <barzer_ru_stemmer.h>
#include <ay_char.h>
#include <ay_util.h>
#include <ay_translit_ru.h>
#include <vector>
#include <stdint.h>

namespace barzer {

namespace {

inline bool is_vowel( const char* str ) 
{
    const uint8_t b0 = (unsigned char)(str[0]), b1 = (unsigned char)(str[1]);

    return (ay::tl::is_russian_vowel(b0,b1) || ay::tl::is_russian_iota(b0,b1) || ay::tl::is_russian_znak(b0,b1) );
}
inline bool is_consonant( const char* str ) 
{
    uint8_t b0 = (uint8_t)(str[0]), b1 = (uint8_t)(str[1]);
    return ( (!ay::tl::is_russian_vowel(b0,b1)) && (!ay::tl::is_russian_znak(b0,b1)) && (!ay::tl::is_russian_iota(b0,b1)) );
}

/// pure vowel 
inline bool is_truevowel( const char* s ) 
{
    return ay::tl::is_russian_vowel((uint8_t)(s[0]), (uint8_t)(s[1]) );
}

} // anonymous namespace 

///  russian morphology (stemmer) 

namespace suffix {
/// ATTENTION!! make sure suffix arrays are always terminated with 0

/// for _truevowel suffixes: one of these vowels is also chopped if its preceding the suffix, otherwise just the suffix is chopped
// 6 - ньк ( 6 or 7 characters to chop)
const char* verb_6[] = {
    "вшаяся", "вшаяся", "вшуюся", "вшийся", "вшимся", "вшемся","вшиеся","вшихся",
    "ущаяся", "ущаяся", "ущуюся", "ущийся", "ущимся", "ущемся","ущиеся","ущихся",
    "ющаяся", "ющаяся", "ющуюся", "ющийся", "ющимся", "ющемся","ющиеся","ющихся",
    "ящаяся", "ящаяся", "ящуюся", "ящийся", "ящимся", "ящемся","ящиеся","ящихся", 
    0
};
const char* adj_67_nk_truevowel[]   = {
    "нькими", "нького",0 };
// 5 == ньк (6 characters to chop)
const char* adj_56_nk_truevowel[]    = {"нький","нькое","нькой","нькие","ньким","ньких","нькая",0 };
// 4 = л (4 or 5 charcters to be chopped)
const char* verb_56_sch[]     = {"щему","щего","щими",0};
const char* verb_45_sch[]     = { "щая","щую","щей","щие","щих","щее","щий","щим", "щем", 0 };
const char* verb_45_truevowel[]     = {
"лось" ,"лась","лись","лися","лося","ться","тесь",0};

const char* verb_5[]        = {
    "емого", "имого",
    "емому", "емыми",
    "имому", "имыми",
    0
};
const char* verb_4[]        = {
    "ящий", "ящая", "ящее", "ящие", 
    "ющий", "ющая", "ющее", "ющие", 
    "утся", "ются", 
    "емый", "емые", "емое", "емая", "емся", 
    "ется", "ится", "айся","ейся","ойся","уйся", 0

};
const char* noun_3_truevowel[]        = {"ами","ями",0};
const char* verb_3_truevowel[] = {"лся","тся",0 };

const char* adj_3_novowel[]     = {"ает","ают","ому","ого","ыми","ими","аму","аго","его",0};
const char* adj_3_vowel[]       = {"ями", 0 };
// 2 == л or т 
const char* verb_3_a[]  = { "ает","ают", "ала" ,"али" ,"ало" ,"ать",0 }; 
const char* verb_3_e[]  = { "еет", "еют", "ела" ,"ели" ,"ело" ,"еть",0 };
const char* verb_3_i[]  = { "ите", "ись", "ила" ,"или" ,"ило" ,"ить",0 };
const char* verb_3_u[]  = { "ула" ,"ули" ,"уло" ,"уть",0 };
const char* verb_3_ja[] = { "яет", "яют", "яла" ,"яли" ,"яло" ,"ять",0 };
const char* verb_3_y[]  = { "ыла" ,"ыли" ,"ыло" ,"ыть",0 };
const char* verb_3[]        = {"ишь" ,"ись", "ешь" ,"ься",0};

const char* noun_23_truevowel[]        = {"ом","ой","ою","ах","ов","ам", "ем", "ей", "ью", "ию","ье","ья","ее", "ии", "ие","ия", "ая", "ую", 0};
const char* noun_21_truevowel[]        = { /*"ию","ии","ия",*/ 0};
const char* noun_32_truevowel[]        = {"ией","иям","иях", 0};
const char* noun_43_truevowel[]        = {"иями", 0};

const char* verb_2[]        = {"ил" ,"ят", "ял", "ит", "ал" ,"ол","ел","ыл","ул","ся","им","ет", "ут","ёт",  0};
const char* adj_2[]         = {"ий" ,"ое" , "ых", "ые","ый","ым","ой","ую", "юю","ие","их","им","яя", "ая",0};

inline const char* get_exception_stem( const char* s )
{
	if (!strcmp(s, "недели"))
		return "недель";
	else if (!strcmp(s, "память"))
		return "памят";
	else
		return 0;
}

}

namespace {
// this function is essentially a macro used by Russian::stem
inline bool chop_into_string( std::string& out, size_t toChop, const char* s, size_t s_len )
    { return( out.assign( s, (s_len-2*toChop) ), true ); }
inline bool suffix_3char_compatible( const char* suffix, const char* w, size_t w_sz )
{
    if( !suffix || w_sz < 5*2 ) 
        return false;

    if( !strcmp(suffix,"ели") ) {
        ay::Char2B_accessor l4( w, w_sz-2*4), l5( w, w_sz-5*2 );
        if( l5.isChar( "у" ) || l5.isChar( "о" ) )
            return true;
        else 
            return false;
    }
    return true;
}

} // anonymous namespace 
using namespace ay;
bool Russian::normalize( std::string& out, const char* s, size_t* len, const Russian_StemmerSettings* settings ) 
{
    using namespace suffix;
    size_t s_len = strlen( s );
    if( len ) 
        *len = s_len;
    
    const char* tmp = get_exception_stem(s);
    if( tmp ) {
        out.assign(tmp);
        return true;
    }
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
        if( numLetters >= 8 ) { // 67 suffix
            Char2B_accessor l7(l6.prev());
            if( numLetters > 9 ) {
                if( l6(adj_67_nk_truevowel) ) 
                    return( chop_into_string( out, (is_truevowel(l7.c_str()) ? 7:6 ), s, s_len ) );
                else if( l6(verb_6) )
                    return( chop_into_string( out, (is_truevowel(l7.c_str()) ? 7:6 ), s, s_len ) );
            }
            bool l5_isVowel = is_truevowel(l5.c_str()),
                 l4_isVowel = is_truevowel(l4.c_str());
            if( numLetters >= 9 && l5(verb_5) )
                return( chop_into_string( out, (is_truevowel(l6.c_str()) ? 6:5 ), s, s_len ) );

            if( l5_isVowel && l4(verb_56_sch) )
                return( chop_into_string( out, (is_truevowel(l6.c_str()) ? 6:5 ), s, s_len ) );
            if( l5("ньк") && l5(adj_56_nk_truevowel) ) 
                return( chop_into_string( out, (is_truevowel(l6.c_str()) ? 6:5 ), s, s_len ) );
            if( l4_isVowel && l3(verb_45_sch) ) 
                return( chop_into_string(out, (l5_isVowel ? 5:4 ), s, s_len) );
            if( l4(verb_45_truevowel) ) 
                return( chop_into_string(out, (l5_isVowel ? 5:4 ), s, s_len) );
            if( l4(verb_4) ) 
                return( chop_into_string( out, (l5_isVowel ? 5:4 ), s, s_len ) );
        }
    }
    /// trying 3 letter noun suffixes
    if( numLetters > 5 ) {
        if( l4(noun_43_truevowel) )
            return( chop_into_string( out, 3, s, s_len ) );
        if( numLetters>5 ) {
            if(l3(noun_3_truevowel))
                return( chop_into_string( out, (is_truevowel(l4.c_str())&& numLetters>6 ? 4:3 ), s, s_len ) );
        }
    }
    if( numLetters > 4 ) {
        if( l3(noun_32_truevowel) )
            return( chop_into_string( out, 2, s, s_len ) );
    }
    if( numLetters > 3 ) {
        if( l2(noun_21_truevowel) )
            return( chop_into_string( out, 1, s, s_len ) );
    }
    
    /// trying verb 3 letter suffixes
    if( numLetters > 5 && 
        (
        suffix_3char_compatible( l3(verb_3_a), s, s_len)  ||
        suffix_3char_compatible( l3(verb_3_e), s, s_len)  ||
        suffix_3char_compatible( l3(verb_3_i), s, s_len)  ||
        suffix_3char_compatible( l3(verb_3_u), s, s_len)  ||
        suffix_3char_compatible( l3(verb_3_ja), s, s_len) ||
        suffix_3char_compatible( l3(verb_3_y), s, s_len)  ||
        suffix_3char_compatible( l3(verb_3_y), s, s_len)  ||
        suffix_3char_compatible( l3(verb_3), s, s_len) ||
        suffix_3char_compatible( l3(verb_3_truevowel), s, s_len)
        )
    ) {
        return( chop_into_string( out, 3, s, s_len ) );
    }
    if( numLetters > 4 ) {
        if( settings && l2(verb_2) )
            return( chop_into_string( out, 2, s, s_len ) );
    }


    if( 
        numLetters > 5 && (
        l3(noun_3_truevowel) ||
        l3(adj_3_novowel) ||
        l3(adj_3_vowel) )
    ) {
        return( chop_into_string( out, 3, s, s_len ) );
    } else {
        if(numLetters > 4 && l2(noun_23_truevowel))
            return( chop_into_string( out, (is_truevowel(l3.c_str()) ? 3:2 ), s, s_len ) );
        else if(numLetters > 3 &&(l2(adj_2)) )
            return( chop_into_string( out, 2, s, s_len ) );
        else if( is_vowel(l1.c_str() ) )  
            return( chop_into_string( out, 1, s, s_len ) ); // generic terminal vowel chop
    }
    return false;
} // Russian::stem 

} // namespace barzer
