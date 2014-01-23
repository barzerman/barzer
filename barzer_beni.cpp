#include <barzer_beni.h>
#include <zurch_docidx.h>
#include <ay/ay_sets.h>
#include <ay/ay_util_time.h>
#include <boost/regex.hpp>

namespace barzer {

SmartBENI::SmartBENI( StoredUniverse& u ) : 
    d_beniStraight(u),
    d_beniSl(u),
    d_isSL(u.checkBit( UBIT_BENI_SOUNDSLIKE)),
    d_universe(u),
    d_zurchUniverse(0)
{
    if( d_isSL ) {
        d_beniSl.setSL( true );
        d_beniStraight.setSL( false );
    }
}

void  SmartBENI::applyMandatoryRegex( std::string& dest ) const
{
    for( const auto& p : d_mandatoryRegex )
        dest = boost::regex_replace(dest, p.first, p.second.c_str());
}
void  SmartBENI::addMandatoryRegex( const std::string& pattern, const std::string& replace ) 
{
    d_mandatoryRegex.resize( d_mandatoryRegex.size() +1 );

    d_mandatoryRegex.back().first= boost::regex(pattern, boost::regex::perl);
    if( !replace.empty() )
        d_mandatoryRegex.back().second = replace;
}

size_t SmartBENI::addEntityFile( const char* path, const char* modeStr, const StoredEntityClass& dfec, const StoredEntityClass& dfTopicEc )
{
    FILE* fp= 0;
    if( path ) {
        fp = fopen( path, "r" );
        if( !fp ) {
            AYLOG(ERROR) << "cant open file " << path << std::endl;
            return 0;
        }
        std::cerr << "reading BENI entities from " << path << " ";
    } else
        return( (std::cerr << "0 path specified\n"), 0 );

    char buf[ 1024 ];
    size_t entsRead = 0;
    std::string name;
    std::string id;
    std::string topicId;
    StoredEntityClass topicEc;

    // name mode s - name skip
    enum {
        NAME_MODE_OVERRIDE,
        NAME_MODE_SKIP
    };
    bool overrideEc = false, overrideSc = false; // endity class/subclass override with default
    int mode = NAME_MODE_OVERRIDE;
    if( modeStr ) {
        for( const char* x = modeStr; *x; ++x )  {
            switch( *x ) {
            case 's': mode=NAME_MODE_SKIP; break;
            case 'c': overrideEc = overrideSc = true; break; 
            case 'C': overrideEc = true; break;
            }
        }
    }
    enum {
        TOK_CLASS,
        TOK_SUBCLASS,
        TOK_ID,
        TOK_RELEVANCE,
        TOK_NAME,
        /// 
        TOK_TOPIC_CLASS, 
        TOK_TOPIC_SUBCLASS,
        TOK_TOPIC_ID,
        TOK_MAX
    };
    std::string tmp,  normName, lowerCase;
    std::vector<char> tmpBuf;
    ay::stopwatch timer;
    
    size_t reportEvery = 500, timesReported = 0;

    while( fgets( buf, sizeof(buf)-1, fp ) ) {
        buf[ sizeof(buf)-1 ] = 0;
        size_t len = strlen( buf );
        if( !len ) 
            continue;
        len--;
        if( buf[ len ]  == '\n' ) buf[ len ]=0;

        if( buf[0] == '#' ) continue;

        tmpBuf.clear();
        id.clear();
        name.clear();
        normName.clear();
        int relevance = 0;
        StoredEntityUniqId euid;

        size_t numTokRead = 0;
        ay::parse_separator( 
            [&]( size_t tokNum, const char* s_beg, const char* s_end ) -> int {
                numTokRead= tokNum;
                if( tokNum >= TOK_MAX ) return 1;
                switch( tokNum ) {
                case TOK_CLASS:     
                    if( overrideEc || s_end == s_beg ) {
                        euid.eclass.ec = dfec.ec;
                    } else {
                        tmp.assign(s_beg,(s_end-s_beg)); euid.eclass.ec = atoi(tmp.c_str()); 
                    }
                    break;
                case TOK_SUBCLASS:  
                    if( overrideSc || s_end == s_beg ) {
                        euid.eclass.subclass = dfec.subclass;
                    } else {
                        tmp.assign(s_beg,(s_end-s_beg)); euid.eclass.subclass = atoi(tmp.c_str()); 
                    }
                    break;
                case TOK_ID:        id.assign( s_beg, (s_end - s_beg ) ); break;
                case TOK_RELEVANCE: tmp.assign(s_beg,(s_end-s_beg)); relevance = atoi(tmp.c_str()); break;
                case TOK_NAME:      name.assign( s_beg, (s_end - s_beg ) ); break;
                case TOK_TOPIC_CLASS: 
                    if( s_end == s_beg ) { topicEc.ec = dfTopicEc.ec; } else { tmp.assign(s_beg,(s_end-s_beg)); topicEc.ec = atoi(tmp.c_str()); }
                    break;
                case TOK_TOPIC_SUBCLASS: 
                    if( s_end == s_beg ) { topicEc.subclass = dfTopicEc.subclass; } else { tmp.assign(s_beg,(s_end-s_beg)); topicEc.subclass = atoi(tmp.c_str()); }
                    break;
                case TOK_TOPIC_ID: 
                    if( s_end != s_beg ) 
                        topicId.assign( s_beg, (s_end - s_beg ) );
                    break;
                default: return 1;
                }
                return 0;
            },
            buf, 
            buf+len
        );
        if( numTokRead < TOK_NAME ) continue;
        if( id.empty() )
            continue;

        euid.tokId = d_universe.getGlobalPools().internString_internal( id.c_str() );
        StoredEntity& ent = d_universe.getGlobalPools().getDtaIdx().addGenericEntity( euid.tokId, euid.eclass.ec, euid.eclass.subclass );
        if( name.empty() ) { // empty name - it's topic linkage or trash
            if( !topicEc.isValid() )
                continue;
        } 

        if( topicEc.isValid() ) { // trying to link topic
            StoredEntityUniqId topicEuid;
            topicEuid.eclass = topicEc;
            if( !topicId.empty() )
                topicEuid.tokId = d_universe.getGlobalPools().internString_internal( topicId.c_str() );

            d_universe.linkEntTotopic( topicEuid, euid );

            if( name.empty() )
                continue;
        }

        Lang::stringToLower( tmpBuf, lowerCase, name.c_str() );
        BENI::normalize( normName, lowerCase, &d_universe );
        if( mode == NAME_MODE_OVERRIDE )
            d_universe.setEntPropData( ent.getEuid(), name.c_str(), relevance, true );
        else if( mode == NAME_MODE_SKIP )
            d_universe.setEntPropData( ent.getEuid(), name.c_str(), relevance, false );
        d_beniStraight.addWord(normName, ent.getEuid() );
        if( d_isSL ) 
            d_beniSl.addWord(normName, ent.getEuid());
        if (!(++entsRead % reportEvery)) {
            char repChar = ( reportEvery>= 10000 ? 'O' : (reportEvery>=3000 ? 'o': '.') );
            if( entsRead> 2000 )
                reportEvery = entsRead> 50000 ? 10000 : 4000;

			std::cerr << repChar;
        }
    }

    std::cerr << " " << entsRead << " entities loaded in " << timer.calcTime() << " seconds" << std::endl;
    return entsRead;    
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
    std::cerr << "Creating BENI for " << d_beniStraight.d_universe.getSubclassName(ec) ;
    for( auto i = theMap.lower_bound( id );i!= theMap.end() && i->first.eclass == ec; ++i ) {
        const EntityData::EntProp* edata = d_beniStraight.d_universe.getEntPropData( i->first );
        if( edata && !edata->canonicName.empty() ) {
            Lang::stringToLower( tmpBuf, dest, edata->canonicName );
            BENI::normalize( normDest, dest, &d_universe );
			d_beniStraight.addWord(normDest, i->first);
            if( d_isSL ) 
			    d_beniSl.addWord(normDest, i->first);
            
            ++numNames;
            if( !(numNames%5000) ) std::cerr << ".";
        }
    }
    std::cerr << " done " << numNames << " names for " << ec << std::endl;
}

int SubclassBENI::search( BENIFindResults_t& out, const char* query, const StoredEntityClass& sc , double minCov ) const
{
    if( const BENI* b = getBENI(sc) ) {
        b->search( out, query, minCov );
        return ERR_OK;
    } else
        return ERR_NO_BENI;
}

SubclassBENI::~SubclassBENI()
{
	clear();
}

void SubclassBENI::clear()
{
	for (const auto& pair : m_benies)
		delete pair.second;
	m_benies.clear();
}

void SubclassBENI::addSubclassIds(const StoredEntityClass& sec, const char *pattern, const char *replace)
{
	auto pos = m_benies.find(sec);
	if (pos == m_benies.end())
		pos = m_benies.insert({ sec, new BENI(m_universe) }).first;

	const auto& theMap = m_universe.getDtaIdx().entPool.getEuidMap();
	std::vector<char> tmpBuf;
	std::string dest;
	std::string normDest;

	boost::regex rxObj;
	if (pattern)
	{
		rxObj = boost::regex(pattern, boost::regex::perl);
		if (!replace)
			replace = "";
	}
	for (auto i = theMap.lower_bound(StoredEntityUniqId (sec, 0)), end = theMap.end(); i != end && i->first.eclass == sec; ++i)
	{
		const auto tokId = i->first.getTokId();
		const auto str = m_universe.getGlobalPools().internalString_resolve(tokId);
        if( !str )
            continue;
        dest = str;
		if (pattern)
			dest = boost::regex_replace(dest, rxObj, replace);
		Lang::stringToLower(tmpBuf, dest, str);
		BENI::normalize(normDest, dest, &m_universe);

		pos->second->addWord(normDest, i->first);
	}
}

void SmartBENI::search( 
    BENIFindResults_t& out, 
    const char* query,
    double minCov, 
    Barz* barz,
    const BENIFilter_f& filter,
    size_t maxCount) const
{
    std::string processedStr;
    if( hasMandatoryRegex() ) {
        processedStr.assign(query);
        applyMandatoryRegex(processedStr);
        query = processedStr.c_str();
    }
    double maxCov = d_beniStraight.search( out, query, minCov, filter);
    const double SL_COV_THRESHOLD= 0.85;

    if( d_isSL ) {
        if( maxCov< SL_COV_THRESHOLD || out.empty() ) {
            BENIFindResults_t slOut;
            maxCov = d_beniSl.search( slOut, query, minCov, filter);
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
    /// trying to filter by the topics
    if( barz && d_universe.checkBit( UBIT_BENI_TOPIC_FILTER ) && barz->topicInfo.hasTopics() ) {
        TopicFilter topicFilter( d_universe.getTopicEntLinkage() );
        for( const auto& i: out ) { topicFilter.addFilteredEntClass(i.ent.eclass); }
        for( const auto& i : barz->topicInfo.getTopicMap() ) { topicFilter.addTopic( i.first ); }
        topicFilter.optimize();
        BENIFindResults_t newOut;
        for( const auto& i: out ) {
            if( topicFilter.isEntityGood( i.ent ) ) 
                newOut.push_back( i );
        }
        if( out.size() != newOut.size() ) 
            out.swap( newOut );
    }
    std::sort( out.begin(), out.end(), 
        []( const BENIFindResult& l, const BENIFindResult& r ) -> bool
            { 
                const auto ll = l.coverage*(1+l.popRank), rr=r.coverage*(1+r.popRank);
                return ( rr < ll ? true : ( ll<rr ? false : l.nameLen< r.nameLen) );
                // return (l.coverage> r.coverage?  true:(r.coverage>l.coverage ? false: (l.popRank>r.popRank ? true: l.nameLen< r.nameLen))  ); 
            } 
        );

    if( out.size() > 1 && !d_universe.checkBit( UBIT_BENI_NO_COVDROPCUT ) ) { // coverage cut drop
        const double MAX_COVDROP_FROMTOP = 0.15; // max coverage difference with best result before cut off
        const double MAX_COVDROP_ADJ = 0.8; // max adjacent coverage results coverage drop before cut off
        
        double bestCov = out.front().coverage;
        size_t i = 1;
        for( size_t i_max = out.size(); i< i_max; ++i ) {
            double curCov = out[i].coverage;
            if( bestCov> curCov + MAX_COVDROP_FROMTOP ||out[i-1].coverage > curCov + MAX_COVDROP_ADJ ) 
                break;
        }
        if( i< out.size() )
            out.resize(i);
    }

    if( out.size() > maxCount ) 
        out.resize(maxCount);
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
            BENI::normalize( normDest, dest, &d_universe );
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

double BENI::search( BENIFindResults_t& out, const char* query, double minCov, const BENIFilter_f& filter) const
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
    normalize( normDest, dest, &d_universe );
    enum { MAX_BENI_RESULTS = 64 };
    d_storage.getMatches( normDest.c_str(), normDest.length(), vec, MAX_BENI_RESULTS, minCov, filter);
	
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

    bool doBoost = d_universe.checkBit( UBIT_BENI_NO_BOOST_MATCH_LEN );
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
                const BarzerEntity& theEnt = *(i.m_data);
                int popRank =0;
                size_t nameLen = 0;
                if( const EntityData::EntProp* edata = d_universe.getEntPropData( theEnt ) )  {
                    popRank = edata->relevance;
                    nameLen = edata->canonicName.length();
                }
				out.push_back(
                    BENIFindResult( theEnt, popRank, cov, i.m_relevance, nameLen ) 
                );
			}
        }
    }
    return maxCov;
}

bool BENI::normalize( std::string& out, const std::string& in, const StoredUniverse* uni ) 
{
    out.clear();
    out.reserve( 2*in.length() );

    size_t in_length = in.length(), in_length_1 = ( in_length ? in_length -1 : 0 );
    
    enum {
        CT_CHAR, //  everything that's not a digit or pucntspace is considered a char 
        CT_DIGIT , // isdigit
        CT_PUNCTSPACE, //ispunct or isspace
        CT_2BYTEJUNK // ® and such stripping off this junk
    };
    #define GET_CT(c) ( (!c || ispunct(c)||isspace(c))? CT_PUNCTSPACE: (isdigit(c)? CT_DIGIT: \
        ((uint8_t)(c) ==0xc2 ? CT_2BYTEJUNK : CT_CHAR) )  ) 
    bool altered = false;

    int lastOutNonspaceCT = CT_CHAR;

    for( size_t i = 0; i< in_length; ++i ) {
        const char* inStr = in.c_str()+i;
        char prevC = ( i>0 ? in[i-1] : 0 );
        char c1 = ( i< in_length_1 ? in[i+1] : 0 );
        char c = in[i];
        char c_1 = ( i>0 ? in[i-1] : 0 );

        int ct_prev = GET_CT(prevC); 
        int ct_next = GET_CT(c1);
        int ct = GET_CT(c);
        
        if( c == '&' ) {
            if( c1 == '#' ) {
                const char* tmp = in.c_str()+i+2, *tmp_end = in.c_str()+in_length; 
                size_t tmpI = i+2;
                for( ; tmp < tmp_end && *tmp>='0' && *tmp<='9'; ++tmp ) 
                    ++tmpI;
             
                if( tmp < tmp_end && *tmp == ';' ) {
                    i= tmpI;
                    if( out.empty() || *(out.rbegin()) != ' ' ) 
                        out.push_back( ' ' );
                    if( !altered )
                        altered = true;
                    continue;
                }
            }
        } else if( c =='a' && ( !c_1||isspace(c_1)) ) { // normalizing AND to & 
            if( !uni || !uni->checkBit( UBIT_BENI_NORM_KEEPANDS ) ) {
                if( c1 == 'n' && inStr[2] == 'd' && (isspace(inStr[3]) || !inStr[3]) ) { 
                    i+=2;
                    continue;
                }
            }
        }

        if( ct == CT_2BYTEJUNK ) { // absorbing "funny chars"
            if( i< in_length-1 ) ++i;
            ct= CT_PUNCTSPACE;
        }
        if( ct == CT_PUNCTSPACE ) {
            if( (!out.empty() && *(out.rbegin()) != ' ') )
                out.push_back( ' ' );
            else if( !altered )
                altered = true;
        } else {
            if( lastOutNonspaceCT != ct ) {
                if( !out.empty() && *(out.rbegin()) != ' ' ) 
                    out.push_back(' ' );
                out.push_back( c );
            } else
                out.push_back( c );
            lastOutNonspaceCT = ct;
        }
    }
    return altered;
}
bool BENI::normalize_old( std::string& out, const std::string& in, const StoredUniverse* uni )
{
    out.clear();
    out.reserve( in.length() );

    size_t in_length = in.length(), in_length_1 = ( in_length ? in_length -1 : 0 );
    
    enum {
        CT_CHAR, //  everything that's not a digit or pucntspace is considered a char 
        CT_DIGIT , // isdigit
        CT_PUNCTSPACE, //ispunct or isspace
        CT_2BYTEJUNK // ® and such stripping off this junk
    };
    #define GET_CT(c) ( (!c || ispunct(c)||isspace(c))? CT_PUNCTSPACE: (isdigit(c)? CT_DIGIT: \
        ((uint8_t)(c) ==0xc2 ? CT_2BYTEJUNK : CT_CHAR) )  ) 
    bool altered = false;

    int lastOutNonspaceCT = CT_CHAR;

    for( size_t i = 0; i< in_length; ++i ) {
        char prevC = ( i>0 ? in[i-1] : 0 );
        char c1 = ( i< in_length_1 ? in[i+1] : 0 );
        char c = in[i];

        int ct_prev = GET_CT(prevC); 
        int ct_next = GET_CT(c1);
        int ct = GET_CT(c);
        
        if( c == '&' && c1 == '#' ) {
            const char* tmp = in.c_str()+i+2, *tmp_end = in.c_str()+in_length; 
            size_t tmpI = i+2;
            for( ; tmp < tmp_end && *tmp>='0' && *tmp<='9'; ++tmp ) 
                ++tmpI;
            
            if( tmp < tmp_end && *tmp == ';' ) {
                i= tmpI;
                if( out.empty() || *(out.rbegin()) != ' ' ) 
                    out.push_back( ' ' );
                if( !altered )
                    altered = true;
                continue;
            }
        }

        if( ct == CT_2BYTEJUNK ) { // absorbing "funny chars"
            if( i< in_length-1 ) ++i;
            ct= CT_PUNCTSPACE;
        }
        if( ct == CT_PUNCTSPACE ) {
            if( (out.empty() || *(out.rbegin()) != ' ') )
                out.push_back( ' ' );
            else if( !altered )
                altered = true;
        } else {
            if( lastOutNonspaceCT != ct && (!out.empty() && *(out.rbegin()) == ' ') ) {
                *(out.rbegin()) = c;
            } else
                out.push_back( c );
            lastOutNonspaceCT = ct;
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

        // here entity name length is irrelevant as is the weight - this is zurch

        out.push_back( {i.first, 1, i.second, 0, 0 } ); 
    }
    std::sort( out.begin(), out.end(), 
        []( const BENIFindResult& l, const BENIFindResult& r ) 
            { return (l.coverage> r.coverage?  true:(r.coverage>l.coverage ? false: (l.popRank>r.popRank ? true: l.nameLen<r.nameLen))  ); } 
    );
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
