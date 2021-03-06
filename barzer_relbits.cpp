
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#include "barzer_relbits.h"
#include <fstream>
#include <sstream>
#include <ay/ay_logger.h>
#include <ay/ay_util.h>

/** Ok, so since we don't want to keep an enum for this, let's describe
 * the release bits here.
 * 
 * 1: use synonyms autoexpansion.
 */

namespace barzer
{
	RelBitsMgr::RelBitsMgr()
	: m_bits(static_cast<size_t>(RBMAX), false)
	{
	}

	RelBitsMgr& RelBitsMgr::inst()
	{
		static RelBitsMgr inst;
		return inst;
	}

	size_t RelBitsMgr::reparse (std::ostream& outStream, const char *filename)
	{
		m_bits = std::vector<bool>(static_cast<size_t>(RBMAX), false);

		FILE* fp = fopen( filename, "r" );
        outStream << "loading release bits from " << filename << "...";
        if( !fp ) {
            outStream << "failed to open " << filename << std::endl; 
            return 0;
        }
        char buf[ 256 ]; 
        size_t line=0;
        while (fgets( buf, sizeof(buf)-1, fp )) {
            size_t buf_len = strlen(buf);
            if( !buf_len )
                continue;
            size_t buf_last = buf_len-1;
            
            if( buf[0] == '#' ) 
                continue;
            if( buf[ buf_last ]=='\n' )
                buf[ buf_last ] = 0;
            std::stringstream sstr(buf);

            size_t bitNum=0xffffffff;
            sstr >> bitNum;

            if( !setBit(bitNum) ) {
                AYLOG(ERROR) << "failed to set bit " << bitNum << " on line " << line << std::endl;
            }
            ++line;
        }
        outStream << line << " lines read" << std::endl;
        fclose(fp);
        return line;
	}
}
