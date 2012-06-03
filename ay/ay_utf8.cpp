#include "ay_utf8.h"
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <algorithm>

namespace ay
{
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
    /* 
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

}
