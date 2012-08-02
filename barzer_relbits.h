#pragma once

#include <vector>

namespace barzer
{
	class RelBitsMgr
	{
		std::vector<bool> m_bits;
	public:
        enum { RBMAX=1024 };
		RelBitsMgr();

		static RelBitsMgr& inst();
		inline bool check(size_t b) const 
            { return ( b< m_bits.size() ? m_bits[b] : false ); }

		void reparse(const char*);
	private:
        bool setBit(size_t b, bool v=true) 
        {
            if( b< m_bits.size() ) {
                m_bits[b]= v;
                return true;
            } else {
                return false;
            }
        }
	};
}
