#pragma once

#include <math.h>
#include <ay_statistics.h>

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

} // namespace zurch 
