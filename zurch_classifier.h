#pragma once

namespace zurch {

class DataSetFeaturePool {
};

/// features for single document
class StandardizedMoment {
    double mean;
    double variance;
    double stdDev;

    double kurtosis;
    double skewness;
}; 

} // namespace zurch 
