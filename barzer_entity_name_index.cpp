#include <barzer_entity_name_index.h>

namespace barzer {
void BENI::addEntityClass( const StoredEntityClass& ec )
{
    /// iterate over entities of ec 
    const auto& theMap = d_universe.getDtaIdx().entPool.getEuidMap();
    BarzerEntity id( ec, 0 );
    size_t numNames =0;
    std::vector<char> tmpBuf;
    std::string dest;
    for( auto i = theMap.lower_bound( id );i!= theMap.end() && i->first.eclass == ec; ++i ) {
        const EntityData::EntProp* edata = d_universe.getEntPropData( i->first );
        if( edata && !edata->canonicName.empty() ) {
            Lang::stringToLower( tmpBuf, dest, edata->canonicName );
			d_storage.addWord( dest.c_str(), i->first );

            
            ++numNames;
        }
    }
    std::cerr << "BENI: " << numNames << " names for " << ec << std::endl;
}

void BENI::search( BENIFindResults_t& out, const char* query ) const
{

    std::vector< NGramStorage<BarzerEntity>::FindInfo > vec;
	
    std::vector<char> tmpBuf;
    std::string dest;
	Lang::stringToLower( tmpBuf, dest, std::string(query) );
    d_storage.getMatches( dest.c_str(), dest.size(), vec );
	
    if( !vec.empty() ) 
        out.reserve( vec.size() );

    for( const auto& i : vec ) {
        if( i.m_data ) {
            out.push_back({ *(i.m_data), i.m_levDist, i.m_coverage });
        }
    }
}

} // namespace barzer
