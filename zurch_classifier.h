
#include <ay_math.h>
#include <ay_statistics.h>
#include <ay_string_pool.h>
#include <limits>
#include <zurch_tokenizer.h>

namespace zurch {


class FeatureStatsAccumulator;
typedef ay::double_standardized_moments_weighted FeatureAccumulatedStats;

/// first is feature id 

typedef std::map<size_t,double> ExtractedFeatureMap;

struct FeatureStats {
    size_t featureId; // unique feature id 
    double mean, stdDev; // 
    bool ignore;  // unused for now 

    FeatureStats() : mean(0), stdDev(0), ignore(false), featureId(0xffffffff) {}
    void zero() { mean = stdDev= 0.0; }
    
    bool equals( const FeatureStats& o, double epsilon ) const
        { return ( ay::epsilon_equals(mean,o.mean,epsilon)&&ay::epsilon_equals(stdDev,o.stdDev,epsilon) ); }

    inline void init( size_t fid, const FeatureAccumulatedStats& o )
    {
        featureId = fid;
        double x = boost::accumulators::mean(o);
        if( !boost::math::isnan(x) ) 
            mean = x;
        x = boost::accumulators::variance(o);
        if( !boost::math::isnan(x) && x> 0 ) 
            stdDev = sqrt(x);
    }

    void setIgnore( bool v = true ) { ignore=v; }
};

struct TagStatsAccumulator;
/// results of accumulation, needed for classification proper. can be initialized from TagStatsAccumulator or another source (stored on disk)
struct TagStats {
    typedef std::map< size_t , FeatureStats > Map;

    Map hasMap, hasNotMap;

    size_t  hasCount, hasNotCount;  /// number of docs with and without the tag
    
    TagStats(): hasCount(0), hasNotCount(0) {}

    void init( const TagStatsAccumulator& x ) ; 

    TagStats( const TagStatsAccumulator& x ) { init(x); }
    
    void setIgnore( double epsilon) 
    {
        Map newHas, newHasNot;

        for( Map::iterator i = hasMap.begin(); i!=  hasMap.end(); ++i ) {
            Map::iterator not_i = hasNotMap.find(i->first);
            if( not_i != hasNotMap.end() ) {
                if( !i->second.equals( not_i->second, epsilon ) ) 
                    continue;
                newHasNot.insert( newHasNot.end(), *i );
            }
            newHas.insert( newHas.end(), *i );
        }
        hasMap.swap(newHas);
        hasNotMap.swap(newHasNot);
    }
    void zero() 
    {
        for( auto i = hasMap.begin();i!= hasMap.end(); ++i ) 
            i->second.zero();
        for( auto i = hasNotMap.begin();i!= hasNotMap.end(); ++i ) 
            i->second.zero();
    }

    static double distance( const ExtractedFeatureMap& doc, const Map& centroid )
    {
        const double stdDevEpsilon = 1.0/centroid.size();
            double cumulative = 0.0;
        for( TagStats::Map::const_iterator ci = centroid.begin(), centroid_end = centroid.end(); ci != centroid_end; ++ci ) {
            if( !ci->second.ignore ) {
                ExtractedFeatureMap::const_iterator di = doc.find(ci->second.featureId);

                double x = ( di == doc.end() ? ci->second.mean : (di->second - ci->second.mean) );
                cumulative += (x*x)/(stdDevEpsilon+ci->second.stdDev);
            }
        }
        return sqrt(cumulative);
    }
    double yesDistance( const ExtractedFeatureMap& doc ) const
        { return distance( doc, hasMap ); }
    double noDistance( const ExtractedFeatureMap& doc ) const
        { return distance( doc, hasNotMap ); }

    double getYesProbability(const ExtractedFeatureMap& doc) const
    {
        double yesDist = yesDistance(doc);
        double noDist = noDistance(doc);
        double sumDist = noDist+yesDist; 
        if( !sumDist )
            return .5;
        else 
            return noDist/sumDist;
    }
};
/// 
/// features are assumed to correspond to numbers 0 - getNumberOfFeatures()

class FeatureStatsAccumulator {
    /// key is featureId 
public: 
    typedef std::map< size_t, FeatureAccumulatedStats > Map;
private:
    Map d_feature;
public:
    Map& featureMap() { return d_feature; }
    const Map& featureMap() const { return d_feature; }
    size_t getNumberOfFeatures( ) const { return d_feature.size(); }

    // adds data point to a feature accumulator
    void operator()( size_t idx, double val, double w=1.0 )
    {
        Map::iterator i = d_feature.find(idx);
        if( i == d_feature.end() ) 
            i = d_feature.insert( std::make_pair(idx,FeatureAccumulatedStats()) ).first;

        i->second( val, boost::accumulators::weight=w);
    }
    void zero() 
    {
        for( Map::iterator i = d_feature.begin(); i!= d_feature.end(); ++i ) 
            i->second = FeatureAccumulatedStats();
    }
    
    #define DECL_GET(ACC) double ACC(size_t idx) { \
        Map::const_iterator i = d_feature.find(idx);\
        return ( i==d_feature.end() ? 0 : boost::accumulators::ACC( i->second ) );\
        }
    DECL_GET(variance)
    DECL_GET(count)
    DECL_GET(skewness)
    DECL_GET(kurtosis)
    DECL_GET(sum)
    #undef DECL_GET 

    void getCentroid( TagStats::Map& centroid )
    {
        centroid.clear();
        for( Map::const_iterator i = d_feature.begin(); i != d_feature.end(); ++i ){

            FeatureStats st;
            st.init(i->first,i->second);
            centroid.insert( std::make_pair( i->first, st ) );
        }
    }
};

/// for each tag accumulates YES and NO feature stats in two separate accumulators
struct TagStatsAccumulator {
    FeatureStatsAccumulator has, hasNot;
    std::set< size_t > featureIdMap;

    size_t  hasCount, hasNotCount; 
    
    TagStatsAccumulator(): hasCount(0), hasNotCount(0) {}
    void zero() {
        has.zero();
        hasNot.zero();
    }
    // idx, double val, double w=1.0
    void accumulate( const ExtractedFeatureMap& f, bool hasTag, double docWeight=1.0 )
    {
        for( auto i = f.begin(); i!= f.end(); ++i ) {
            size_t idx = i->first;
            featureIdMap.insert( idx );
            if( hasTag ) {
                has( idx, i->second, docWeight );
                ++hasCount;
            } else {
                hasNot( idx, i->second, docWeight );
                ++hasNotCount;
            }
        }
    }
};
inline void TagStats::init( const TagStatsAccumulator& x ) 
{
    hasMap.clear();
    hasNotMap.clear();

    for( const auto& i : x.has.featureMap() ) {
        FeatureStats fs;
        fs.init( i.first, i.second);
        hasMap.insert( std::make_pair<size_t,FeatureStats>(i.first, fs) );
    }
    for( const auto& i : x.hasNot.featureMap() ) {
        FeatureStats fs;
        fs.init( i.first, i.second);
        hasNotMap.insert( std::make_pair<size_t,FeatureStats>(i.first, fs) );
    }
    hasCount = x.hasCount;
    hasNotCount = x.hasNotCount;
}


/// basic token / normalization based feature extractor
class FeatureExtractor {
    ay::UniqueCharPool&     d_tokPool;
    ZurchTokenizer&         d_tokenizer;
public:
    FeatureExtractor( ay::UniqueCharPool&   tokPool, ZurchTokenizer& tokenizer ):
        d_tokPool( tokPool), 
        d_tokenizer(tokenizer)
    {
    }
    size_t getNumFeatures() const { return d_tokPool.getMaxId(); }
    /// if learnmode = true will add tokens to the token pool
    int extractFeatures( ExtractedFeatureMap& features, const char* buf, bool learnMode=true );
};
/// accumulates stats for all tags for all docs 
struct DocSetStatsAccumulator {
    FeatureExtractor& d_featureExtractor;

    /// for each tag this type of accumulator will be 
    std::vector< TagStatsAccumulator > d_tagStats;

    DocSetStatsAccumulator( FeatureExtractor& fe ) : d_featureExtractor(fe) {}

    /// zero out the stats
    void zero() { for( auto& i : d_tagStats ) i.zero(); }

    void setNumOfTags( size_t n ) { d_tagStats.resize(n); }
    size_t getNumOfTags() const { return d_tagStats.size(); }
    

    void accumulate( size_t tagId, const ExtractedFeatureMap& f, bool hasTag, double docWeight=1.0 )
    {
        if( tagId< d_tagStats.size() ) 
            d_tagStats[tagId].accumulate(f,hasTag,docWeight);
    }
    const std::vector< TagStatsAccumulator >& getAccumulatedStats() const { return d_tagStats; }
};

struct DocSetStats {
    std::vector< TagStats > tagStats;
    
    void zero() 
        { for( auto i = tagStats.begin(); i!= tagStats.end(); ++i ) i->zero(); }

    void init( const DocSetStatsAccumulator& acc )
    {
        tagStats.clear();

        const std::vector< TagStatsAccumulator >& accVec = acc.getAccumulatedStats();
        if( !accVec.size() )
            return;
        tagStats.resize( accVec.size() );
        for( auto i = accVec.begin(), i_end = accVec.end();  i != i_end; ++i ) 
            tagStats[ (i-accVec.begin()) ].init( *i );
    }
    size_t getNumOfTags() const { return tagStats.size(); }
};


/// ACHTUNG!! we may wanna templatize on FeatureExtractor
class DataSetTrainer {
    FeatureExtractor& d_featureExtractor; 
    DocSetStatsAccumulator& d_acc;
public: 
    DataSetTrainer( FeatureExtractor& featureExtractor, DocSetStatsAccumulator& acc ) : 
        d_featureExtractor(featureExtractor),
        d_acc(acc)
    {}

    void init( size_t numOfTags ) 
    {
        d_acc.zero();
        d_acc.setNumOfTags(numOfTags);
    }

    int addDoc( ExtractedFeatureMap& features, const char* doc, const std::set< size_t > hasTags, double docWeight = 1.0 )
    {
        d_featureExtractor.extractFeatures( features, doc, true );
        for( size_t i = 0, i_end = hasTags.size(); i< i_end; ++i ) {
            bool gotTag = (hasTags.find(i) != hasTags.end() );
            d_acc.accumulate( i, features, gotTag, docWeight);
        }
        return 0;
    }
};
/// use this object after initializing DocSetStats (as a result of training or otherwise)
struct  DocClassifier {
    DocSetStats d_stats;
    FeatureExtractor& d_featureExtractor;
public:
    DocClassifier( FeatureExtractor& featureExtractor ) : d_featureExtractor(featureExtractor) {}

    DocSetStats& stats() { return d_stats; }
    const DocSetStats& stats() const { return d_stats; }


    size_t getNumOfTags() const { return d_stats.getNumOfTags(); }
    
    void extractFeatures( ExtractedFeatureMap& features, const char* doc )
    {
        d_featureExtractor.extractFeatures( features, doc, true );
    }

    double classify( size_t tagId, const ExtractedFeatureMap& features ) const
    {
        if(tagId< getNumOfTags()) {
            double yesProb= d_stats.tagStats[ tagId ].getYesProbability(features);
            return yesProb;
        }
        return 0.0;
    }
    void classifyAll( std::vector<double>& tagYesProb, const ExtractedFeatureMap& features ) const
    {
        tagYesProb.resize( d_stats.getNumOfTags() );
        for( size_t i = 0, i_end = tagYesProb.size(); i< i_end; ++i ) 
            tagYesProb[i] = classify(i,features);
    }
};

} // namespace zurch 
