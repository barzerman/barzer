#include "barzer_relbits.h"
#include <fstream>
#include <sstream>
#include <ay/ay_logger.h>
#include <ay/ay_util.h>

namespace barzer
{
	RelBitsMgr::RelBitsMgr()
	: m_bits(static_cast<std::size_t>(RBMAX), false)
	{
	}

	RelBitsMgr& RelBitsMgr::inst()
	{
		static RelBitsMgr inst;
		return inst;
	}

	void RelBitsMgr::reparse (const char *filename)
	{
		m_bits = std::vector<bool>(static_cast<std::size_t>(RBMAX), false);

		std::ifstream istr(filename);
        ay::InputLineReader reader( istr );
        size_t line = 0;
        while (reader.nextLine()&& reader.str.length()) {
            if( reader.str[0] == '#' ) 
                continue;
            std::stringstream sstr(reader.str);

            size_t bitNum=0xffffffff;
            sstr >> bitNum;

            if( !setBit(bitNum) ) {
                AYLOG(ERROR) << "failed to set bit " << bitNum << " on line " << line << std::endl;
            }
            ++line;
        }
	}
}
