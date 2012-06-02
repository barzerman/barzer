#include "ay_utf8.h"
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <algorithm>

namespace ay
{
	CharUTF8::CharUTF8 ()
	: m_size (0)
	{
		std::memset (m_buf, 0, MaxBytes);
	}

	/*
	 * Bits	Last cp			Byte 1		Byte 2		Byte 3		Byte 4
	 * 7	U+007F			0xxxxxxx
	 * 11	U+07FF			110xxxxx	10xxxxxx
	 * 16	U+FFFF			1110xxxx	10xxxxxx	10xxxxxx
	 * 21	U+1FFFFF		11110xxx	10xxxxxx	10xxxxxx	10xxxxxx
	 *
	 * Shifts:
	 * 3 →   11110 → 0x1E
	 * 4 →    1110 → 0xE
	 * 5 →     110 → 0x6
	 * 7 →       0 → 0x0
	 */
    /* what is this for? i dont know (AY)
	CharUTF8::CharUTF8 (const char *beginning)
	: m_size (1)
	, m_int (0)
	{
		unsigned char first = *beginning;
		m_buf [0] = first;

		char vals [] = { 0x0, 0x6, 0xE, 0x1E };
		for (; m_size < MaxBytes && first >> (6 - m_size) >= vals [m_size];
				++m_size)
			m_buf [m_size] = beginning [m_size];
	}
    */ 

	StrUTF8::StrUTF8 ()
	{
		appendZero ();
	}

	StrUTF8::StrUTF8 (const char *string)
	{
		const char *begin = string;

		size_t lastPos = 0;
		while (*string)
		{
			const size_t glyphSize = CharUTF8 (string).size ();
			m_positions.push_back (lastPos);
			lastPos += glyphSize;
			string += glyphSize;
		}

		m_buf.reserve (lastPos);
		std::copy (begin, string, std::back_inserter (m_buf));
		appendZero ();
	}

	void StrUTF8::clear ()
	{
		m_buf.clear ();
		m_positions.clear ();

		appendZero ();
	}

	/*

	StrUTF8& StrUTF8::operator+= (const StrUTF8& other)
	{
		std::copy (other.begin (), other.end (), std::back_inserter (*this));
		return *this;
	}

	StrUTF8::iterator StrUTF8::erase (StrUTF8::iterator pos)
	{
		return m_chars.erase (pos);
	}

	StrUTF8::iterator StrUTF8::erase (StrUTF8::iterator first, StrUTF8::iterator last)
	{
		return m_chars.erase (first, last);
	}

	void StrUTF8::insert (StrUTF8::iterator at, const StrUTF8::value_type& c)
	{
		m_chars.insert (at, c);
	}

	void StrUTF8::insert (StrUTF8::iterator at, const StrUTF8& str)
	{
		m_chars.insert (at, str.begin (), str.end ());
	}
	*/

	void StrUTF8::swap (StrUTF8::iterator first, StrUTF8::iterator second)
	{
		const CharUTF8& tmp = *first;
		*first = *second;
		*second = tmp;
	}

	/*
	StrUTF8 operator+ (const StrUTF8& a, const StrUTF8& b)
	{
		StrUTF8 result (a);
		result += b;
		return result;
	}
	*/
}
