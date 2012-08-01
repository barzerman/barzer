#include "barzer_relbits.h"
#include <fstream>
#include <ay/ay_logger.h>

namespace barzer
{
	RelBitsMgr::RelBitsMgr()
	: m_bits(static_cast<std::size_t>(RelBit::RBMAX), false)
	{
	}

	RelBitsMgr& RelBitsMgr::inst()
	{
		static RelBitsMgr inst;
		return inst;
	}

	void RelBitsMgr::reparse (const char *filename)
	{
		m_bits = std::vector<bool>(static_cast<std::size_t>(RelBit::RBMAX), false);
		std::ifstream istr(filename);
		while (istr)
		{
			std::string opt;
			istr >> opt;

			if (opt == "synonyms")
				setBit(RelBit::Synonyms, true);
			else
				AYLOG(ERROR) << "unknown bit " << opt;
		}
	}
}
