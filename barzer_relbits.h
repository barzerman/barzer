#pragma once

#include <vector>

namespace barzer
{
	enum class RelBit
	{
		Synonyms,
		RBMAX
	};

	class RelBitsMgr
	{
		std::vector<bool> m_bits;
	public:
		RelBitsMgr();

		static RelBitsMgr& inst();
		inline bool check(RelBit b) const { return m_bits[static_cast<std::size_t>(b)]; }

		void reparse(const char*);
	private:
		void setBit(RelBit b, bool val) { m_bits[static_cast<std::size_t>(b)] = val; }
	};
}
