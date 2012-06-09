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

	bool StrUTF8::hasLower(const char *s, size_t size)
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

	bool StrUTF8::hasUpper(const char *s, size_t size)
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
}
