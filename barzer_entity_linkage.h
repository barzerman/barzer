#pragma once
#include <map>
#include <string>
#include <iostream>
#include <ay/ay_linkage.h>
#include <barzer_entity.h>

namespace barzer {
class GlobalPools;

class GlobalEntityLinkage {
public:
    typedef ay::weighed_hash_linkage<> EntLinkIndex;
private:
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

struct EntityLinkageFilter {
    const GlobalEntityLinkage::EntLinkIndex* d_idx;
    BarzerEntity d_ent;
    double val_at0, one_at_val;

    EntityLinkageFilter() : 
        d_idx(0), 
        val_at0(0), 
        one_at_val(0) 
    {}

    EntityLinkageFilter(const GlobalEntityLinkage::EntLinkIndex* idx, const BarzerEntity& ent, double v0, double v1=1) : 
        d_idx(idx), 
        d_ent(ent), 
        val_at0(v0), 
        one_at_val(v1) 
    {}

    void clear() { val_at0= one_at_val = 0.0; d_idx=0;}

    bool valid() const { return d_idx && val_at0 >=0 && one_at_val >= 0; } 
    operator bool() const { return valid(); }

    double operator()( const BarzerEntity& ent, double score ) const
    {
        if( !d_idx ) 
            return 0;
        else  {
            int16_t w = d_idx->getWeight(d_ent.tokId, ent.tokId);

            if( w<= 0 )
                return ( val_at0 ? val_at0 * score : 0 );
            else if( w >= one_at_val ) 
                return score;
            else
                return ( one_at_val ? (1.0 - val_at0)/one_at_val + val_at0: score);
        }
            
        return 1.0;
    }
};

} // namespace barzer
