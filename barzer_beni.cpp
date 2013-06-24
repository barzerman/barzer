#include <barzer_beni.h>
#include <zurch_docidx.h>
#include <ay/ay_sets.h>

namespace barzer {

SmartBENI::SmartBENI( StoredUniverse& u ) : 
    d_beniStraight(u),
    d_beniSl(u),
    d_isSL(u.checkBit( StoredUniverse::UBIT_BENI_SOUNDSLIKE)),
    d_universe(u),
    d_zurchUniverse(0)
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
			d_beniStraight.addWord(normDest, i->first);
            if( d_isSL ) 
			    d_beniSl.addWord(normDest, i->first);
            
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
            maxCov = d_beniSl.search( slOut, query, minCov );
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

const NGramStorage<BarzerEntity>& BENI::getStorage() const
{
	return d_storage;
}

void BENI::addWord(const std::string& str, const BarzerEntity& ent)
{
	d_storage.addWord( str.c_str(), ent );
	d_backIdx.insert({ ent, str });
}

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
			addWord(normDest, i->first);
            
            ++numNames;
        }
    }
    std::cerr << "BENI: " << numNames << " names for " << ec << std::endl;
}

namespace
{
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

	// for =zero length val isn't changed, for bigger lengths
	// val goes to threshold
	double calcPenalty(double val, double length)
	{
		// val → y
		// length → x
		const double threshold = 0.5;
		const double smoothness = 0.1;
		const double affect = 0.7;

		return val * ((1 - affect) + affect / (1 + exp ((length - threshold) / smoothness)));
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
    size_t query_len = strlen(query);
    std::string queryStr(query);
	Lang::stringToLower( tmpBuf, dest, queryStr );
    normalize( normDest, dest );
    enum { MAX_BENI_RESULTS = 64 };
    d_storage.getMatches( normDest.c_str(), normDest.length(), vec, MAX_BENI_RESULTS, minCov );
	
    if( !vec.empty() ) 
        out.reserve( vec.size() );
    size_t cutOffSz = compute_cutoff_by_coverage( vec );
    if( cutOffSz< vec.size() ) 
        vec.resize( cutOffSz );

	ay::SetXSection xsect;
	xsect.minLength = 10;
	xsect.skipLength = 1;

    enum { MIN_MATCH_BOOST_QLEN = 10 };
    size_t queryGlyphCount = ay::StrUTF8::glyphCount(query, query+ query_len) ; 

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
			{
				auto cov = i.m_coverage;
				if (queryGlyphCount >= MIN_MATCH_BOOST_QLEN)
				{
					const auto strPos = d_backIdx.find(*(i.m_data));
					if (strPos != d_backIdx.end())
					{
						const auto& str = strPos->second;
						const ay::StrUTF8 strUtf8(str.c_str(), str.size());
						const ay::StrUTF8 normUtf8(normDest.c_str(), normDest.size());
						const auto& longest = xsect.findLongest(strUtf8, normUtf8);

						if (longest.second < 3)
						{
							const double minLength = std::min(strUtf8.size(), normUtf8.size());
							const double relLength = longest.first / minLength;

							// here we compute the penalty for the difference between coverage and 1
							cov = 1 - calcPenalty(1 - cov, relLength);
						}
					}
				}
				out.push_back({ *(i.m_data), i.m_levDist, cov, i.m_relevance });
			}
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

size_t SmartBENI::getZurchEntities( BENIFindResults_t& out, const zurch::DocWithScoreVec_t& vec ) const
{
    std::map< BarzerEntity, double > resultMap;
    for( const auto& i : vec ) {
        getEntLinkedToZurchDoc(
            [&]( const BarzerEntity& ent ) {
                auto x = resultMap.find( ent );
                if( x == resultMap.end() ) 
                    resultMap.insert( {ent,i.second} );
                else 
                    x->second += i.second;
            },
            i.first
        );
    }
    if( resultMap.empty() ) 
        return 0;

    double minScore = 0, maxScore = 0;
    for( const auto& i : resultMap ) {
        if( i.second > maxScore )
            maxScore= i.second;
        if( minScore == 0.0 || minScore > i.second ) 
            minScore = i.second;

        out.push_back( {i.first, 1, i.second, 0 } );
    }
    std::sort( out.begin(), out.end(), []( const BENIFindResult& l, const BENIFindResult& r ){ return (l.coverage> r.coverage); } );
    // normalizing doc scores to 1
    double minToMax = maxScore-minScore;
    if( minToMax > 0.00000001 ) {
        for( auto& i : out ) 
            i.coverage = (i.coverage-minScore)/minToMax;
    }
    return resultMap.size();
}

void SmartBENI::zurchEntities( BENIFindResults_t& out, const char* str, const QuestionParm& qparm )
{
    if( !d_zurchUniverse ) 
        return;

    if( const zurch::DocIndexAndLoader* zurch = d_zurchUniverse->getZurchIndex() ) {
        zurch::ExtractedDocFeature::Vec_t featureVec;
        zurch::DocWithScoreVec_t docVec;
        std::map<uint32_t, zurch::DocFeatureIndex::PosInfos_t> positions;
        zurch::DocFeatureIndex::TraceInfoMap_t barzTrace;

        Barz barz;
        barz.setUniverse( d_zurchUniverse );
        QParser qparser( *d_zurchUniverse );

        qparser.tokenize_only( barz, str, qparm );
        qparser.lex_only( barz, qparm );
        qparser.semanticize_only( barz, qparm );

        auto index = zurch->getIndex();
        if( index->fillFeatureVecFromQueryBarz( featureVec, barz ) )  {
            zurch::DocFeatureIndex::SearchParm parm( qparm.d_maxResults, 0, &positions, &barzTrace );
            index->findDocument( docVec, featureVec, parm, barz );
        }
        getZurchEntities( out, docVec );
    }
}
} // namespace barzer
