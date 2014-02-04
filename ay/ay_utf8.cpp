#include "ay_utf8.h"
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <algorithm>
#include "tables/u2l.cpp"
#include "tables/l2u.cpp"
#include "tables/decompositions.cpp"

namespace ay
{
	namespace
	{
		typedef uint32_t p_t[2];

		bool fstComp(uint32_t *left, uint32_t right)
		{
			return left[0] < right;
		}
	}

	bool CharUTF8::toLower()
	{
		const uint32_t utf32 = toUTF32();
		p_t *end = tableU2L + sizeof(tableU2L) / sizeof(tableU2L[0]);
		p_t *pair = std::lower_bound(tableU2L, end, utf32, fstComp);
		if (pair == end || (*pair)[0] != utf32)
			return false;
		
		setUTF32((*pair)[1]);
		return true;
	}

	bool CharUTF8::toUpper()
	{
		const uint32_t utf32 = toUTF32();
		p_t *end = tableL2U + sizeof(tableL2U) / sizeof(tableL2U[0]);
		p_t *pair = std::lower_bound(tableL2U, end, utf32, fstComp);
		if (pair == end || (*pair)[0] != utf32)
			return false;
		
		setUTF32((*pair)[1]);
		return true;
	}

	bool CharUTF8::isLower() const
	{
		const uint32_t utf32 = toUTF32();
		p_t *end = tableL2U + sizeof(tableL2U) / sizeof(tableL2U[0]);
		p_t *pair = std::lower_bound(tableL2U, end, utf32, fstComp);
		return pair != end && (*pair)[0] == utf32;
	}

	bool CharUTF8::isUpper() const
	{
		const uint32_t utf32 = toUTF32();
		p_t *end = tableU2L + sizeof(tableU2L) / sizeof(tableU2L[0]);
		p_t *pair = std::lower_bound(tableU2L, end, utf32, fstComp);
		return pair != end && (*pair)[0] == utf32;
	}

	// This one will most likely fail with some exotic stuff like Hangul
	// languages, but who cares about Hangul after all?
	//
	// Also, here we assume we're in the UCS-2 plane, and that should be fixed
	// as soon as we introduce CharUTF8::fromUTF16(), since the data is in
	// UTF-16.
	StrUTF8& CharUTF8::decompose(StrUTF8& result) const
	{
		uint32_t utf32 = toUTF32();

		const uint16_t index = GET_DECOMPOSITION_INDEX(utf32);
		if (index == 0xFFFF)
			return result;

		const uint16_t *decomposition = uc_decomposition_map + index;
		const size_t length = (*decomposition) >> 8;

		for (size_t i = 0; i < length; ++i)
			result.append(fromUTF32(decomposition [i + 1]));
		return result;
	}

	bool StrUTF8::toLower()
	{
		bool hasLower = false;
		for (size_t i = 0, len = size(); i < len; ++i)
		{
			CharUTF8 c = getGlyph(i);
			if (!c.toLower())
				continue;

			setGlyph(i, c);
			hasLower = true;
		}
		return hasLower;
	}

	bool StrUTF8::toUpper()
	{
		bool hasUpper = false;
		for (size_t i = 0, len = size(); i < len; ++i)
		{
			CharUTF8 c = getGlyph(i);
			if (!c.toUpper())
				continue;

			setGlyph(i, c);
			hasUpper = true;
		}
		return hasUpper;
	}

	bool StrUTF8::normalize()
	{
		size_t sz = size();
		bool noNeed = true;
		for (size_t i = 0; i < sz; ++i)
			if (getGlyph(i).toUTF32() > 0x80)
			{
				noNeed = false;
				break;
			}

		if (noNeed)
			return false;

		bool changed = false;
		for (size_t i = 0; i < sz; ++i)
		{
			StrUTF8 dec;

			if (getGlyph(i).decompose(dec).empty())
				continue;

			dec.removeZero();

			const size_t thisPos = m_positions[i];
			std::vector<char>::iterator bufPos = m_buf.begin() + thisPos;
			bufPos = m_buf.erase(bufPos + getGlyphSize(i));
			std::vector<size_t>::iterator posPos = m_positions.erase(m_positions.begin() + i);

			size_t decBC = dec.bytesCount();
			for (std::vector<size_t>::iterator posi = posPos, pose = m_positions.end(); posi != pose; ++posi)
				*posi += decBC;

			m_buf.insert(bufPos, dec.c_str(), dec.c_str() + dec.bytesCount());

			for (std::vector<size_t>::iterator psi = dec.m_positions.begin(), pse = dec.m_positions.end(); psi != pse; ++psi)
				*psi += thisPos;
			m_positions.insert(posPos, dec.m_positions.begin(), dec.m_positions.end());

			// Standard says decomposition should be done recursively, but I doubt
			// it's sane, so let's leave this one for now.
			//i += dec.size() - 1;

			changed = true;
		}

		//canonicalOrderHelper()

		return changed;
	}
namespace {
void vecAppend( std::vector<char>& v, const char* s ) 
{
    for( ; *s; ++s ) 
        v.push_back( *s);
}
void vecAppend( std::vector<char>& v, const char* s, size_t s_len )
{
    for( const char* s_end = s+s_len; s< s_end; ++s ) 
        v.push_back( *s);
}

} // anon namespace

int unicode_normalize_punctuation( std::string& outStr, const char* srcStr, size_t srcStr_sz ) 
{
    std::vector<char> tmp;
    tmp.reserve( srcStr_sz+16 );

    const char * s_beg = srcStr, *s_end =srcStr+ srcStr_sz, *s_end_2= ( s_end > s_beg+2 ? s_end -2: 0), *s_end_1 = (s_end > s_beg ? s_end -1: 0),
        *s_end_3= ( s_end > s_beg+3 ? s_end -3: 0);
    for( const char* s = s_beg; s< s_end; ++s ) {
        char c= *s;
        const uint8_t uc = (uint8_t)(c);
        const uint8_t lastUc = ( tmp.empty() ? 0 : tmp.back() );
        if( (uc >= 0xf0 && uc <= 0xff) ) { // 4 character unicode 
            if(  s< s_end_3 ) {
                tmp.push_back( s[0] );
                tmp.push_back( s[1] );
                tmp.push_back( s[2] );
                tmp.push_back( s[3] );
                s+= 3;
            } else {
                s= s_end_1;
            }
            continue;
        }
        if( uc == 0xd7 ) { // hedbrew
            if( s< s_end_1 ) { 
                tmp.push_back( s[0] );
                tmp.push_back( s[1] );
                s+=1;
            }
            continue;
        }
        if( isascii(c) ) {
            switch(c) {
            case '`': c = '\''; break;
            }
        } else if( !(uc == 0xd0 || uc == 0xd1 ) && !(lastUc>=0xc0 && lastUc<=0xd6) ) { // non russian chars
            switch((int)(uc)) {
            case 160: c=' '; break;
            case 161: c='!'; break; 
            case 162: c=' '; break; //cent;  cent
            case 163: c=' '; break; //pound; pound
            case 164: c=' '; break; //curren;    currency
            case 165: c='Y'; break; //yen;   yen
            case 166: c=':'; break; //brvbar;    broken vertical bar
            case 167: c=';'; break; //sect;  section
            case 168: c=' '; break; //uml;   spacing diaeresis
            case 169: vecAppend( tmp, "(c"); c=')'; break; //copy;  copyright
            case 170: c=';'; break;//ordf;  feminine ordinal indicator
            case 171: c='"'; break; //laquo; angle quotation mark (left)
            case 172: c='!'; break; //not;   negation
            case 173: c='-'; break; //shy;   soft hyphen
            case 174: vecAppend( tmp, "(tm" ); c=')'; break; //reg;   registered trademark
            case 175: c=' '; break; //macr;  spacing macron
            case 176: c=' '; break; //deg;   degree
            case 177: tmp.push_back('+'); c='-'; break;  //plusmn;    plus-or-minus 
            case 178:  c='2'; break;//sup2;  superscript 2
            case 179:  c='3'; break;//sup3;  superscript 3
            case 180:  c=' '; break; //acute; spacing acute
            case 181:  c=' '; break; //micro; micro
            case 182:  c='\n'; break;//para;  paragraph
            case 183:  c='.'; break; //middot;    middle dot
            case 184:  c=' '; break; //cedil; spacing cedilla
            case 185:  c='1'; break; //sup1;  superscript 1
            case 186:  c=' '; break; //ordm;  masculine ordinal indicator
            case 187:  c='"'; break; //raquo; angle quotation mark (right)
            case 188:  vecAppend( tmp, " 1/"); c='4'; break; //frac14;    fraction 1/4
            case 189:  vecAppend( tmp, " 1/"); c='2'; break;//frac12;    fraction 1/2
            case 190:  vecAppend( tmp, " 3/"); c='4'; break;//frac34;    fraction 3/4
            case 191:  c='"'; break;//iquest;    inverted question mark
            case 215:  c='*'; break;//times; multiplication
            case 247:  c='/'; break; //divide;    division
            default: {
                auto oldS = s;
                if( (uc >= 0xe0 && uc <= 0xef) && s< s_end_2 ) { // 3 char unicode 
                    switch( uc ) {
                    case 0xe2: 
                        switch( (uint8_t)(s[1]) ) {
                        case 0x80:
                            switch( (uint8_t)(s[2]) ) {
                                // single quote
                                case 0x98: case 0x99: case 0x9b: case 0x9a: case 0xb2: case 0xb5:
                                    s+=2; c = '\''; break;
                                // double quote
                                case 0x9c: case 0x9d: case 0x9e: case 0x9f: case 0xb3: case 0xb6:
                                    s+=2; c = '"'; break;
                                // hyphens
                                case 0x90: case 0x91: case 0x92: case 0x93: case 0x94: case 0x95:
                                    s+=2; c = '-'; break;
                                // dots
                                case 0xa4: case 0xa7: case 0xa6:
                                    s+=2; c = '.'; break;
                                case 0x3a: 
                                    s+=2; c = ' '; break;
                            }
                            break;
                        case 0x81:
                            switch( (uint8_t)(s[2]) ) {
                            case 0x83:
                                s+=2; c = '-'; break;
                            case 0x8f:
                                s+=2; c = ';'; break;
                            }
                            break;
                        case 0x82:
                            if( !isascii(s[2]) ) {
                                vecAppend( tmp, s, 3 );
                                s+= 2;
                            }
                                continue;
                        case 0x84:
                            switch( (uint8_t)(s[2]) ) {
                            case 0xa2: // (tm)
                                vecAppend( tmp, s, 3 );
                                s+= 2;
                                continue;
                            case 0x96:
                                s+=2; c='#'; break;
                                break;
                            }
                            break;
                        }
                    default: 
                        tmp.push_back( s[0] );
                        tmp.push_back( s[1] );
                        tmp.push_back( s[2] );
                        s+= 2;
                        continue;
                    }
                }
                if( (s == oldS) && s< s_end_1 ) { // two byte 
                    switch( uc ) {
                    case 0xc2:  // double quotes russian style
                        if( (uint8_t)(s[1]) == 0xab || (uint8_t)(s[1]) == 0xbb ) {
                            ++s; c='"';
                        } else if( (uint8_t)(s[1]) == 0x98 ) { // dash
                            ++s; c='-';
                        } else if( (uint8_t)(s[1]) == 0xa0 ) { // space (russian) 
                            ++s; c=' ';
                        }
                        break;
                    default: 
                        break;
                    }
                } 
            } // end of default
                break;
            }
        } else {
            tmp.push_back( s[0] );
            tmp.push_back( s[1] );
            if( s< s_end ) {
                ++s;
                continue;
            }
            else 
                break;
        }
        tmp.push_back( c );
    }
    outStr.assign( &(tmp[0]), tmp.size() );
    return 0;
}
int unicode_normalize_punctuation( std::string& qstr ) 
{
    return unicode_normalize_punctuation( qstr, qstr.c_str(), qstr.length() );
}
} // namespace ay
