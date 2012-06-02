#include "ay_utf8.h"
#include <cstdlib>
#include <cstring>
#include <iostream>

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
	CharUTF8::CharUTF8 (const char *beginning)
	: m_size (1)
	{
		std::memset (m_buf, 0, MaxBytes);

		unsigned char first = *beginning;
		m_buf [0] = first;

		char vals [] = { 0x0, 0x6, 0xE, 0x1E };
		for (; m_size < MaxBytes && first >> (6 - m_size) >= vals [m_size];
				++m_size)
			m_buf [m_size] = beginning [m_size];
	}

	CharUTF8::CharUTF8 (const CharUTF8& other)
	: m_size (other.size ())
	{
		std::memcpy (m_buf, other.m_buf, MaxBytes);
	}

	CharUTF8& CharUTF8::operator= (const char *beginning)
	{
		*this = CharUTF8 (beginning);
		return *this;
	}

	CharUTF8& CharUTF8::operator= (const CharUTF8& other)
	{
		m_size = other.size ();
		std::memcpy (m_buf, other.m_buf, MaxBytes);
		return *this;
	}

	bool CharUTF8::operator== (const CharUTF8& other) const
	{
		return m_size == other.m_size &&
			!std::memcmp (m_buf, other.m_buf, m_size);
	}

	const char* CharUTF8::getBuf () const
	{
		return m_buf;
	}

	StrUTF8::StrUTF8 ()
	{
	}

	StrUTF8::StrUTF8 (const char *string)
	{
		while (true)
		{
			if (!*string)
				break;

			const CharUTF8 ch (string);
			m_chars.push_back (ch);
			string += ch.size ();
		}
	}

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

	void StrUTF8::swapChars (StrUTF8::iterator first, StrUTF8::iterator second)
	{
		std::swap (*first, *second);
	}

	size_t StrUTF8::bytesCount () const
	{
		size_t result = 0;
		for (size_t i = 0, sz = size (); i < sz; ++i)
			result += m_chars [i].size ();
		return result;
	}

	char* StrUTF8::buildString () const
	{
		const size_t bytes = bytesCount ();
		char *result = new char [bytes + 1];
		char *current = result;
		for (size_t i = 0, sz = size (); i < sz; ++i)
		{
			const CharUTF8& ch = m_chars [i];
			std::memcpy (current, ch.getBuf (), ch.size ());
			current += ch.size ();
		}
		result [bytes] = 0;
		return result;
	}

	StrUTF8 operator+ (const StrUTF8& a, const StrUTF8& b)
	{
		StrUTF8 result (a);
		result += b;
		return result;
	}
}
