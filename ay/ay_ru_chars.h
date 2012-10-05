#pragma once

/// superfast russian utf8 stuff 
namespace ay {

namespace russian {


typedef std::pair<uint8_t,uint8_t> RuChar;
inline RuChar single_char_tolower( const char* src )
{

    const uint8_t c0=static_cast<uint8_t>(src[0]);
    const uint8_t c1=static_cast<uint8_t>(src[1]);
    if( c0 == 0xd0 ) {
        switch(c1) {
        case 0x81 : return RuChar(0xd1,0x91); // Ё 81 81d0
        case 0x90 : return RuChar(0xd0,0xb0); // А 90 90d0
        case 0x91 : return RuChar(0xd0,0xb1);// Б 91 91d0
        case 0x92 : return RuChar(0xd0,0xb2); // В 92 92d0
        case 0x93 : return RuChar(0xd0,0xb3); // Г 93 93d0
        case 0x94 : return RuChar(0xd0,0xb4); // Д 94 94d0
        case 0x95 : return RuChar(0xd0,0xb5); // Е 95 95d0
        case 0x96 : return RuChar(0xd0,0xb6); // Ж 96 96d0
        case 0x97 : return RuChar(0xd0,0xb7); // З 97 97d0
        case 0x98 : return RuChar(0xd0,0xb8); // И 98 98d0
        case 0x99 : return RuChar(0xd0,0xb9); // Й 99 99d0
        case 0x9a : return RuChar(0xd0,0xba); // К 9a 9ad0
        case 0x9b : return RuChar(0xd0,0xbb); // Л 9b 9bd0
        case 0x9c : return RuChar(0xd0,0xbc); // М 9c 9cd0
        case 0x9d : return RuChar(0xd0,0xbd); // Н 9d 9dd0
        case 0x9e : return RuChar(0xd0,0xbe); // О 9e 9ed0
        case 0x9f : return RuChar(0xd0,0xbf); // П 9f 9fd0
        case 0xa0 : return RuChar(0xd1,0x80); // Р a0 a0d0
        case 0xa1 : return RuChar(0xd1,0x81); // С a1 a1d0
        case 0xa2 : return RuChar(0xd1,0x82); // Т a2 a2d0
        case 0xa3 : return RuChar(0xd1,0x83); // У a3 a3d0
        case 0xa4 : return RuChar(0xd1,0x84); // Ф a4 a4d0
        case 0xa5 : return RuChar(0xd1,0x85); // Х a5 a5d0
        case 0xa6 : return RuChar(0xd1,0x86); // Ц a6 a6d0
        case 0xa7 : return RuChar(0xd1,0x87); // Ч a7 a7d0
        case 0xa8 : return RuChar(0xd1,0x88); // Ш a8 a8d0
        case 0xa9 : return RuChar(0xd1,0x89); // Щ a9 a9d0
        case 0xaa : return RuChar(0xd1,0x8a); // Ъ aa aad0
        case 0xab : return RuChar(0xd1,0x8b); // Ы ab abd0
        case 0xac : return RuChar(0xd1,0x8c); // Ь ac acd0
        case 0xad : return RuChar(0xd1,0x8d); // Э ad add0
        case 0xae : return RuChar(0xd1,0x8e); // Ю ae aed0
        case 0xaf : return RuChar(0xd1,0x8f); // Я af afd0
        default: return RuChar(c0,c1);
        }
    } else 
    if( c0 == 0xd1 ) { // there are no uppercase russian letters in 0xd1
        return RuChar(c0,c1);
    } else
        return RuChar(c0,c1);
}

} // namespace ru

} // namespace ay
