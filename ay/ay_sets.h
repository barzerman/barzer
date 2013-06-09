
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
///

#pragma once

#include <iterator>
#include <algorithm>
#include <vector>

namespace ay
{
	template<typename T>
	struct Range
	{
		T first;
		T second;
		size_t skip;
	};
	
	template<typename T>
	using RangesVec = std::vector<Range<T>>;

	template<typename T>
	RangesVec<T> findXSections(T hayStart, T hayEnd, T neeStart, T neeEnd, size_t minLength = 0, size_t skipLength = 0)
	{
		if (neeStart == neeEnd || hayStart == hayEnd)
			return {};

		const auto cols = static_cast<size_t>(std::distance(neeStart, neeEnd));
		const auto rows = static_cast<size_t>(std::distance(hayStart, hayEnd));
		std::vector<std::vector<bool>> matrix(cols, std::vector<bool>(rows, false));
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

		RangesVec<T> result;
		for (size_t c = 0; c < cols - minLength; ++c)
		{
			size_t currentSkip = 0;
			size_t totalSkip = 0;
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
				[](const typename RangesVec<T>::value_type& l, const typename RangesVec<T>::value_type& r)
					{ return std::distance(l.first, l.second) > std::distance(r.first, r.second); });
		return result;
	}
}
