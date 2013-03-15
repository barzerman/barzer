#pragma once
#include <barzer_spell_features.h>
#include <barzer_entity.h>
namespace barzer {

class StoredUniverse;

class BarzerEntityNameIndex {
public:
    typedef NGramStorage<BarzerEntity> FindInfo;
    typedef NGramStorage<BarzerEntity> Storage;
    Storage d_storage;
    StoredUniverse& d_universe;

    typedef std::pair< BarzerEntity, size_t > EntityLevDist; // entity + levenshtein distance to query
    typedef std::vector< EntityLevDist >      EntityLevDistVec; 
    /// add all entities by name for given class 
    void addEntityClass( const StoredEntityClass& ec );
    void search( EntityLevDistVec&, const char* str );

    void clear() { d_storage.clear(); }
};

} // namespace barzer
