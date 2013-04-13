#include <barzer_beni.h>

namespace barzer {
void BENI::addEntityClass( const StoredEntityClass& ec )
{
    /// iterate over entities of ec 
    const auto& theMap = d_universe.getDtaIdx().entPool.getEuidMap();
    BarzerEntity id( ec, 0 );
    size_t numNames =0;
    std::vector<char> tmpBuf;
    std::string dest;
    std::string normDest;
    for( auto i = theMap.lower_bound( id );i!= theMap.end() && i->first.eclass == ec; ++i ) {
        const EntityData::EntProp* edata = d_universe.getEntPropData( i->first );
        if( edata && !edata->canonicName.empty() ) {
            Lang::stringToLower( tmpBuf, dest, edata->canonicName );
            normalize( normDest, dest );
			d_storage.addWord( normDest.c_str(), i->first );
            
            ++numNames;
        }
    }
    std::cerr << "BENI: " << numNames << " names for " << ec << std::endl;
}

void BENI::search( BENIFindResults_t& out, const char* query, double minCov ) const
{
    out.clear();
    std::vector< NGramStorage<BarzerEntity>::FindInfo > vec;
	
    std::vector<char> tmpBuf;
    std::string dest;
    std::string normDest;
	Lang::stringToLower( tmpBuf, dest, std::string(query) );
    normalize( normDest, dest );
    d_storage.getMatches( normDest.c_str(), normDest.length(), vec );
	
    if( !vec.empty() ) 
        out.reserve( vec.size() );

    for( const auto& i : vec ) {
        if( i.m_data && i.m_coverage>= minCov ) {
            bool isNew = true;
            for( const auto& x : out ) {
                if( *(i.m_data) == x.ent ) {
                    isNew = false;
                    break;
                }
            }
            if( isNew )
                out.push_back({ *(i.m_data), i.m_levDist, i.m_coverage, i.m_relevance });
        }
    }
}

bool BENI::normalize( std::string& out, const std::string& in ) 
{
    out.clear();
    out.reserve( in.length() );

    size_t in_length = in.length(), in_length_1 = ( in_length ? in_length -1 : 0 );
    
    enum {
        CT_CHAR, //  everything that's not a digit or pucntspace is considered a char 
        CT_DIGIT , // isdigit
        CT_PUNCTSPACE //ispunct or isspace
    };
    #define GET_CT(c) ( (!c || ispunct(c)||isspace(c))? CT_PUNCTSPACE: (isdigit(c)? CT_DIGIT: CT_CHAR)  )
    bool altered = false;
    for( size_t i = 0; i< in_length; ++i ) {
        char prevC = ( i>0 ? in[i-1] : 0 );
        char nextC = ( i< in_length_1 ? in[i+1] : 0 );
        char c = in[i];

        int ct_prev = GET_CT(prevC); 
        int ct_next = GET_CT(nextC);
        int ct = GET_CT(c);

        if( ct == CT_PUNCTSPACE ) {
            if( ct_prev != CT_PUNCTSPACE && ct_prev== ct_next )
                out.push_back( ' ' );
            else if( !altered )
                altered = true;
        } else {
            out.push_back( c );
        }
    }
    return altered;
}

} // namespace barzer
