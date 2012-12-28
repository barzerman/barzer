#pragma once
#include <barzer_entity.h>
#include <barzer_barz.h>
#include <ay/ay_pool_with_id.h>
#include <barzer_parse.h>

namespace zurch {

class DocFeatureIndex; /// main object that links features to documents. the inverted index

struct DocFeatureIndexHeuristics {
    enum : int {
        BIT_NOUNIQUE, // not unique 
        BIT_MAX
    };
    ay::bitflags<BIT_MAX> d_bits;
};

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

    typedef DocFeature Id_t;
};

inline bool operator< ( const DocFeature& l, const DocFeature& r ) 
{
    return( l.featureClass == r.featureClass ? (l.featureId < r.featureId) : l.featureClass < r.featureClass );
}
//// position  and weight of feature in the document
struct FeatureDocPosition {
    std::pair< uint32_t, uint32_t > offset; /// begin and end byte offsets
    int weight;  /// weight of this feature doc
    FeatureDocPosition() : offset(0,0), weight(0) {}
    FeatureDocPosition( uint32_t o ) : offset(o,o), weight(0) {}

    int serialize( std::ostream& ) const;
    int deserialize( std::istream& );
};
/// every document is a vector of ExtractedDocFeature 
struct ExtractedDocFeature {
    DocFeature feature;
    FeatureDocPosition docPos;
    ExtractedDocFeature( const DocFeature& f, const FeatureDocPosition& p ) : 
        feature(f), docPos(p) {}

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
    uint32_t count; /// count of the feature in the doc 
    
    DocFeatureLink() : docId(0xffffffff), weight(0), position(0), count(0) {}
    DocFeatureLink(uint32_t i) : docId(i), weight(0), position(0), count(0) {}
    DocFeatureLink(uint32_t i, int w ) : docId(i), weight(w), position(0), count(0) {}
    DocFeatureLink(uint32_t i, int w, uint32_t p ) : docId(i), weight(w), position(p), count(0) {}
    
    typedef std::vector< DocFeatureLink > Vec_t;

    int serialize( std::ostream& ) const;
    int deserialize( std::istream& );
};

inline bool operator < ( const DocFeatureLink& l, const DocFeatureLink& r ) 
    { return ( l.weight == r.weight ? (l.docId< r.docId): r.weight< l.weight ); }

/// document id - uint32_t 
/// feature id  - uint32_t 
class DocFeatureIndex {
    typedef std::map< DocFeature::Id_t,  DocFeatureLink::Vec_t > InvertedIdx_t;
    InvertedIdx_t d_invertedIdx;

    ay::UniqueCharPool d_stringPool; // both internal strings and literals will be in the pool 
    ay::InternerWithId<barzer::BarzerEntity> d_entPool; // internal representation of entities

    DocFeatureIndexHeuristics d_heuristics; // /never 0 - guaranteed too be initialized in constructor
public:

    /// given an entity from the universe returns internal representation of the entity 
    /// if it can be found and null entity (isValid() == false) otherwise
    barzer::BarzerEntity translateExternalEntity( const barzer::BarzerEntity& ent, const barzer::StoredUniverse& u ) const;

    uint32_t resolveExternalEntity( const barzer::BarzerEntity& ent, const barzer::StoredUniverse& u ) const 
        { return d_entPool.getIdByObj(translateExternalEntity(ent,u)); }

    /// place external entity into the pool (add all relevant strings to pool as well)
    uint32_t storeExternalEntity( const barzer::BarzerEntity& ent, const barzer::StoredUniverse& u );

    uint32_t storeExternalString( const char*, const barzer::StoredUniverse& u );
    uint32_t storeExternalString( const barzer::BarzerLiteral&, const barzer::StoredUniverse& u );
    uint32_t resolveExternalString( const char* str ) const { return d_stringPool.getId(str); }
    uint32_t resolveExternalString( const barzer::BarzerLiteral&, const barzer::StoredUniverse& u ) const;

    int serializeVec( std::ostream&, const DocFeatureLink::Vec_t& v ) const; 
    int deserializeVec( std::istream&, const DocFeatureLink::Vec_t& v );

    DocFeatureIndex();
    ~DocFeatureIndex();
    
    /// return new offset (assuming it begins with posOffset)
    size_t appendDocument( uint32_t docId, const ExtractedDocFeature::Vec_t&, size_t posOffset );
    size_t appendDocument( uint32_t docId, const barzer::Barz&, size_t posOffset  );
    /// should be called after the last doc has been appended . 
    void sortAll();

    typedef std::pair< uint32_t, double > DocWithScore_t;
    typedef std::vector< DocWithScore_t > DocWithScoreVec_t;

    void findDocument( DocWithScoreVec_t&, const ExtractedDocFeature::Vec_t& f );

    int serialize( std::ostream& fp ) const;
    int deserialize( std::istream& fp ); 

    std::ostream& printStats( std::ostream& ) const ;
};

/// objects to process actual documents and load them into an index
class DocFeatureLoader {
    const barzer::StoredUniverse&         d_universe;
    barzer::QuestionParm                  d_qparm;
    barzer::QParser                       d_parser;
    DocFeatureIndex&                      d_index;
    barzer::Barz                          d_barz;
public:
    DocFeatureLoader( DocFeatureIndex& index, const barzer::StoredUniverse& u );
    void addPieceOfDoc( uint32_t docId, const char* str );
};

} // namespace zurch 
