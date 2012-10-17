#pragma once

#include <math.h>
#include <ay_statistics.h>
#include <ay_string_pool.h>

namespace zurch {

struct FeatureStats {
    double mean, stdDev; //  
    bool   isBlank;
    FeatureStats() : mean(0), stdDev(0), isBlank(true) {}
};

typedef std::vector<FeatureStats> FeatureStatsVec;
/// 
/// features are assumed to correspond to numbers 0 - getNumberOfFeatures()
class FeatureStatsAccumulator {
    std::vector< ay::double_standardized_moments_weighted > d_feature;
public:
    void setNumberOfFeatures( size_t n ) { d_feature.resize(n); }
    size_t getNumberOfFeatures( ) const { return d_feature.size(); }

    // adds data point to a feature accumulator
    void operator()( size_t idx, double val, double w=1.0 )
    {
        if( idx < d_feature.size() ) 
            d_feature[ idx ]( val, boost::accumulators::weight=weight);
    }
    void zero() 
    {
        for( auto i = d_feature.begin(); i!= d_feature.end(); ++i ) {
            i->clear();
        }
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
    
    void getFeatureStatsVec( FeatureStatsVec& vec )
    {
        if( !d_feature.size() )
            return;
        vec.reserve( d_feature.size() );
        vec.clear();
        vec.resize( d_feature.size() );
        FeatureStatsVec::iterator vi = vec.begin(); 
        for( auto i = d_feature.begin(); i != d_feature.end(); ++i, ++vi ) {
            double x = boost::accumulators::mean(*i);
            if( !boost::math::isnan(x) ) {
                vi->mean = x;
            }
                
            x = boost::accumulators::variance(*i);
            if( !boost::math::isnan(x) && x> 0 ) 
                vi->stdDev = sqrt(x);
        }
    }
    static double distance( const std::vector<double>& doc, const FeatureStatsVec& centroid )
    {
        if( doc.size() != centroid.size() ) 
            return 
        FeatureStatsVec::const_iterator ci = centroid.begin();
        const std::vector<double>::const_iterator di = doc.begin();
        const double stdDevEpsilon = 1.0/centroid.size();
        double cumulative = 0.0;
        for( auto centroid_end = centroid.end(); ci != centroid_end; ++ci, ++di ) {
            double x = *di-ci->mean;
            cumulative += x*x/(stdDevEpsilon+ci->stdDev);
        }
        return sqrt(cumulative);
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
/// results of accumulation, needed for classification proper. can be initialized from TagStatsAccumulator or another source (stored on disk)
struct TagStats {
    FeatureStatsVec has, hasNot;
    size_t  hasCount, hasNotCount; 
    
    TagStats(): hasCount(0), hasNotCount(0) {}

    TagStats( const TagStatsAccumulator& x ) :
        hasCount(x.hasCount),
        hasNotCount(x.hasNotCount)
    {
        x.has.getFeatureStatsVec(has);
        x.hasNot.getFeatureStatsVec(hasNot);
    }
};

/// accumulates stats for all tags for all docs 
struct DocStatsAccumulator {
    ay::UniqueCharPool&  d_tokPool;
    ZurchTokenizer&      d_tokenizer;
    
    std::vector< TagStats > d_tagStats;

    /// for each tag this type of accumulator will be 
    typedef std::pair< FeatureStatsAccumulator, FeatureStatsAccumulator > TagStatsAccumulator;
    enum {
        FM_TOKEN, // features are tokens
        FM_BARZER // features are barzer types 
    };

    /// zero out the stats
    void zero() 
    {
    }
    void setNumOfTags( size_t n ) { d_tagStats.resize(); }
};

class DocumentStatsAccumulator {
    ay::UniqueCharPool&  d_tokPool;
    ZurchTokenizer&      d_tokenizer;
public: 
    /// for each tag this type of accumulator will be 
    typedef std::pair< FeatureStatsAccumulator, FeatureStatsAccumulator > TagStatsAccumulator;
    enum {
        FM_TOKEN, // features are tokens
        FM_BARZER // features are barzer types 
    };
private:
    TagStatsAccumulator  
    int d_featureMode;
public:
    DocumentStatsAccumulator() : d_featureMode(FM_TOKEN) {}
    /// features are tokens
    int classifyDoc_FeToken( const char* buf );
    /// features are barzer types
    int classifyDoc_Barzer( const char* buf );
    
    int classifyDoc( const char* doc )
    {
        if( d_featureMode == FM_TOKEN )
            return classifyDoc_FeToken(doc);
        else
            return classifyDoc_Barzer(doc);
    }
};


/// corresponds to single tag classification. Every tag corresponds to 2 classes 
struct SingleTagClassifier {
protected:
    ay::UniqueCharPool&  d_tokPool;
    ZurchTokenizer&      d_tokenizer;

    /// this can be initialized from saved state or actually populated from d_statsAcc
    FeatureStatsVec     d_featureStats; // result of accumulation

    FeatureStatsAccumulator d_statsAcc;
public:
    /// features are tokens
    int classifyDoc_FeToken( const char* buf );
    /// features are barzer types
    int classifyDoc_FeToken( const char* buf );

    /// adding documents (training) 
    /// features are tokens
    int addDoc_FeToken( const char* buf );
    /// features are barzer types
    int addDoc_FeToken( const char* buf );
};

} // namespace zurch 
