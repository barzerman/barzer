#pragma once
#include <map>
#include <string>
#include <iostream>
#include <ay/ay_linkage.h>
#include <barzer_entity.h>

namespace barzer {
class GlobalPools;

class GlobalEntityLinkage {
    typedef ay::weighed_hash_linkage<> EntLinkIndex;
    boost::unordered_map<std::string, EntLinkIndex> d_map;
public:
    const EntLinkIndex* getIndex( const char* name ) const
    {
        auto i = d_map.find(name);
        return ( i== d_map.end()? 0: &(i->second));
    }
    EntLinkIndex& produceIndex(const char* name)
        { return d_map.insert({std::string(name), EntLinkIndex()}).first->second; }
    
    int loadFile(GlobalPools& gp, const std::string& fileName, const std::string& idxName, const StoredEntityClass& leftClass, const StoredEntityClass& rightClass, int16_t defWeight=1);
};

} // namespace barzer
