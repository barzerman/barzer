#pragma once

#include <vector>
#include <string>

/// russian language functions 
namespace ay
{
namespace tl
{
	typedef std::pair<std::string, std::string> TLException_t;
	typedef std::vector<TLException_t> TLExceptionList_t;
	void ru2en(const char *russian, size_t len, std::string& english, const TLExceptionList_t& = TLExceptionList_t());
	void en2ru(const char *english, size_t len, std::string& russian, const TLExceptionList_t& = TLExceptionList_t());
    // s msut have at least 2 bytes s is ASSUMED to be 2byte russian 
    inline bool is_russian_vowel( uint8_t b0, uint8_t b1 )
    {
        return( 
            (b0== 0xd0 && (b1==0xb0||b1==0xb5||b1==0xbe||b1==0xb8||b1==0x8b)) ||
            (b0== 0xd1 && (b1==0x91||b1==0x8f||b1==0x83||b1==0x8d||b1==0x8e))
        );  
    }
    inline bool is_russian_vowel( const char *s ) { return is_russian_vowel( (uint8_t)(s[0]), (uint8_t)(s[1]) ); }

    inline bool is_russian_znak( uint8_t b0, uint8_t b1 ) { return( b0 == 0xd1 && (b1 == 0x8c || b1 == 0x8a)); }
    inline bool is_russian_znak( const char *s ) { return is_russian_znak((uint8_t)(s[0]), (uint8_t)(s[1]) ); }

    inline bool is_not_russian_consonant( uint8_t b0, uint8_t b1 ) { 
        return ( is_russian_vowel(b0,b1) ||  is_russian_znak(b0,b1) );
    }

    inline bool is_not_russian_consonant( const char *s ) { return (is_russian_vowel(s) ||  is_russian_znak(s)); }
}
}
