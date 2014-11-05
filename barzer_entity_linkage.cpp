#include <barzer_entity_linkage.h>
#include <barzer_global_pools.h>
#include <barzer_dtaindex.h>
#include "ay/ay_fileiter.h"

namespace barzer {

int GlobalEntityLinkage::loadFile( GlobalPools& gp, const std::string& fileName, const std::string& idxName, const StoredEntityClass& leftClass, const StoredEntityClass& rightClass, int16_t defWeight)
{
    size_t recCount = 0;
    const size_t tok_l = 0,  // left id 
        tok_r = tok_l+1,   // right id 
        tok_weight = tok_r+1;  // weight 
    
    const char sep_char = '\t';

    auto& idx = produceIndex(idxName.c_str());
    auto& entityIndex = gp.getDtaIdx();

    ay::parse_ascii_file( fileName.c_str(), sep_char, [&](const std::vector<const char*>& col) -> int {
        if( col.size() <= tok_r )
            return 0;
        const char* l = col[tok_l], *r = col[tok_r];
        uint32_t lTokId = gp.internString_internal(l), rTokId = gp.internString_internal(r);

        entityIndex.addGenericEntity( lTokId, leftClass.ec, leftClass.subclass );
        entityIndex.addGenericEntity( rTokId, rightClass.ec, rightClass.subclass );

        int16_t w = ( col.size() > tok_weight ? (int16_t)(atoi(col[tok_weight])): defWeight);
        idx.link( lTokId, rTokId, w );
        return 0;
    });
    return recCount;
}

} //namespace barzer
