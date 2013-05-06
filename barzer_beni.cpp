#include <barzer_beni.h>

namespace barzer {

SmartBENI::SmartBENI( StoredUniverse& u ) : 
    d_beniStraight(u),
    d_beniSl(u),
    d_isSL(u.checkBit( StoredUniverse::UBIT_BENI_SOUNDSLIKE))
{
    if( d_isSL ) {
        d_beniSl.setSL( true );
        d_beniStraight.setSL( false );
    }
}
void SmartBENI::addEntityClass( const StoredEntityClass& ec )
{
    /// iterate over entities of ec 
    const auto& theMap = d_beniStraight.d_universe.getDtaIdx().entPool.getEuidMap();
    BarzerEntity id( ec, 0 );
    size_t numNames =0;
    std::vector<char> tmpBuf;
    std::string dest;
    std::string normDest;
    for( auto i = theMap.lower_bound( id );i!= theMap.end() && i->first.eclass == ec; ++i ) {
        const EntityData::EntProp* edata = d_beniStraight.d_universe.getEntPropData( i->first );
        if( edata && !edata->canonicName.empty() ) {
            Lang::stringToLower( tmpBuf, dest, edata->canonicName );
            BENI::normalize( normDest, dest );
			d_beniStraight.d_storage.addWord( normDest.c_str(), i->first );
            if( d_isSL ) 
			    d_beniSl.d_storage.addWord( normDest.c_str(), i->first );
            
            ++numNames;
        }
    }
    std::cerr << "BENI: " << numNames << " names for " << ec << std::endl;
}
void SmartBENI::search( BENIFindResults_t& out, const char* query, double minCov ) const
{
    double maxCov = d_beniStraight.search( out, query, minCov );
    const double SL_COV_THRESHOLD= 0.7;

    if( d_isSL ) {
        if( maxCov< SL_COV_THRESHOLD || out.empty() ) {
            BENIFindResults_t slOut;
            double maxCov = d_beniSl.search( slOut, query, minCov );
            size_t numAdded = 0;
            for( const auto& i: slOut ) {
                BENIFindResults_t::iterator outIter = std::find_if(out.begin(), out.end(), [&]( const BENIFindResult& x ) { return ( x.ent == i.ent ) ; });
                if( out.end() == outIter ) {
                    out.push_back( i );
                    ++numAdded;
                } else
                    outIter->coverage = i.coverage;
            }
        }
    }
    std::sort( out.begin(), out.end(), []( const BENIFindResults_t::value_type& l, const BENIFindResults_t::value_type& r ) { return (l.coverage > r.coverage); } );
    if( out.size() > 128 ) 
        out.resize(128);
}

BENI& SmartBENI::getPrimaryBENI()
{
	return d_beniStraight;
}

void SmartBENI::clear()
{
    d_beniSl.clear();
    d_beniStraight.clear();
}

void BENI::setSL( bool x )
{
    d_storage.setSLEnabled( x );
}

BENI::BENI( StoredUniverse& u ) : 
    d_storage(d_charPool),
    d_universe(u)
{}

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
            BENI::normalize( normDest, dest );
			d_storage.addWord( normDest.c_str(), i->first );
            
            ++numNames;
        }
    }
    std::cerr << "BENI: " << numNames << " names for " << ec << std::endl;
}

namespace {

size_t compute_cutoff_by_coverage( std::vector< NGramStorage<BarzerEntity>::FindInfo >& vec )
{
    if( vec.empty() ) 
        return 0;
    double topCov = vec[0].m_coverage;

    double diff = 0;
    size_t i = 1;
    for( ; i< vec.size(); ++i ) {
        if( vec[i].m_coverage + 0.1 < topCov ) 
            return i;
    }
    return i;
}

} //end of anon namespace 
double BENI::search( BENIFindResults_t& out, const char* query, double minCov ) const
{
    double maxCov = 0.0;
    out.clear();
    std::vector< NGramStorage<BarzerEntity>::FindInfo > vec;
	
    std::vector<char> tmpBuf;
    std::string dest;
    std::string normDest;
	Lang::stringToLower( tmpBuf, dest, std::string(query) );
    normalize( normDest, dest );
    enum { MAX_BENI_RESULTS = 64 };
    d_storage.getMatches( normDest.c_str(), normDest.length(), vec, MAX_BENI_RESULTS, minCov );
	
    if( !vec.empty() ) 
        out.reserve( vec.size() );
    size_t cutOffSz = compute_cutoff_by_coverage( vec );
    if( cutOffSz< vec.size() ) 
        vec.resize( cutOffSz );

    for( const auto& i : vec ) {
        if( !i.m_data )
            continue;
        if( i.m_coverage > maxCov )
            maxCov = i.m_coverage;

        if( i.m_coverage>= minCov ) {
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
    return maxCov;
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
