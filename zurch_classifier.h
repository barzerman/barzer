#pragma once

#include <ay_math.h>
#include <ay_statistics.h>
#include <ay_string_pool.h>
#include <limits>
#include <zurch_tokenizer.h>

namespace zurch {

class FeatureStatsAccumulator;
typedef ay::double_standardized_moments_weighted FeatureAccumulatedStats;

struct FeatureStats {
    double mean, stdDev; //  
    bool ignore;

    FeatureStats() : mean(0), stdDev(0), ignore(false) {}
    void zero() { mean = stdDev= 0.0; }
    
    bool equals( const FeatureStats& o, double epsilon ) const
        { return ( ay::epsilon_equals(mean,o.mean,epsilon)&&ay::epsilon_equals(stdDev,o.stdDev,epsilon) ); }

    inline void init( const FeatureAccumulatedStats& o )
    {
        double x = boost::accumulators::mean(o);
        if( !boost::math::isnan(x) ) 
            mean = x;
        x = boost::accumulators::variance(o);
        if( !boost::math::isnan(x) && x> 0 ) 
            stdDev = sqrt(x);
    }

    void setIgnore( bool v = true ) { ignore=v; }
    static double distance( const std::vector<double>& doc, const std::vector<FeatureStats>& centroid );
};

double FeatureStats::distance( const std::vector<double>& doc, const std::vector<FeatureStats>& centroid )
{
    if( doc.size() != centroid.size() ) 
        return std::numeric_limits<double>::quiet_NaN();

    std::vector<FeatureStats>::const_iterator ci = centroid.begin();
    std::vector<double>::const_iterator di = doc.begin();
    const double stdDevEpsilon = 1.0/centroid.size();
        double cumulative = 0.0;
    for( std::vector<FeatureStats>::const_iterator centroid_end = centroid.end(); ci != centroid_end; ++ci, ++di ) {
        if( !ci->ignore ) {
            double x = *di -ci->mean;
            cumulative += x*x/(stdDevEpsilon+ci->stdDev);
        }
    }
    return sqrt(cumulative);
}

/// 
/// features are assumed to correspond to numbers 0 - getNumberOfFeatures()
class FeatureStatsAccumulator {
    std::vector< FeatureAccumulatedStats > d_feature;
public:
    std::vector< FeatureAccumulatedStats >& vec() { return d_feature; }
    const std::vector< FeatureAccumulatedStats >& vec() const { return d_feature; }
    void setNumberOfFeatures( size_t n ) { d_feature.resize(n); }
    size_t getNumberOfFeatures( ) const { return d_feature.size(); }

    // adds data point to a feature accumulator
    void operator()( size_t idx, double val, double w=1.0 )
    {
        if( idx < d_feature.size() ) 
            d_feature[ idx ]( val, boost::accumulators::weight=w);
    }
    void zero() 
    {
        for( auto i = d_feature.begin(); i!= d_feature.end(); ++i ) 
            *i = FeatureAccumulatedStats();
    }
    double variance( size_t idx ) const 
        { return ( boost::accumulators::variance(d_feature[ idx ]) ); }
    size_t count( size_t idx ) const 
        { return ( boost::accumulators::count(d_feature[ idx ]) ); }
    double skewness( size_t idx ) const 
        { return ( boost::accumulators::skewness(d_feature[ idx ]) ); }
    double kurtosis( size_t idx ) const 
        { return ( boost::accumulators::kurtosis(d_feature[ idx ]) ); }
    double mean( size_t idx ) const 
        { return ( boost::accumulators::mean(d_feature[ idx ]) ); }
    double sum( size_t idx ) const 
        { return ( boost::accumulators::sum(d_feature[ idx ]) ); }

    double variance_safe( size_t idx ) const 
        { return ( idx < d_feature.size() ? variance(idx) : 0 ); }
    double count_safe( size_t idx ) const 
        { return ( idx < d_feature.size() ? count(idx) : 0 ); }
    double skewness_safe( size_t idx ) const 
        { return ( idx < d_feature.size() ? skewness(idx) : 0 ); }
    double kurtosis_safe( size_t idx ) const 
        { return ( idx < d_feature.size() ? kurtosis(idx) : 0 ); }
    double mean_safe( size_t idx ) const 
        { return ( idx < d_feature.size() ? mean(idx) : 0 ); }
    double sum_safe( size_t idx ) const 
        { return ( idx < d_feature.size() ? sum(idx) : 0 ); }
    
    void getFeatureStatsVec( std::vector< FeatureStats> && vec )
    {
        vec.clear();
        if( !d_feature.size() )
            return;
        vec.resize( d_feature.size() );
        std::vector< FeatureStats>::iterator vi = vec.begin(); 
        for( auto i = d_feature.begin(); i != d_feature.end(); ++i, ++vi ) 
            vi->init(*i);
    }
};

struct TagStatsAccumulator;
/// results of accumulation, needed for classification proper. can be initialized from TagStatsAccumulator or another source (stored on disk)
struct TagStats {
    std::vector< FeatureStats > hasVec, hasNotVec;
    size_t  hasCount, hasNotCount;  /// number of docs with and without the tag
    
    TagStats(): hasCount(0), hasNotCount(0) {}

    void init( const TagStatsAccumulator& x ) ; 

    TagStats( const TagStatsAccumulator& x ) { init(x); }
    
    bool checkIntegrity() const { return hasVec.size() == hasNotVec.size(); }
    void setIgnore( double epsilon) 
    {
        for( size_t i = 0; i< hasVec.size(); ++i ) {
            FeatureStats& y = hasVec[i];
            FeatureStats& n = hasNotVec[i];
            if( y.equals( n, epsilon ) ) {
                y.setIgnore();
                n.setIgnore();
            }
        }
    }
    void zero() 
    {
        for( auto i = hasVec.begin();i!= hasVec.end(); ++i ) 
            i->zero();
        for( auto i = hasNotVec.begin();i!= hasNotVec.end(); ++i ) 
            i->zero();
    }

    double yesDistance( const std::vector<double>& doc ) const
        { return FeatureStats::distance( doc, hasVec ); }
    double noDistance( const std::vector<double>& doc ) const
        { return FeatureStats::distance( doc, hasNotVec ); }

    double getYesProbability(const std::vector<double>& doc) const
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
/// for each tag accumulates YES and NO feature stats in two separate accumulators
struct TagStatsAccumulator {
    FeatureStatsAccumulator has, hasNot;
    size_t  hasCount, hasNotCount; 
    
    TagStatsAccumulator(): hasCount(0), hasNotCount(0) {}
    void zero() {
        has.zero();
        hasNot.zero();
    }
    // idx, double val, double w=1.0
    void accumulate( std::vector<double>& f, bool hasTag, double docWeight=1.0 )
    {
        for( auto i = f.begin(); i!= f.end(); ++i ) {
            size_t idx = (i-f.begin());
            if( hasTag ) {
                has( idx, *i, docWeight );
                ++hasCount;
            } else {
                hasNot( idx, *i, docWeight );
                ++hasNotCount;
            }
        }
    }
};
inline void TagStats::init( const TagStatsAccumulator& x ) 
{
    if( x.has.getNumberOfFeatures() != x.hasNot.getNumberOfFeatures() ) {
        std::cerr << "PANIC: TagStats::init has/hasNot mismatch\n";
        return;
    }
    hasVec.clear();
    hasVec.resize( x.has.getNumberOfFeatures() ); 
    hasNotVec.clear();
    hasNotVec.resize( x.hasNot.getNumberOfFeatures() );

    size_t j = 0;
    for( const auto& i : x.has.vec() ) 
        hasVec[ j++ ].init( i );

    j = 0;
    for( const auto& i : x.hasNot.vec() ) 
        hasNotVec[ j++ ].init( i );

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
    int extractFeatures( std::vector<double>& features, const char* buf, bool learnMode=true );
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
    
    void accumulate( size_t tagId, std::vector<double>& f, bool hasTag, double docWeight=1.0 )
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

    int addDoc( std::vector<double>& features, const char* doc, const std::set< size_t > hasTags, double docWeight = 1.0 )
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
    
    void extractFeatures( std::vector<double>& features, const char* doc )
    {
        d_featureExtractor.extractFeatures( features, doc, true );
    }

    double classify( size_t tagId, const std::vector<double>& features ) const
    {
        if(tagId< getNumOfTags()) {
            double yesProb= d_stats.tagStats[ tagId ].getYesProbability(features);
            return yesProb;
        }
        return 0.0;
    }
    void classifyAll( std::vector<double>& tagYesProb, const std::vector<double>& features ) const
    {
        tagYesProb.resize( d_stats.getNumOfTags() );
        for( size_t i = 0, i_end = tagYesProb.size(); i< i_end; ++i ) 
            tagYesProb[i] = classify(i,features);
    }
};

} // namespace zurch 
