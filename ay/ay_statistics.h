
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#pragma once

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/covariance.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/variance.hpp>
#include <boost/accumulators/statistics/kurtosis.hpp>
#include <boost/accumulators/statistics/skewness.hpp>
#include <boost/accumulators/statistics/variates/covariate.hpp>

#include <boost/accumulators/statistics/weighted_variance.hpp>
#include <boost/accumulators/statistics/weighted_covariance.hpp>
#include <boost/accumulators/statistics/weighted_density.hpp>
#include <boost/accumulators/statistics/weighted_extended_p_square.hpp>
#include <boost/accumulators/statistics/weighted_kurtosis.hpp>
#include <boost/accumulators/statistics/weighted_mean.hpp>
#include <boost/accumulators/statistics/weighted_median.hpp>
#include <boost/accumulators/statistics/weighted_moment.hpp>

#include <boost/version.hpp>
#if BOOST_VERSION >= 105000
#include <boost/accumulators/statistics/weighted_p_square_cumul_dist.hpp>
#else
#include <boost/accumulators/statistics/weighted_p_square_cumulative_distribution.hpp>
#endif

#include <boost/accumulators/statistics/weighted_p_square_quantile.hpp>
#include <boost/accumulators/statistics/weighted_peaks_over_threshold.hpp>
#include <boost/accumulators/statistics/weighted_skewness.hpp>
#include <boost/accumulators/statistics/weighted_sum.hpp>

#include <boost/math/special_functions/fpclassify.hpp>

namespace ay {
//// wrapper of boost accumulators

typedef boost::accumulators::accumulator_set<
    double, 
    boost::accumulators::stats<
        boost::accumulators::tag::weighted_mean,
        boost::accumulators::tag::weighted_skewness,
        boost::accumulators::tag::weighted_variance,
        boost::accumulators::tag::weighted_kurtosis
    >,
    double
> double_standardized_moments_weighted;

typedef boost::accumulators::accumulator_set<
    double, 
    boost::accumulators::stats<
        boost::accumulators::tag::mean,
        boost::accumulators::tag::skewness,
        boost::accumulators::tag::variance,
        boost::accumulators::tag::kurtosis
    >
> double_standardized_moments;

}
