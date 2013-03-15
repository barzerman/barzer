#include <barzer_entity_name_index.h>

namespace barzer {
void BarzerEntityNameIndex::addEntityClass( const StoredEntityClass& ec )
{
    /// iterate over entities of ec 
    const auto& theMap = d_universe.getDtaIdx().entPool.getEuidMap();
    BarzerEntity id( ec, 0 );
    for( auto i = theMap.lower_bound( id );i!= theMap.end() && i->first.eclass == ec; ++i ) {
        const EntityData::EntProp* edata = d_universe.getEntPropData( i->first );
        if( edata && !edata->canonicName.empty() ) {
            d_storage.addWord( i->second, edata->canonicName.c_str(), i->first );
        }
    }
}

void BarzerEntityNameIndex::search( EntityLevDistVec& out, const char* query ) const
{

    std::vector< NGramStorage<BarzerEntity>::FindInfo > vec;
    d_storage.getMatches( query, strlen(query), vec );
    if( !vec.empty() ) 
        out.reserve( vec.size() );

    for( const auto& i : vec ) {
        if( i.m_data ) {
            out.push_back( { *(i.m_data), i.m_levDist } );
        }
    }
}

} // namespace barzer
