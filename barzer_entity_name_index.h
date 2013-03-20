#pragma once
#include <barzer_entity.h>
#include <barzer_spell_features.h>

namespace barzer {

class StoredUniverse;

class BENI {
    ay::UniqueCharPool d_charPool;
public:
    NGramStorage<BarzerEntity> d_storage;
    StoredUniverse& d_universe;

    /// add all entities by name for given class 
    void addEntityClass( const StoredEntityClass& ec );
    void search( BENIFindResults_t&, const char* str ) const;

    void clear() { d_storage.clear(); }
    BENI( StoredUniverse& u ) : 
        d_storage(d_charPool), d_universe(u) {}
};

} // namespace barzer
