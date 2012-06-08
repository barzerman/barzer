#include "ay_utf8.h"
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <algorithm>
#include "tables/u2l.cpp"
#include "tables/l2u.cpp"

namespace ay
{
	namespace
	{
		bool fstComp(uint32_t *left, uint32_t right)
		{
			return left[0] < right;
		}
	}

	bool CharUTF8::toLower()
	{
		const uint32_t utf32 = toUTF32();
		typedef uint32_t p_t[2];
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
		typedef uint32_t p_t[2];
		p_t *end = tableL2U + sizeof(tableL2U) / sizeof(tableL2U[0]);
		p_t *pair = std::lower_bound(tableL2U, end, utf32, fstComp);
		if (pair == end || (*pair)[0] != utf32)
			return false;
		
		setUTF32((*pair)[1]);
		return true;
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
}
