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
		p_t *end = tableU2L + sizeof(tableU2L) / sizeof(tableU2L[0]);
		p_t *pair = std::lower_bound(tableU2L, end, utf32, fstComp);
		return pair != end && (*pair)[0] == utf32;
	}

	bool CharUTF8::isUpper() const
	{
		const uint32_t utf32 = toUTF32();
		p_t *end = tableL2U + sizeof(tableL2U) / sizeof(tableL2U[0]);
		p_t *pair = std::lower_bound(tableL2U, end, utf32, fstComp);
		return pair != end && (*pair)[0] == utf32;
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

	bool StrUTF8::hasLower() const
	{
		for (size_t i = 0, sz = size(); i < sz; ++i)
			if (getGlyph(i).isLower())
				return true;
		return false;
	}

	bool StrUTF8::hasUpper() const
	{
		for (size_t i = 0, sz = size(); i < sz; ++i)
			if (getGlyph(i).isUpper())
				return true;
		return false;
	}

	bool StrUTF8::hasLower (const char *s, size_t size)
	{
		while(s < s + size)
		{
			const CharUTF8 ch(s);
			if (ch.isLower())
				return true;

			s += ch.size();
		}
		return false;
	}

	bool StrUTF8::hasUpper (const char *s, size_t size)
	{
		while(s < s + size)
		{
			const CharUTF8 ch(s);
			if (ch.isUpper())
				return true;

			s += ch.size();
		}
		return false;
	}
}
