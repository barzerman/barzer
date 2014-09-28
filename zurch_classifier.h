
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#pragma once
#include <ay_math.h>
#include <ay_statistics.h>
#include <ay_string_pool.h>
#include <limits>
#include <zurch_tokenizer.h>

namespace barzer { class BZSpell; }

namespace zurch {


class FeatureStatsAccumulator;
typedef ay::double_standardized_moments_weighted FeatureAccumulatedStats;

struct PrintContext {
    const ay::UniqueCharPool*     stringPool;
    
    const char* getString( uint32_t i ) const 
        { return ( stringPool? stringPool->printableStr( i ) : "" ); }

    std::ostream& printStringById( std::ostream& fp, uint32_t i ) const ;
    PrintContext( const ay::UniqueCharPool* s ) : stringPool(s) {}
};

/// first is feature id 

typedef std::map<uint32_t,double> ExtractedFeatureMap;


///// 
struct FeatureStats {
    size_t featureId; // unique feature id 
    double mean, stdDev; // 
    bool ignore;  // unused for now 

    FeatureStats() : featureId(0xffffffff), mean(0), stdDev(0), ignore(false){}
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
    std::ostream& print( std::ostream& fp) const ;
    std::ostream& print( std::ostream& fp, const PrintContext& ) const ;
};
inline std::ostream& operator<< ( std::ostream& fp, const FeatureStats& fs ) 
{
    return fs.print( fp );
}

struct TagStatsAccumulator;
/// results of accumulation, needed for classification proper. can be initialized from TagStatsAccumulator or another source (stored on disk)
struct TagStats {
    typedef std::map< size_t , FeatureStats > Map;

    Map hasMap, hasNotMap;

    size_t  hasCount, hasNotCount;  /// number of docs with and without the tag
    
    TagStats(): hasCount(0), hasNotCount(0) {}

    void init( const TagStatsAccumulator& x ) ; 

    TagStats( const TagStatsAccumulator& x ) { init(x); }
    
    std::ostream& print( std::ostream& fp ) const;
    std::ostream& printFeatureStatsMap( std::ostream& fp, const Map& m, const PrintContext& ctxt ) const ;
        
    std::ostream& printDeep( std::ostream& fp, const PrintContext& ctxt ) const;
    void setIgnore( double epsilon) ;
    void zero() ;

    static double distance( const ExtractedFeatureMap& doc, const Map& centroid );
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
    void accumulate( const ExtractedFeatureMap& f, bool hasTag, double docWeight=1.0 );
    std::ostream& print( std::ostream& fp, const PrintContext& ) const;
};

/// basic token / normalization based feature extractor
struct FeatureExtractor_Base {
    ay::UniqueCharPool&     d_tokPool;

    FeatureExtractor_Base(ay::UniqueCharPool& pool) : d_tokPool(pool) {}

    typedef std::vector< std::string > StringVec;
    virtual int extractFeatures( ExtractedFeatureMap& features, const FeatureExtractor_Base::StringVec& ntVec, bool learnMode=true )=0;
    virtual int extractFeatures( ExtractedFeatureMap& features, const char* buf, bool learnMode=true ) = 0;

    size_t getNumTokensInPool()const { return d_tokPool.getMaxId(); }
    virtual size_t getNumFeatures() const { return d_tokPool.getMaxId(); }
    void normalizeFeatures( ExtractedFeatureMap& features, double normalizeBy ) {
        for( ExtractedFeatureMap::iterator i = features.begin(); i!= features.end(); ++i ) 
            i->second/= normalizeBy;
    }
    virtual ~FeatureExtractor_Base() {}
};

/// most basic feature extractor (no normalization)
class FeatureExtractor : public FeatureExtractor_Base {
protected:
    ZurchTokenizer&         d_tokenizer;
public:
    FeatureExtractor( ay::UniqueCharPool&   tokPool, ZurchTokenizer& tokenizer ):
        FeatureExtractor_Base(tokPool),
        d_tokenizer(tokenizer)
        {}
    /// if learnmode = true will add tokens to the token pool
    int extractFeatures( ExtractedFeatureMap& features, const FeatureExtractor_Base::StringVec& ntVec, bool learnMode=true );
    int extractFeatures( ExtractedFeatureMap& features, const char* buf, bool learnMode=true );
};
/// normalizing extarctor - in addition to tokenizing the document also 
/// normalizes the tokens 
class FeatureExtractor_Normalizing : public FeatureExtractor_Base {
protected:
    ZurchTokenizer&      d_tokenizer;
    ZurchWordNormalizer d_normalizer;
public:
    FeatureExtractor_Normalizing( ay::UniqueCharPool& pool, ZurchTokenizer& tokenizer, const barzer::BZSpell* spell ):
        FeatureExtractor_Base(pool),
        d_tokenizer(tokenizer),
        d_normalizer(spell)
    {}
    virtual int extractFeatures( ExtractedFeatureMap& features, const FeatureExtractor_Base::StringVec& ntVec, bool learnMode=true );
    virtual int extractFeatures( ExtractedFeatureMap& features, const char* buf, bool learnMode=true );
};
/// accumulates stats for all tags for all docs 
struct DocSetStatsAccumulator {
    /// for each tag this type of accumulator will be 
    typedef  std::map< uint32_t, TagStatsAccumulator > TagStatsAccumulatorMap;
    TagStatsAccumulatorMap d_tagStats;

    /// zero out the stats
    void zero() { for( auto& i : d_tagStats ) i.second.zero(); }

    TagStatsAccumulator& obtainTagStatsAccumulator( uint32_t tagId ) 
    {
        TagStatsAccumulatorMap::iterator i = d_tagStats.find( tagId );
        if( i == d_tagStats.end() ) 
            i = d_tagStats.insert( TagStatsAccumulatorMap::value_type(tagId,TagStatsAccumulator()) ).first;
        return i->second;
    }
    void accumulate( size_t tagId, const ExtractedFeatureMap& f, bool hasTag, double docWeight=1.0 )
    {
        obtainTagStatsAccumulator(tagId).accumulate(f,hasTag,docWeight);
    }
    TagStatsAccumulatorMap& getAccumulatedStats() { return d_tagStats; }
    const TagStatsAccumulatorMap& getAccumulatedStats() const{ return d_tagStats; }

    std::ostream& printTagStatsAccumulation( std::ostream& fp, uint32_t tagId, const PrintContext& ctxt ) const;
    std::ostream& print( std::ostream& fp, const PrintContext& ctxt ) const;
};

struct DocSetStats {
    typedef std::map< uint32_t, TagStats > TagStatsMap;
    TagStatsMap tagStats;
    
    void zero() 
        { for( auto i = tagStats.begin(); i!= tagStats.end(); ++i ) i->second.zero(); }

    TagStatsMap& getTagStatsMap() { return tagStats; }
    const TagStatsMap& getTagStatsMap() const { return tagStats; }

    const TagStats* getTagStatsPtr( uint32_t tagId )  const
    {
        TagStatsMap::const_iterator i = tagStats.find(tagId);
        return( i == tagStats.end() ? 0 : &(i->second) );
    }
    TagStats* getTagStatsPtr( uint32_t tagId ) 
    {
        TagStatsMap::iterator i = tagStats.find(tagId);
        return( i == tagStats.end() ? 0 : &(i->second) );
    }

    TagStats& obtainTagStats( uint32_t tagId ) 
    {
        TagStatsMap::iterator i = tagStats.find(tagId);
        if( i== tagStats.end() )
            i= tagStats.insert( TagStatsMap::value_type( tagId, TagStats() ) ).first;
        return i->second;
    }
    void init( const DocSetStatsAccumulator& acc )
    {
        tagStats.clear();

        for( auto i = acc.getAccumulatedStats().begin(), i_end = acc.getAccumulatedStats().end();  i != i_end; ++i ) 
            obtainTagStats( i->first ).init( i->second );
    }
    size_t getNumOfTags() const { return tagStats.size(); }

    std::ostream& printDeep( std::ostream& fp, const PrintContext& ctxt ) const;
    std::ostream& print( std::ostream& fp ) const;
};


/// ACHTUNG!! we may wanna templatize on FeatureExtractor
class DataSetTrainer {
    FeatureExtractor_Base& d_featureExtractor; 
    DocSetStatsAccumulator& d_acc;
public: 
    DataSetTrainer( FeatureExtractor_Base& featureExtractor, DocSetStatsAccumulator& acc ) : 
        d_featureExtractor(featureExtractor),
        d_acc(acc)
    {}

    void init( size_t numOfTags ) 
    {
        d_acc.zero();
    }

    int addDoc( ExtractedFeatureMap& features, const char* doc, size_t tagId, bool hasIt, double docWeight = 1.0 )
    {
        d_featureExtractor.extractFeatures( features, doc, true );
        d_acc.accumulate( tagId, features, hasIt, docWeight);
        return 0;
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
    std::ostream& print( std::ostream& fp, const PrintContext& ctxt ) const;
};
/// use this object after initializing DocSetStats (as a result of training or otherwise)
struct  DocClassifier {
    DocSetStats d_stats;
    FeatureExtractor_Base& d_featureExtractor;
public:
    DocClassifier( FeatureExtractor_Base& featureExtractor ) : d_featureExtractor(featureExtractor) {}

    DocSetStats& stats() { return d_stats; }
    const DocSetStats& stats() const { return d_stats; }

    size_t getNumOfTags() const { return d_stats.getNumOfTags(); }
    
    void extractFeatures( ExtractedFeatureMap& features, const char* doc, bool learnMode  )
        { d_featureExtractor.extractFeatures( features, doc, true ); }

    double classify( size_t tagId, const ExtractedFeatureMap& features ) const
    {
        const TagStats* tstat = d_stats.getTagStatsPtr( tagId );
        if( tstat ) {
            double yesProb= tstat->getYesProbability(features);
            return yesProb;
        }
        return 0.0;
    }
    typedef std::map< uint32_t, double > TagIdToProbabilityMap;
    void classifyAll( TagIdToProbabilityMap& tagYesProb, const ExtractedFeatureMap& features ) const
    {
        for( DocSetStats::TagStatsMap::const_iterator i= d_stats.getTagStatsMap().begin(), i_end=d_stats.getTagStatsMap().end();i != i_end; ++i ) 
            tagYesProb.insert( TagIdToProbabilityMap::value_type( i->first,classify(i->first,features) ) );
    }

    std::ostream& print( std::ostream& fp, const PrintContext& ctxt )  const;
};

class ZurchTrainerAndClassifier {
    ay::UniqueCharPool& stringPool;
    
    ZurchTokenizer  tokenizer;
    ZurchWordNormalizer normalizer;

    FeatureExtractor_Base* extractor;
    DocClassifier* classifier;
    DocSetStatsAccumulator* accumulator;
    DataSetTrainer*         trainer;
    
    ZurchTrainerAndClassifier( const ZurchTrainerAndClassifier& o ): 
        stringPool(o.stringPool),
        extractor(o.extractor) {}
    void clear();
public:
    ZurchTokenizer& getTokenizer() { return tokenizer; }
    const ZurchTokenizer& getTokenizer() const { return tokenizer; }
    ZurchWordNormalizer& getNormalizer() { return normalizer; }
    const ZurchWordNormalizer& getNormalizer() const { return normalizer; }
    std::ostream& print( std::ostream& fp, const PrintContext& ctxt ) const;

    typedef enum {
        EXTRACTOR_TYPE_BASIC,
        EXTRACTOR_TYPE_NORMALIZING
    } extractor_type_t;

    ZurchTrainerAndClassifier(ay::UniqueCharPool& pool) : 
        stringPool(pool),
        extractor(0),
        classifier(0),
        accumulator(0),
        trainer(0)
    {}
    
    ~ZurchTrainerAndClassifier() ;
    void init( extractor_type_t t, const barzer::BZSpell* spell=0 ) ;

    void accumulate( size_t tagId, const ExtractedFeatureMap& f, bool hasTag, double docWeight=1.0 )
        { accumulator->accumulate(tagId,f,hasTag,docWeight); }
    void computeStats()
    {
        if( classifier && accumulator )
            classifier->stats().init(*accumulator);
    }

    void extractFeatures(ExtractedFeatureMap& features, const char* doc, bool learnMode)
        { classifier->extractFeatures(features,doc,learnMode); }
    void extractFeaturesLearning(ExtractedFeatureMap& features, const char* doc)
        { classifier->extractFeatures(features,doc,true); }
    void extractFeaturesClassification( ExtractedFeatureMap& features, const char* doc)
        { classifier->extractFeatures(features,doc,false); }

    void classifyAll( DocClassifier::TagIdToProbabilityMap& tagYesProb, const ExtractedFeatureMap& features ) const
        { classifier->classifyAll(tagYesProb,features); }
};

} // namespace zurch 
