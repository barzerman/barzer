#pragma once
#include <barzer_entity.h>
#include <barzer_barz.h>
#include <ay/ay_pool_with_id.h>
#include <barzer_parse.h>
#include <boost/filesystem.hpp>

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
        CLASS_TOKEN, /// legitimate token known to the barzer Universe
        CLASS_STEM, /// low grade token stem not known to barzer Universe 

        CLASS_MAX
    } class_t;
    class_t featureClass;

    DocFeature( ): featureId(0xffffffff), featureClass(CLASS_ENTITY) {}
    DocFeature( class_t c, uint32_t i ): featureId(i), featureClass(c) {}

    int serialize( std::ostream& ) const;
    int deserialize( std::istream& );
    
    bool isClassValid() const { return ( featureClass >= CLASS_ENTITY && featureClass < CLASS_MAX ); }
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
    
    enum {
        BIT_INTERN_STEMS,  /// interns all stems ergardless of whether or not they had been stored as literals in barzer Universe

        BIT_MAX
    };
    ay::bitflags<BIT_MAX> d_bits;
    
    bool internStems( ) const { return d_bits.check( BIT_INTERN_STEMS ); }
    void setInternStems( bool x=true ) { d_bits.set( BIT_INTERN_STEMS, x ); }

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
    
    /// returns the number of counted features 
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
    enum : size_t  { DEFAULT_BUF_SZ = 1024*128 };
private:
    size_t d_bufSz;
public:
    enum { MAX_QUERY_LEN = 1024*64, MAX_NUM_TOKENS = 1024*32 };
     
    DocFeatureIndex& index() { return d_index; }
    const DocFeatureIndex& index() const { return d_index; }

    barzer::Barz& barz() { return d_barz; }
    const barzer::Barz& barz() const { return d_barz; }
    const barzer::QParser& parser() const { return parser(); }
    barzer::QParser& parser() { return d_parser; }
    barzer::QuestionParm& qparm() { return d_qparm; }
    const barzer::QuestionParm& qparm() const { return d_qparm; }
    DocFeatureLoader( DocFeatureIndex& index, const barzer::StoredUniverse& u );
    virtual ~DocFeatureLoader();
    size_t getBufSz() const { return d_bufSz; }
    void setBufSz( size_t x ) { d_bufSz = x; }

    void addPieceOfDoc( uint32_t docId, const char* str );
    // returns roughly the number of all beads the document had been broken into  (this is at least as high as the number of features and in most cases equal to it)
    struct DocStats {
        size_t numPhrases;
        size_t numBeads; // total number of beads 
        size_t numFeatureBeads; /// total number of beads counted as features 

        DocStats() : numPhrases(0) , numBeads(0), numFeatureBeads(0) {} 

        std::ostream& print( std::ostream& ) const;
    };
    size_t addDocFromStream( uint32_t docId, std::istream&, DocStats&  );

    void parseTokenized() 
    {
        d_parser.lex_only( d_barz, d_qparm );
        d_parser.semanticize_only( d_barz, d_qparm );
    }
};

class DocFeatureIndexFilesystem : public DocFeatureLoader {
    ay::UniqueCharPool d_docnamePool; // both internal strings and literals will be in the pool 
public: 
    DocFeatureIndexFilesystem( DocFeatureIndex& index, const barzer::StoredUniverse& u  );
    ~DocFeatureIndexFilesystem( );
    
    uint32_t addDocName( const char* docName ) { return d_docnamePool.internIt(docName); }
    uint32_t getDocIdByName( const char* s ) const { return d_docnamePool.getId( s ); }
    const char* getDocNameById( uint32_t id ) const { return d_docnamePool.resolveId( id ); }
    
    struct LoaderOptions {
        std::string regex; /// only add files amtching the regexp
        enum {
            BIT_DIR_RECURSE,
            BIT_MAX
        };
        ay::bitflags<BIT_MAX> d_bits;
    };
    void addAllFilesAtPath( const char* path, const LoaderOptions& opt );
    
    /// filesystem iterator callback 
    struct fs_iter_callback {
        DocFeatureIndexFilesystem& index;
        bool usePureFileNames; /// when true (default) only file name (no path) is used. this is good when all file names are unique

        fs_iter_callback( DocFeatureIndexFilesystem& idx ) : index(idx), usePureFileNames(true) {}

        bool operator()( boost::filesystem::directory_iterator& di, size_t depth );
    };
};


// CB must have operator()( Barz& )
struct BarzTokPrintCB {
    std::ostream& fp;
    size_t count;
    BarzTokPrintCB(std::ostream& f ) : fp(f), count(0) {}
    void operator() ( barzer::Barz& barz ) {
        const auto& ttVec = barz.getTtVec();
        fp << "[" << count++ << "]:" << "(" << ttVec.size()/2+1 << ")";
        for( auto ti = ttVec.begin(); ti != ttVec.end(); ++ti )  {
            if( ti != ttVec.begin() )
                fp << " ";
            fp << ti->first.buf;
        }
        fp << std::endl;
    }
};
template <typename CB>
struct BarzerTokenizerCB {
    CB& callback;
    barzer::QParser& parser;
    barzer::Barz& barz;
    const barzer::QuestionParm& qparm;

    size_t count;
    std::string queryBuf; 

    BarzerTokenizerCB( CB& cb, barzer::QParser& p, barzer::Barz& b, const barzer::QuestionParm& qp ) : callback(cb), parser(p), barz(b), qparm(qp), count(0) {}
    void operator()( const char* s, size_t s_len ) { 
        ++count;
        queryBuf.assign( s, s_len );
        barz.clear();
        parser.tokenize_only( barz, queryBuf.c_str(), qparm );
        callback( barz );
    }
};
typedef BarzerTokenizerCB<BarzTokPrintCB> PrintStringCB;

struct PhraseBreaker {
    std::vector< char > buf;
    
    bool d_breakOnPunctSpace; /// when true 
    PhraseBreaker( ) : 
        buf( zurch::DocFeatureLoader::DEFAULT_BUF_SZ ), 
        d_breakOnPunctSpace(true) {}

    bool breakOnPunctuation( const char* s ) 
    {
        return( d_breakOnPunctSpace && ( ispunct(*s) && s[1] ==' ' ) );
    }
    // breaks the buffer into phrases . callback is invoked for each phrase
    // phrase delimiter is a new line character 
    // returns the size  of the processed text
    template <typename CB>
    size_t breakBuf( CB& cb, const char* str, size_t str_sz ) 
    {
        const char *s_to = str;
        for( const char* s_from=str, *s_end = str+str_sz; s_to <= s_end && s_from< s_end; ) {
            char c = *s_to;
            if( c  == '\n' || c == '\r' || c=='(' || c==')' || c=='\t' || c ==';' || (c ==' '&&s_to[1]=='.'&&s_to[2]==' ') || 
                breakOnPunctuation(s_to) 
            ) {
                if( s_to> s_from ) 
                    cb( s_from, s_to-s_from );
                s_from = ++s_to;
            } else
                ++s_to;
        }
        return s_to-str;
    }

    template <typename CB>
    void breakStream( CB& cb, std::istream& fp ) 
    {
        size_t curOffset = 0;
        while( true ) {
            size_t bytesRead = fp.readsome( &(buf[curOffset]), (buf.size()-curOffset-1) );
            if( !bytesRead )   
                return;
            if( bytesRead < buf.size() ) {
                buf[ bytesRead ] = 0;
                size_t endOffset = breakBuf(cb, &(buf[curOffset]), bytesRead );
                if( endOffset < bytesRead ) { /// something is left over
                    for( size_t dest = 0, src = endOffset; src< bytesRead; ++src, ++dest ) 
                        buf[dest] = buf[src];
                }
                curOffset= bytesRead-endOffset;
            } 
        }
    }
};

} // namespace zurch 
