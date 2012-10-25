#pragma once

namespace barzer {

typedef std::vector< uint32_t > FeatureStrIdVec;

struct ExtractedStringFeature {
    const char* str;
    uint16_t    len;
    uint16_t    offset;

    ExtractedStringFeature() : str(0), len(0),offset(0) {}
};
/// we assume that any kind of feature is possible to epres with 3 values 
/// for example - uint32_t for substring id , and two other 16bit numbers for position and length
/// or if the feature is a double we can store something else in these 8 bytes 
struct StoredStringFeature {
    uint32_t strId;
    uint16_t u41;  
    uint16_t u42;
};

typedef std::vector<ExtractedStringFeature> ExtractedStringFeatureVec;
typedef std::vector<StoredStringFeature>    StoredStringFeatureVec;

/// feature extractor classes
/// every class must have the following operator defined:
/// void operator()( StringFeatureVec&, const char* str, size_t str_len, int lang );

struct TFE_ngram {
    void operator()( ExtractedStringFeatureVec&, const char* str, size_t str_len, int lang );
};
/// end of feature extractor classes

typedef boost::unordered_map< StoredStringFeature, std::vector<uint32_t> > InvertedFeatureMap;

struct TFE_TmpBuffers {
    StoredStringFeatureVec& storedVec;
    ExtractedStringFeatureVec& extractedVec;
    TFE_TmpBuffers( StoredStringFeatureVec&  sv, ExtractedStringFeatureVec& ev ) : 
        storedVec(sv), extractedVec(ev) {}
};
//// stores inverted index for a specific feature extractor 
template <typename FE>
struct TFE_storage {
    InvertedFeatureMap d_fm;
    ay::UniqueCharPool& d_pool;
    FE d_extractor;

    typedef boost::unordered_set< uint32_t > StringSet;
    StringSet d_strSet; 

    TFE_storage( ay::UniqueCharPool&  p ) : d_pool(p) {}

    /// extracts all features and adds entries to d_fm strId is id of str - we're passing str 
    void extractAndStore( TFE_TmpBuffers& tmp, uint32_t strId, const char* str, int lang )
    {
        StoredStringFeatureVec x;
        ExtractedStringFeatureVec y;

        // void operator()( ExtractedStringFeatureVec&, const char* str, size_t str_len, int lang );
    }
    void extractFeatures( ExtractedStringFeatureVec&, const char* str, int lang ) const
    {
        
    }
    
    /// performs simple lookup
    void featureConvert( StoredStringFeature&, const ExtractedStringFeature& ) const {}
    void featureConvert( StoredStringFeature&, const ExtractedStringFeature& ) {}
    
    FE& extractor() { return d_extractor; }
    const FE& extractor() const { return d_extractor; }
};

typedef boost::variant<
    TFE_storage<TFE_ngram>,
> InvertedTokenIndex;

} // namespace barzer
