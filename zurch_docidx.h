#pragma once
#include <barzer_entity.h>

namespace zurch {

class DocFeatureIndex; /// main object that links features to documents. the inverted index

class DocFeatureIndexHeuristics;

/// feature we keep track off (can be an entity or a token - potentially we will add more classes to it)
struct DocFeature {
    uint32_t featureId;

    typedef enum : uint8_t {
        CLASS_ENTITY,
        CLASS_TOKEN
    } class_t;
    class_t featureClass;

    DocFeature( ): featureId(0xffffffff), featureClass(CLASS_ENTITY) {}
    DocFeature( class_t c, uint32_t i ): featureId(i), featureClass(c) {}

    int serialize( std::ostream& ) const;
    int deserialize( std::istream& );
    
    bool isClassValid() const { return ( featureClass >= CLASS_ENTITY && featureClass <= CLASS_TOKEN ); }
    bool isValid() const { return (featureId!= 0xffffffff || isClassValid() ); }

    typedef DocFeature::Id_t;
};

inline operator< ( const DocFeature& l, const DocFeature& r ) 
{
    return( l.featureClass == r.featureClass ? (l.featureId < r.featureId) : l.featureClass < r.featureClass );
}
//// position  and weight of feature in the document
struct FeatureDocPosition {
    std::pair< uint32_t, uint32_t > offset; /// begin and end byte offsets
    int weight;  /// weight of this feature doc
    FeatureDocPosition() : offset(0,0), weight(0) {}

    int serialize( std::ostream& ) const;
    int deserialize( std::istream& );
};
/// every document is a vector of ExtractedDocFeature 
struct ExtractedDocFeature {
    DocFeature feature;
    FeatureDocPosition docPos;
    typedef std::vector< ExtractedDocFeature > Vec_t;

    int serialize( std::ostream& ) const;
    int deserialize( std::istream& );
    
    uint32_t simplePosition() const { return((docPos.offset.first+docPos.offset.second)/2) ; }
    int weight() const { return docPos.weight; }
};
////  ann array of DocFeatureLink's is stored for every feature in the corpus 
struct DocFeatureLink {
    uint32_t docId; 
    int      weight; /// -1000000, +100000 - negative means disassociation , 0 - neutral association, positive - boost
    uint32_t position;  /// some 1 dimensional positional number for feature within doc (can be middle between begin and end offset, or phrase number) 
    
    DocFeatureLink() : docId(0xffffffff), weight(0), position(0) {}
    DocFeatureLink(uint32_t i) : docId(i), weight(0), position(0) {}
    DocFeatureLink(uint32_t i, int w ) : docId(i), weight(w), position(0) {}
    
    typedef std::vector< DocFeatureLink > Vec_t;

    int serialize( std::ostream& ) const;
    int deserialize( std::istream& );
};

inline operator < ( const DocFeatureLink& l, const DocFeatureLink& r ) 
    { return ( l.weight == r.weight ? (l.docId< r.docId): r.weight< l.weight ); }

/// document id - uint32_t 
/// feature id  - uint32_t 
class DocFeatureIndex {
    typedef boost::map< DocFeature::Id_t,  DocFeatureLink::Vec_t > InvertedIdx_t;
    InvertedIdx_t d_invertedIdx;
    
    DocFeatureIndexHeuristics* d_heuristics; // /never 0 - guaranteed too be initialized in constructor
    
public:

    int serializeVec( std::ostream&, const DocFeatureLink::Vec_t& v ) const; 
    int deserializeVec( std::istream&, const DocFeatureLink::Vec_t& v );

    DocFeatureIndex();
    ~DocFeatureIndex();
    
    void appendDocument( uint32_t docId, const ExtractedDocFeature::Vec_t&  );
    /// should be called after the last doc has been appended . 
    void sortAll();

    typedef std::pair< uint32_t, double > DocWithScore_t;
    typedef std::vector< DocWithScore > DocWithScoreVec_t;

    void findDocument( DocWithScoreVec_t&, const ExtractedDocFeature::Vec_t& f );

    int serialize( std::ostream& fp ) const;
    int deserialize( std::istream& fp ); 

    std::ostream& printStats( std::ostream& ) const ;
};

} // namespace zurch 
