
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#pragma once

#include <vector>
#include <cstring>
#include <ostream>

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

		size_t reparse(std::ostream&, const char*);
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
