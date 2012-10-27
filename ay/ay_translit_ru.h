#pragma once

#include <vector>
#include <string>

/// russian language functions 
namespace ay
{
namespace tl
{

inline const char* ru_to_ascii( const char* s ) 
{
    if( static_cast<uint8_t>(s[0]) == 0xd0 ) {
        switch(static_cast<uint8_t>(s[1]) ) {
        case  0x81 : return "yo"; // Ё 81 81d0
        case  0x90 : return "a"; // А 90 90d0
        case  0x91 : return "b"; // Б 91 91d0
        case  0x92 : return "v"; // В 92 92d0
        case  0x93 : return "g"; // Г 93 93d0
        case  0x94 : return "d"; // Д 94 94d0
        case  0x95 : return "ye"; // Е 95 95d0
        case  0x96 : return "zh"; // Ж 96 96d0
        case  0x97 : return "z"; // З 97 97d0
        case  0x98 : return "i"; // И 98 98d0
        case  0x99 : return "j"; // Й 99 99d0
        case  0x9a : return "k"; // К 9a 9ad0
        case  0x9b : return "l"; // Л 9b 9bd0
        case  0x9c : return "m"; // М 9c 9cd0
        case  0x9d : return "n"; // Н 9d 9dd0
        case  0x9e : return "o"; // О 9e 9ed0
        case  0x9f : return "p"; // П 9f 9fd0
        case  0xa0 : return "r"; // Р a0 a0d0
        case  0xa1 : return "s"; // С a1 a1d0
        case  0xa2 : return "t"; // Т a2 a2d0
        case  0xa3 : return "u"; // У a3 a3d0
        case  0xa4 : return "f"; // Ф a4 a4d0
        case  0xa5 : return "h"; // Х a5 a5d0
        case  0xa6 : return "tz"; // Ц a6 a6d0
        case  0xa7 : return "ch"; // Ч a7 a7d0
        case  0xa8 : return "sh"; // Ш a8 a8d0
        case  0xa9 : return "sch"; // Щ a9 a9d0
        case  0xaa : return ""; // Ъ aa aad0
        case  0xab : return "y"; // Ы ab abd0
        case  0xac : return ""; // Ь ac acd0
        case  0xad : return "e"; // Э ad add0
        case  0xae : return "yu"; // Ю ae aed0
        case  0xaf : return "ya"; // Я af afd0
        case  0xb0 : return "a"; // а b0 b0d0
        case  0xb1 : return "b"; // б b1 b1d0
        case  0xb2 : return "v"; // в b2 b2d0
        case  0xb3 : return "g"; // г b3 b3d0
        case  0xb4 : return "d"; // д b4 b4d0
        case  0xb5 : return "ye"; // е b5 b5d0
        case  0xb6 : return "zh"; // ж b6 b6d0
        case  0xb7 : return "z"; // з b7 b7d0
        case  0xb8 : return "i"; // и b8 b8d0
        case  0xb9 : return "j"; // й b9 b9d0
        case  0xba : return "k"; // к ba bad0
        case  0xbb : return "l"; // л bb bbd0
        case  0xbc : return "m"; // м bc bcd0
        case  0xbd : return "n"; // н bd bdd0
        case  0xbe : return "o"; // о be bed0
        case  0xbf : return "p"; // п bf bfd0
        }
    } else if( static_cast<uint8_t>(s[0]) == 0xd1 ) {
        switch(static_cast<uint8_t>(s[1]) ) {
        case  0x80 : return "r"; // р 80 80d1
        case  0x81 : return "s"; // с 81 81d1
        case  0x82 : return "t"; // т 82 82d1
        case  0x83 : return "u"; // у 83 83d1
        case  0x84 : return "f"; // ф 84 84d1
        case  0x85 : return "h"; // х 85 85d1
        case  0x86 : return "tz"; // ц 86 86d1
        case  0x87 : return "ch"; // ч 87 87d1
        case  0x88 : return "sh"; // ш 88 88d1
        case  0x89 : return "sch"; // щ 89 89d1
        case  0x8a : return ""; // ъ 8a 8ad1
        case  0x8b : return "y"; // ы 8b 8bd1
        case  0x8c : return ""; // ь 8c 8cd1
        case  0x8d : return "e"; // э 8d 8dd1
        case  0x8e : return "yu"; // ю 8e 8ed1
        case  0x8f : return "ya"; // я 8f 8fd1
        case  0x91 : return "yo"; // ё 91 91d1
        }
    }
    return s;
}
	typedef std::pair<std::string, std::string> TLException_t;
	typedef std::vector<TLException_t> TLExceptionList_t;
	void ru2en(const char *russian, size_t len, std::string& english, const TLExceptionList_t& = TLExceptionList_t());
	void en2ru(const char *english, size_t len, std::string& russian, const TLExceptionList_t& = TLExceptionList_t());
    // s msut have at least 2 bytes s is ASSUMED to be 2byte russian 
    inline bool is_russian_vowel( uint8_t b0, uint8_t b1 )
    {
        return( 
            (b0== 0xd0 && (b1==0xb0||b1==0xb5||b1==0xbe||b1==0xb8)) ||
            (b0== 0xd1 && (b1==0x91||b1==0x8f||b1==0x83||b1==0x8d||b1==0x8e||b1==0x8b))
        );  
    }
    /// buf must be a valid all utf8 russian string (utf8 russian has 2 byte charafcters)
    /// dedupes in place, returns true if anything was removed
    bool dedupeRussianConsonants( std::string& dest, const char* buf, size_t buf_len );

    /// input: assumes russian characters (utf8 2 byte)
    /// strategy: all consecutive vowels are replaced with a
    ///           all consonants are mapped back to english counterparties (reverse translit)
    ///           
    bool normalize_eng_onevowel( std::string& dest, const char* buf, size_t buf_len );

    inline bool is_russian_vowel( const char *s ) { return is_russian_vowel( (uint8_t)(s[0]), (uint8_t)(s[1]) ); }

    inline bool is_russian_znak( uint8_t b0, uint8_t b1 ) { return( b0 == 0xd1 && (b1 == 0x8c || b1 == 0x8a)); }
    inline bool is_russian_znak( const char *s ) { return is_russian_znak((uint8_t)(s[0]), (uint8_t)(s[1]) ); }

    inline bool is_russian_iota( uint8_t b0, uint8_t b1 ) { return (b0==0xd0 && b1==0xd9); }
    inline bool is_russian_iota( const char *s ) { return is_russian_iota((uint8_t)(s[0]), (uint8_t)(s[1]) ); }

    inline bool is_not_russian_consonant( uint8_t b0, uint8_t b1 ) { 
        return ( is_russian_vowel(b0,b1) ||  is_russian_znak(b0,b1) );
    }
    inline bool is_russian_consonant( uint8_t b0, uint8_t b1 ) { return !is_not_russian_consonant(b0,b1); }

    inline bool is_not_russian_consonant( const char *s ) { return (is_russian_vowel(s) ||  is_russian_znak(s)); }
}
}
