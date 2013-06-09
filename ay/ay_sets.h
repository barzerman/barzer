
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
///

#pragma once

#include <iterator>
#include <algorithm>
#include <vector>
#include "ay_utf8.h"

namespace ay
{
struct SetXSection
{
	template<typename T>
	struct Range
	{
		T first;
		T second;
		double skip;
	};
    
    size_t skipLength, minLength;
	
    SetXSection() : skipLength(0), minLength(0) {}
    
	template<typename T>
	std::vector<Range<T>> compute(T hayStart, T hayEnd, T neeStart, T neeEnd) const
	{ 
		if (neeStart == neeEnd || hayStart == hayEnd)
			return {};

		const auto cols = static_cast<size_t>(std::distance(neeStart, neeEnd));
		const auto rows = static_cast<size_t>(std::distance(hayStart, hayEnd));
		std::vector<std::vector<uint8_t>> matrix(cols, std::vector<uint8_t>(rows, false));
		for (auto n = neeStart; n != neeEnd; ++n)
		{
			const auto c = *n;

			auto& col = matrix[std::distance(neeStart, n)];

			auto pos = std::find(hayStart, hayEnd, c);
			while (pos != hayEnd)
			{
				col[std::distance(hayStart, pos)] = true;

				std::advance(pos, 1);
				pos = std::find(pos, hayEnd, c);
			}
		}

		std::vector<Range<T>> result;
		for (size_t c = 0; c < cols - minLength; ++c)
		{
			size_t currentSkip = 0;
			double totalSkip = 0;
			for (size_t r = 0; r < rows - minLength; ++r)
			{
				if (!matrix[c][r])
					continue;

				size_t nc = c, nr = r;
				while (nc < cols && nr < rows)
				{
					if (!matrix[nc][nr])
					{
						if (currentSkip >= skipLength)
						{
							nc -= currentSkip;
							nr -= currentSkip;
							break;
						}
						else
							++currentSkip;
					}
					else
					{
						totalSkip += currentSkip;
						currentSkip = 0;
					}
					
					matrix[nc][nr] = false;
					++nc;
					++nr;
				}

				if (nc - c < minLength)
					continue;

				auto rStart = neeStart, rEnd = neeStart;
				std::advance(rStart, c);
				std::advance(rEnd, nc);
				result.push_back({ rStart, rEnd, totalSkip });
			}
		}
		
		std::sort(result.begin(), result.end(),
				[](const Range<T>& l, const Range<T>& r)
					{ return std::distance(l.first, l.second) > std::distance(r.first, r.second); });
		return result;
	}
    
    typedef std::pair<size_t, double> FindLongestResult;
	
	template<typename T>
	FindLongestResult findLongest(T hayStart, T hayEnd, T neeStart, T neeEnd) const 
	{
		const auto& res = compute(hayStart, hayEnd, neeStart, neeEnd );
		FindLongestResult result { 0, 0 };
		if (!res.empty())
		{
			result.first = std::distance(res[0].first, res[0].second);
			result.second = res[0].skip;
		}
		return result;
	}
	
	FindLongestResult findLongest(const ay::StrUTF8& s1, const ay::StrUTF8& s2) const
	{
		const auto& chars1 = s1.getChars();
		const auto& chars2 = s2.getChars();
		return findLongest(chars1.begin(), chars1.end(), chars2.begin(), chars2.end());
	}
    
    FindLongestResult findLongest( const std::string& s1, const std::string& s2 ) const 
        { return findLongest( s1.begin(), s1.end(), s2.begin(), s2.end() ); }

    FindLongestResult findLongest( const char *s1, const char *s2 ) const
    {
        const char *s1_end = s1+strlen(s1), *s2_end = s2+strlen(s2);
        return findLongest( s1, s1_end, s2, s2_end );
    }
};

} // anon namespace 
