#pragma once
#include <barzer_entity.h>
#include <barzer_barz.h>
#include <ay/ay_pool_with_id.h>
#include <barzer_parse.h>
#include <boost/filesystem.hpp>
#include <boost/unordered_set.hpp>
#include <boost/property_tree/ptree.hpp>
#include <zurch_phrasebreaker.h>
#include <ay/ay_tag_markup_parser.h>


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
        CLASS_STEM, /// low grade token stem not known to barzer Universe 
        CLASS_TOKEN, /// legitimate token known to the barzer Universe
        CLASS_ENTITY,

        CLASS_MAX
    } class_t;
    class_t featureClass;

    DocFeature( ): featureId(0xffffffff), featureClass(CLASS_ENTITY) {}
    DocFeature( class_t c, uint32_t i ): featureId(i), featureClass(c) {}

    int serialize( std::ostream& ) const;
    int deserialize( std::istream& );
    
    bool isClassValid() const { return ( featureClass >= CLASS_ENTITY && featureClass < CLASS_MAX ); }
    bool isValid() const { return (featureId!= 0xffffffff || isClassValid() ); }
};

inline bool operator< ( const DocFeature& l, const DocFeature& r ) 
{
    return( l.featureClass == r.featureClass ? (l.featureId < r.featureId) : l.featureClass < r.featureClass );
}

template<typename T>
class NGram
{
	std::vector<T> m_features;
	decltype(T().featureClass) m_maxClass;
public:
	NGram() : m_maxClass(static_cast<decltype(m_maxClass)>(0)) {}
	explicit NGram(const T& f) : m_features({ f }), m_maxClass(f.featureClass) {}
	
	void add(const T& f)
	{
		m_features.push_back(f);
		if (f.featureClass > m_maxClass)
			m_maxClass = f.featureClass;
	}
	
	const std::vector<T>& getFeatures() const { return m_features; }
	
	const size_t size() const { return m_features.size(); }
	
	const T& operator[](size_t pos) const { return m_features[pos]; }
	
	decltype(m_maxClass) getClass() const { return m_maxClass; }
};

template<typename T>
bool operator<(const NGram<T>& l, const NGram<T>& r)
{
	const auto& lv = l.getFeatures();
	const auto& rv = r.getFeatures();
	if (lv.size() != rv.size())
		return lv.size() < rv.size();
	
	for (size_t i = 0; i < lv.size(); ++i)
		if (lv[i] < rv[i])
			return true;
		else if (rv[i] < lv[i])
			return false;
		
	return false;
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
    NGram<DocFeature> feature;
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
    
    // we don't use it yet, and if we'd use we'd still need something more advanced
    //uint32_t position;  /// some 1 dimensional positional number for feature within doc (can be middle between begin and end offset, or phrase number) 
    uint32_t count; /// count of the feature in the doc 
    
    DocFeatureLink() : docId(0xffffffff), weight(0), count(0) {}
    DocFeatureLink(uint32_t i) : docId(i), weight(0),count(0) {}
    DocFeatureLink(uint32_t i, int w ) : docId(i), weight(w), count(0) {}
    
    typedef std::vector< DocFeatureLink > Vec_t;
	typedef boost::unordered_set< DocFeatureLink > Set_t;

    int serialize( std::ostream& ) const;
    int deserialize( std::istream& );
};

inline bool operator < ( const DocFeatureLink& l, const DocFeatureLink& r ) 
    { return ( l.weight == r.weight ? (l.docId< r.docId): r.weight< l.weight ); }
    
struct FeaturesStatItem
{
	/// human-readable text
	std::string m_hrText;
	
	/// a list of stats, like top ngrams and their scores
	std::vector<std::pair<NGram<DocFeature>, double>> m_values;
};
    
/// document id - uint32_t 
/// feature id  - uint32_t 
class DocFeatureIndex {
    typedef std::map< NGram<DocFeature>,  DocFeatureLink::Vec_t > InvertedIdx_t;
    InvertedIdx_t d_invertedIdx;
	
	std::map<uint32_t, std::pair<NGram<DocFeature>, size_t>> d_doc2topFeature;

    ay::UniqueCharPool d_stringPool; // both internal strings and literals will be in the pool 
    ay::InternerWithId<barzer::BarzerEntity> d_entPool; // internal representation of entities

    DocFeatureIndexHeuristics d_heuristics; // /never 0 - guaranteed too be initialized in constructor
    
    const double d_classBoosts[DocFeature::CLASS_MAX];

public:
	int   getFeaturesFromBarz( ExtractedDocFeature::Vec_t& featureVec, const barzer::Barz& barz, bool needToInternStems );
	
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

    uint32_t storeExternalString( const char*);
    uint32_t storeExternalString( const barzer::BarzerLiteral&, const barzer::StoredUniverse& u );
    uint32_t resolveExternalString( const char* str ) const { return d_stringPool.getId(str); }
    uint32_t resolveExternalString( const barzer::BarzerLiteral&, const barzer::StoredUniverse& u ) const;

    DocFeatureIndex();
    ~DocFeatureIndex();
    
    /// returns the number of counted features 

    int   fillFeatureVecFromQueryBarz( ExtractedDocFeature::Vec_t& featureVec, const barzer::Barz& barz ) const;

    size_t appendDocument( uint32_t docId, const ExtractedDocFeature::Vec_t&, size_t posOffset );
    size_t appendDocument( uint32_t docId, const barzer::Barz&, size_t posOffset  );

    /// should be called after the last doc has been appended . 
    void sortAll();

    typedef std::pair< uint32_t, double > DocWithScore_t;
    typedef std::vector< DocWithScore_t > DocWithScoreVec_t;

    void findDocument( DocWithScoreVec_t&, const ExtractedDocFeature::Vec_t& f ) const;

    void findDocument( DocWithScoreVec_t&, const char* query, const barzer::QuestionParm& qparm ) const;

    int serialize( std::ostream& fp ) const;
    int deserialize( std::istream& fp ); 

    std::ostream& printStats( std::ostream& ) const ;
	
	std::string resolveFeature(const DocFeature&) const;
	
	FeaturesStatItem getImportantFeatures(size_t count = 20) const;

    friend class ZurchSettings;    
    // bool loadProperties( const boost::property_tree::ptree& );
};

/// objects to process actual documents and load them into an index
class DocFeatureLoader {
    const barzer::StoredUniverse&         d_universe;
    barzer::QuestionParm                  d_qparm;
    barzer::QParser                       d_parser;
    DocFeatureIndex&                      d_index;
    barzer::Barz                          d_barz;
    PhraseBreaker                         d_phraser;

public:
    enum : size_t  { DEFAULT_BUF_SZ = 1024*128 };
    typedef enum { 
        LOAD_MODE_TEXT, // plain text
        LOAD_MODE_XHTML, // something tagged (XHTML or HTML)
        LOAD_MODE_AUTO // UNIMPLEMENTED tries to automatically decide between txt and tags  
    } load_mode_t;
private:
    size_t d_bufSz;
public:
    ay::xhtml_parser_state::xhtml_mode_t    d_xhtmlMode; // ay::xhtml_parser_state::MODE_XXX (HTML - default or XHTML)
    load_mode_t                             d_loadMode; // one of LOAD_MODE_XXX constants LOAD_MODE_TEXT - default

    PhraseBreaker& phraser() { return d_phraser; }
    const PhraseBreaker& phraser() const  { return d_phraser; }

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
    friend class ZurchSettings;    
    // virtual bool loadProperties( const boost::property_tree::ptree& );
};

class DocIndexLoaderNamedDocs : public DocFeatureLoader {
    ay::UniqueCharPool d_docnamePool; // both internal strings and literals will be in the pool 
public: 
    DocIndexLoaderNamedDocs( DocFeatureIndex& index, const barzer::StoredUniverse& u  );
    ~DocIndexLoaderNamedDocs( );
    
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
    } d_loaderOpt;

    LoaderOptions& loaderOpt() { return d_loaderOpt; }
    const LoaderOptions& loaderOpt() const { return d_loaderOpt; }

    void addAllFilesAtPath( const char* path );
    
    /// filesystem iterator callback 
    struct fs_iter_callback {
        DocIndexLoaderNamedDocs& index;
        bool usePureFileNames; /// when true (default) only file name (no path) is used. this is good when all file names are unique

        fs_iter_callback( DocIndexLoaderNamedDocs& idx ) : index(idx), usePureFileNames(true) {}

        bool operator()( boost::filesystem::directory_iterator& di, size_t depth );
    };
    friend class ZurchSettings;    
    // virtual bool loadProperties( const boost::property_tree::ptree& );
};

class DocIndexAndLoader {
    DocFeatureIndex*           index;
    DocIndexLoaderNamedDocs* loader;
public:
    DocIndexAndLoader() : index(0), loader(0) {}

    DocFeatureIndex* getIndex() { return index; }
    const DocFeatureIndex* getIndex() const { return index; }
    DocIndexLoaderNamedDocs* getLoader() { return loader; }
    const DocIndexLoaderNamedDocs* getLoader() const { return loader; }

    const char* getDocName ( uint32_t id ) const
        { return loader->getDocNameById( id ); }
    
    void init( const barzer::StoredUniverse& u );
    void destroy() { delete loader; delete index; loader = 0; index = 0;}

    void addAllFilesAtPath( const char* path ) { loader->addAllFilesAtPath(path); }
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
    void operator()( const char* s, size_t s_len, const PhraseBreakerState& state) { 
        ++count;
        queryBuf.assign( s, s_len );
        barz.clear();
        parser.tokenize_only( barz, queryBuf.c_str(), qparm );
        callback( barz );
    }
};
typedef BarzerTokenizerCB<BarzTokPrintCB> PrintStringCB;

} // namespace zurch 
