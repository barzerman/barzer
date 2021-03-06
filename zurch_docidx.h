
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#pragma once
#include <barzer_entity.h>
#include <barzer_barz.h>
#include <ay/ay_pool_with_id.h>
#include <barzer_parse.h>
#include <boost/filesystem.hpp>
#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>
#include <boost/property_tree/ptree.hpp>
#include <zurch_phrasebreaker.h>
#include <zurch_barzer.h>
#include <ay/ay_tag_markup_parser_types.h>
#include <zurch_docdataindex.h>

#include <zurch_docidx_types.h>

namespace barzer {
    struct ReqFilterCascade;
}
namespace zurch {
using barzer::BarzerEntity;
class ZurchRoute;

class DocFeatureIndex; /// main object that links features to documents. the inverted index

struct DocFeatureIndexHeuristics {
    enum : int {
        BIT_NOUNIQUE, // not unique 
        BIT_MAX
    };
    ay::bitflags<BIT_MAX> d_bits;
};

#pragma pack(push, 1)
/// feature we keep track off (can be an entity or a token - potentially we will add more classes to it)
struct DocFeature {
    uint32_t featureId;

    typedef enum : uint8_t {
        CLASS_STEM, /// low grade token stem not known to barzer Universe 
		CLASS_SYNGROUP, /// a group of synonyms
		CLASS_NUMBER, /// a number
        CLASS_TOKEN, /// legitimate token known to the barzer Universe
		CLASS_DATETIME, /// something datey
        CLASS_ENTITY,

        CLASS_MAX
    } class_t;
    class_t featureClass;

    // extra classification info 
    // for entities this is subordinate weight 0 - means no changes are needed, negative subweight means 
    // means that the feature needs to be suppressed relative to the feature of the same class 
    // positive subweight means it needs to be boosted
    int8_t  extraClass; /// not used in comparisons 

    DocFeature( ): featureId(0xffffffff), featureClass(CLASS_ENTITY), extraClass(0) {}
    DocFeature( class_t c, uint32_t i, int8_t sw=0 ): featureId(i), featureClass(c), extraClass(sw)  {}

    int serialize( std::ostream& ) const;
    int deserialize( std::istream& );
    
    bool isClassValid() const { return ( featureClass >= CLASS_STEM && featureClass < CLASS_MAX ); }
    bool isValid() const { return (featureId!= 0xffffffff || isClassValid() ); }
};
#pragma pack(pop)

inline bool operator< ( const DocFeature& l, const DocFeature& r ) 
{
    return( l.featureClass == r.featureClass ? (l.featureId < r.featureId) : l.featureClass < r.featureClass );
}

inline bool operator==(const DocFeature& l, const DocFeature& r)
{
	return l.featureClass == r.featureClass && l.featureId == r.featureId;
}

template<typename T, size_t MaxSize = 3>
class NGram
{
	T m_features[MaxSize];
public:
	enum { MaxGramSize = MaxSize };
	
	template<typename U>
	class NGramRange
	{
		U m_begin;
		U m_end;
		
		friend class NGram;
		
		NGramRange (U begin, U end)
		: m_begin(begin)
		, m_end(end)
		{
		}
	public:
		U begin() { return m_begin; }
		const U begin() const { return m_begin; }
		
		U end() { return m_end; }
		const U end() const { return m_end; }
	};
	
	NGram() {}
	
	explicit NGram(const T& f)
	{
		add(f);
	}
	
	NGram(const NGram& other)
	{
		memcpy(m_features, other.m_features, sizeof(m_features));
	}
	
	NGram& operator=(const NGram& other)
	{
		memcpy(m_features, other.m_features, sizeof(m_features));
		return *this;
	}
	
	void add(const T& f)
	{
		const size_t s = size();
		if (s == MaxSize)
			return;
		m_features[s] = f;
	}
	
	NGramRange<const T*> getFeatures() const { return NGramRange<const T*>(m_features, m_features + size()); }
	NGramRange<T*> getFeatures() { return NGramRange<T*>(m_features, m_features + size()); }
	
	const size_t size() const
	{
		for (size_t i = 0; i < MaxSize; ++i)
			if (m_features[i].featureId == 0xffffffff)
				return i;
		return MaxSize;
	}
	
	const T& operator[](size_t pos) const { return m_features[pos]; }
	
	bool operator==(const NGram<T>& other) const
	{
		return !memcmp(m_features, other.m_features, sizeof(m_features));
	}
	
	bool operator<(const NGram<T>& other) const
	{
		return memcmp(m_features, other.m_features, sizeof(m_features)) < 0;
	}
};

inline size_t hash_value(const NGram<DocFeature>& gram)
{
	size_t val = 0;
	for (const auto& f : gram.getFeatures())
		val += f.featureId * (f.featureClass == DocFeature::CLASS_ENTITY ? 1 : 10000);
	return val;
}

//// position  and weight of feature in the document
struct FeatureDocPosition {
	std::pair<uint32_t, uint16_t> offset;
    int weight;  /// weight of this feature doc
    FeatureDocPosition() : offset(0, 0), weight(0) {}
    FeatureDocPosition(uint32_t pos, uint16_t length = 0) : offset(pos, length), weight(0) {}

    int serialize( std::ostream& ) const;
    int deserialize( std::istream& );
};
/// every document is a vector of ExtractedDocFeature 
struct ExtractedDocFeature {
    NGram<DocFeature> feature;
    FeatureDocPosition docPos;
    size_t numTokensInFeature;

    size_t getRealTokenSize() const 
        { return ( numTokensInFeature> feature.size() ? numTokensInFeature: feature.size() ); }
        
    ExtractedDocFeature( const DocFeature& f, size_t numTok ) :
        feature(f) ,
        numTokensInFeature(numTok)
    {}
    ExtractedDocFeature( const DocFeature& f, const FeatureDocPosition& p, size_t numTok ) : 
        feature(f), docPos(p), numTokensInFeature(numTok) 
    {}

    typedef std::vector< ExtractedDocFeature > Vec_t;

    int serialize( std::ostream& ) const;
    int deserialize( std::istream& );
    
    uint32_t simplePosition() const { return((docPos.offset.first+docPos.offset.second)/2) ; }
};
////  ann array of DocFeatureLink's is stored for every feature in the corpus 
#pragma pack(push, 1)
struct DocFeatureLink {
    uint32_t docId; 
	
	typedef int16_t Weight_t;
    Weight_t weight; /// negative means disassociation , 0 - neutral association, positive - boost
    
    // we don't use it yet, and if we'd use we'd still need something more advanced
    uint32_t position;  /// some 1 dimensional positional number for feature within doc (can be middle between begin and end offset, or phrase number) 
    uint16_t length;
    uint16_t count; /// count of the feature in the doc 
    
    DocFeatureLink() : docId(0xffffffff), weight(0), position(-1), length(0), count(0) {}
    DocFeatureLink(uint32_t i) : docId(i), weight(0), position(-1), length(0), count(0) {}
    DocFeatureLink(uint32_t i, int16_t w ) : docId(i), weight(w), position(-1), length(0), count(0) {}
    
    typedef std::vector< DocFeatureLink > Vec_t;
	typedef boost::unordered_set< DocFeatureLink > Set_t;

    int serialize( std::ostream& ) const;
    int deserialize( std::istream& );
	
	void addPos(const FeatureDocPosition& newPos)
	{
		if (newPos.offset.first < position)
		{
			position = newPos.offset.first;
			length = newPos.offset.second;
		}
	}
};
#pragma pack(pop)

inline bool operator < ( const DocFeatureLink& l, const DocFeatureLink& r ) 
    { return ( l.weight == r.weight ? (l.docId< r.docId): r.weight< l.weight ); }
    
struct FeaturesStatItem
{
	/// human-readable text
	std::string m_hrText;
	
	/// a list of stats, like top ngrams and their scores
	struct GramInfo
	{
		NGram<DocFeature> gram;
		double score;
		size_t numDocs;
		size_t encounters;

        GramInfo( const NGram<DocFeature>& g, double s, size_t nd, size_t enc ) : 
            gram(g), 
            score(s),
            numDocs(nd),
            encounters(enc)
        {}
	};
	std::vector<GramInfo> m_values;
};
    
/// document id - uint32_t 
/// feature id  - uint32_t 
class DocFeatureIndex {
    typedef std::map< NGram<DocFeature>,  DocFeatureLink::Vec_t > InvertedIdx_t;
    InvertedIdx_t d_invertedIdx;
	
	std::map<uint32_t, std::pair<NGram<DocFeature>, size_t>> d_doc2topFeature;

    ay::UniqueCharPool d_stringPool; // both internal strings and literals will be in the pool 
    ay::InternerWithId<barzer::BarzerEntity> d_entPool; // internal representation of entities
    uint32_t m_meaningsCounter;

    DocFeatureIndexHeuristics d_heuristics; // /never 0 - guaranteed too be initialized in constructor
    
	std::set<uint32_t> m_stopWords;
	
	barzer::MeaningsStorage m_meanings;
	
	bool m_considerFCount;

	bool m_keepDetailedPositions;
public:
	typedef std::pair<uint32_t, uint16_t> DetailedPos_t;
	typedef std::vector<DetailedPos_t> DetailedPosList_t;
	typedef std::map<NGram<DocFeature>, DetailedPosList_t> Gram2DetailedPosList_t;
	typedef std::map<uint32_t, Gram2DetailedPosList_t> Doc2Gram2DetailedPosList_t;
private:
	Doc2Gram2DetailedPosList_t m_detailedPositions;
public:
	struct DocInfo
	{
		size_t titleLength;

		int32_t extWeight;

		DocInfo ()
		: titleLength (0)
		, extWeight (0)
		{
		}

		DocInfo (size_t length, int32_t w)
		: titleLength (length)
		, extWeight (w)
		{
		}
	};
    enum {
        ZBIT_SMART_SCORING,
        /// add new bits above this line only
        ZBIT_MAX
    };

    ay::bitflags<ZBIT_MAX> d_bitflags;
private:
	std::map<uint32_t, DocInfo> m_docInfos;
public:
    template <typename CB> 
    size_t visitAllDocs( const CB& cb) const {
        size_t count = 0;
        for( const auto& i : m_docInfos ) {
            cb( i.first );
            ++count;
        }
        return count;
    }
    bool isDumbScoring() const { return !d_bitflags.checkBit( DocFeatureIndex::ZBIT_SMART_SCORING) ; }

    const char*         resolve_token( uint32_t strId ) const;
    
    const BarzerEntity  resolve_entity( std::string& entIdStr, uint32_t entId, const barzer::StoredUniverse& u ) const;

	DocDataIndex            d_docDataIdx;
    ay::tagindex<uint32_t>  d_docTags;

	typedef std::vector<std::pair<uint32_t, uint16_t>> PosInfos_t;
	
	int   getFeaturesFromBarz( ExtractedDocFeature::Vec_t& featureVec, barzer::Barz& barz, bool needToInternStems );
	
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
	
	/** Adds the synonyms from the given universe.
	 * 
	 * Obviously they should be already loaded in the universe.
	 */
	void addSynonyms(const barzer::StoredUniverse& universe);
	void addSynonymsGroup(const std::vector<std::string>&);
	void loadSynonyms(const std::string&, const barzer::StoredUniverse&);
	
	const barzer::MeaningsStorage& getMeaningsStorage() const { return m_meanings; }
	
	bool hasTokenString(uint32_t) const;
	bool hasTokenString(const char*) const;

    uint32_t resolveExternalEntity( const barzer::BarzerEntity& ent, const barzer::StoredUniverse& u ) const 
        { 
            barzer::BarzerEntity internalEnt = translateExternalEntity(ent,u);
            return d_entPool.getIdByObj(internalEnt); 
        }

    /// place external entity into the pool (add all relevant strings to pool as well)
    uint32_t storeExternalEntity( const barzer::BarzerEntity& ent, const barzer::StoredUniverse& u );
    uint32_t storeOwnedEntity( const barzer::BarzerEntity& ent );

    uint32_t storeExternalString( const char*);
    uint32_t storeExternalString( const barzer::BarzerLiteral&, const barzer::StoredUniverse& u );
    uint32_t resolveExternalString( const char* str ) const { return d_stringPool.getId(str); }
    uint32_t resolveExternalString( const barzer::BarzerLiteral&, const barzer::StoredUniverse& u ) const;

	struct FeaturesQueryResult
	{
		NGram<DocFeature> m_gram;
		size_t m_uniqueness;
	};

	void getUniqueFeatures(std::vector<FeaturesQueryResult>& out,
			uint32_t docId, size_t maxGramSize = 1, size_t uniqueness = 1) const;
	const Gram2DetailedPosList_t* getDetailedFeaturesPositions(uint32_t docId) const;
	void getDocs4Feature(std::vector<uint32_t>& docIds, const NGram<DocFeature>& f) const;

    DocFeatureIndex();
    ~DocFeatureIndex();

	void setKeepDetailedPositions(bool set) { m_keepDetailedPositions = set; }
	bool isKeepDetailedPositionsEnabled() const { return m_keepDetailedPositions; }
    
    /// returns the number of counted features 

	void setStopWords(const std::vector<std::string>&);
	std::set<uint32_t> getStopWords() const { return m_stopWords; }

    int   fillFeatureVecFromQueryBarz( ExtractedDocFeature::Vec_t& featureVec, barzer::Barz& barz ) const;

    size_t appendOwnedEntity( uint32_t docId, const BarzerEntity& ent, float w ); 
	uint32_t getOwnedEntId(const BarzerEntity&) const;

    size_t appendDocument( uint32_t docId, const ExtractedDocFeature::Vec_t&, size_t posOffset, bool weightOverride );
    size_t appendDocument( uint32_t docId, barzer::Barz&, size_t posOffset, DocFeatureLink::Weight_t weight );
	
	void setTitleLength(uint32_t docId, size_t titleLength);
	void setExtWeight(uint32_t docId, int32_t weight);

	// returns the weight for the docId or 0 if not set
	int32_t getExtWeight(uint32_t docId) const;

	void setDocInfo(uint32_t docId, const DocInfo&);
	DocInfo* getDocInfo(uint32_t docId) ;
	const DocInfo* getDocInfo(uint32_t docId) const;

	void setConsiderFeatureCount(bool);
	
    /// should be called after the last doc has been appended . 
    void sortAll();

	struct FeatureTraceInfo
	{
		NGram<DocFeature> feature;
		std::string resolvedFeature;
		
		double scoreAdd;
		
		DocFeatureLink::Weight_t linkWeight;
		uint32_t linkCount;
		size_t numSources;
	};
	typedef std::map<uint32_t, std::vector<FeatureTraceInfo>> TraceInfoMap_t;
    
    struct SearchParm {
        size_t maxBack; 
        const barzer::ReqFilterCascade* filterCascade;

        std::vector< std::string >      docTags;

        std::map<uint32_t, PosInfos_t>* doc2pos;
		
		TraceInfoMap_t *f2trace;

        SearchParm() : 
            maxBack(16),
            filterCascade(0),
            doc2pos(0),
            f2trace(0)
        {}
        SearchParm( size_t mb, const barzer::ReqFilterCascade* fl, std::map<uint32_t, PosInfos_t>* p = 0, TraceInfoMap_t *f = 0 ) : 
            maxBack(mb),
            filterCascade(fl),
            doc2pos(p),
            f2trace(f)
        {}
        SearchParm( size_t mb, const barzer::ReqFilterCascade* fl, const std::vector< std::string >& tags ) :
            maxBack(mb),
            filterCascade(fl),
            docTags(tags),
            doc2pos(0),
            f2trace(0)
        {}
    };

	void findDocumentDumb( DocWithScoreVec_t&, const ExtractedDocFeature::Vec_t& f, SearchParm&, const barzer::Barz& ) const;

	void findDocument( DocWithScoreVec_t&, const ExtractedDocFeature::Vec_t& f, SearchParm&, const barzer::Barz& ) const;

	void getDocsByTags( DocWithScoreVec_t&, const SearchParm& ) const;

    int serialize( std::ostream& fp ) const;
    int deserialize( std::istream& fp ); 

    std::ostream& printStats( std::ostream& ) const ;
	
	std::string resolveFeature(const DocFeature&) const;
    std::ostream& streamFeature(std::ostream& fp, const DocFeature& f, ZurchRoute&) const;

	
	FeaturesStatItem getImportantFeatures(size_t count, double skipPerc) const;

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
	
	DocFeatureLink::Weight_t              m_curWeight;
	std::map<uint32_t, std::string>       m_docs;
	
	// bool m_storeParsed; // when true stores chunks
    
	std::map<uint32_t, std::string> m_parsedDocs;
	
	std::map<uint32_t, size_t> m_lastOffset;
public:
    bool isDumbScoring() const { return d_index.isDumbScoring(); }
    const barzer::StoredUniverse* getUniverse() const{ return &d_universe; }
    enum {
        BIT_NO_PARSE_CHUNKS, // when set doesnt store /output chunks (parse info)
        BIT_NO_STORE_CONTENT, // when set doesnt store / output doc content 

        BIT_MAX
    };
    ay::bitflags<BIT_MAX> d_bits;
    enum : size_t  { DEFAULT_BUF_SZ = 1024*128 };
    typedef enum { 
        LOAD_MODE_TEXT, // plain text
        LOAD_MODE_XHTML, // something tagged (XHTML or HTML)
        LOAD_MODE_PHRASE, // zurch phrase format
        LOAD_MODE_AUTO // UNIMPLEMENTED tries to automatically decide between txt and tags  
    } load_mode_t;
private:
    size_t d_bufSz;
public:
    ay::xhtml_mode_t    d_xhtmlMode; // ay::xhtml_parser_state::MODE_XXX (HTML - default or XHTML)
    load_mode_t                             d_loadMode; // one of LOAD_MODE_XXX constants LOAD_MODE_TEXT - default
    boost::unordered_map< uint32_t, std::string > d_docTitleMap; 

    size_t d_title_avgLength;
    
    double getTitleLengthAdjustment( uint32_t docId ) const;
    void computeTitleStats();

    bool testFilter( uint32_t docId, const barzer::ReqFilterCascade& filter ) const; 

    void setDocTitle( uint32_t docId, const char* s )
        { setDocTitle(docId, std::string(s)); }
    void setDocTitle( uint32_t docId, const std::string& s ) 
        { d_docTitleMap[ docId ] = s; d_index.setTitleLength(docId, s.size()); }
    const std::string getDocTitle( uint32_t docId ) const
        { const auto i = d_docTitleMap.find( docId ); return (i == d_docTitleMap.end() ?  std::string() : i->second ); }
    const size_t getDocTitleLength( uint32_t docId ) const
        { const auto i = d_docTitleMap.find( docId ); return (i == d_docTitleMap.end() ?  0  : i->second.length() ); }
        
	void setNoContent(bool x) { d_bits.set( BIT_NO_STORE_CONTENT, x ); }
    bool hasContent() const { return !d_bits.check( BIT_NO_STORE_CONTENT ); }

	void setNoChunks(bool x) { d_bits.set( BIT_NO_PARSE_CHUNKS, x ); }
    bool hasChunks() const { return !d_bits.check(BIT_NO_PARSE_CHUNKS); }
    bool noChunks() const { return d_bits.check(BIT_NO_PARSE_CHUNKS); }


    DocFeatureLoader( DocFeatureIndex& index, const barzer::StoredUniverse& u );
    virtual ~DocFeatureLoader();

    PhraseBreaker& phraser() { return d_phraser; }
    const PhraseBreaker& phraser() const  { return d_phraser; }
    
    DocFeatureLink::Weight_t setCurrentWeight(DocFeatureLink::Weight_t weight)
	{
		auto old = m_curWeight;
		m_curWeight = weight;
		return old;
	}
	DocFeatureLink::Weight_t getCurrentWeight() const { return ( isDumbScoring() ? 10:m_curWeight ); }

    enum { MAX_QUERY_LEN = 1024*64, MAX_NUM_TOKENS = 1024*32 };
     
    DocFeatureIndex& index() { return d_index; }
    const DocFeatureIndex& index() const { return d_index; }

    barzer::Barz& barz() { return d_barz; }
    const barzer::Barz& barz() const { return d_barz; }
    const barzer::QParser& parser() const { return parser(); }
    barzer::QParser& parser() { return d_parser; }
    barzer::QuestionParm& qparm() { return d_qparm; }
    const barzer::QuestionParm& qparm() const { return d_qparm; }
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
        DocStats& operator+=( const DocStats& o ) 
        {
            numPhrases+=o.numPhrases;
            numBeads+=o.numBeads;
            numFeatureBeads+=o.numFeatureBeads;
            return *this;
        }
    };
    std::map<uint32_t, size_t>::iterator getLastOffset(uint32_t docId);

    void parserSetup();
    size_t addDocFromString( uint32_t docId, const std::string& str, DocStats&, bool reuseBarz = false  );
    size_t addDocFromStream( uint32_t docId, std::istream&, DocStats&  );
	
	void addDocContents(uint32_t docId, const std::string& contents);
	void addDocContents(uint32_t docId, const char* contents );
	bool getDocContents(uint32_t docId, std::string& out) const;
	const char* getDocContentsStrptr(uint32_t ) const;
	
	/** Returns the amount of bytes that where actually appended. This may
	 * include some extra markup.
	 */
	size_t addParsedDocContents(uint32_t docId, const std::string& parsed );
	
	struct ChunkItem
	{
		std::string m_contents;
		bool m_isMatch;
		
		ChunkItem()
		: m_isMatch(false)
		{
		}
		
		ChunkItem(const std::string& contents, bool isMatch)
		: m_contents(contents)
		, m_isMatch(isMatch)
		{
		}
	};
	
	typedef std::vector<ChunkItem> Chunk_t;
	
    struct ChunkPositions {
    };

	void getBestChunks(uint32_t docId, 
			const DocFeatureIndex::PosInfos_t& positions,
			size_t chunkLength, 
			size_t count, 
			std::vector<Chunk_t>& chunks) const;

    void parseTokenized() ;
    friend class ZurchSettings;    
    // virtual bool loadProperties( const boost::property_tree::ptree& );
};

class DocIndexLoaderNamedDocs : public DocFeatureLoader {
    ay::UniqueCharPool d_docnamePool; // both internal strings and literals will be in the pool 
public: 
    BarzerEntityDocLinkIndex d_entDocLinkIdx;

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

    void loadEntLinks( const char* fname );

    LoaderOptions& loaderOpt() { return d_loaderOpt; }
    const LoaderOptions& loaderOpt() const { return d_loaderOpt; }

    void addAllFilesAtPath( const char* path );
	
    /// filesystem iterator callback 
    struct fs_iter_callback {
        DocIndexLoaderNamedDocs& index;
        bool usePureFileNames; /// when true (default) only file name (no path) is used. this is good when all file names are unique
        bool storeFullDocs;

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
    const barzer::StoredUniverse* getUniverse() const 
        { return ( loader ? loader->getUniverse() : 0 ); }
    DocIndexAndLoader() : index(0), loader(0) {}

    DocFeatureIndex* getIndex() { return index; }
    const DocFeatureIndex* getIndex() const { return index; }
    DocIndexLoaderNamedDocs* getLoader() { return loader; }
    const DocIndexLoaderNamedDocs* getLoader() const { return loader; }



    const char* getDocContentsByDocName( const char* s ) const;
    uint32_t getDocIdByName( const char* s ) const { return loader->getDocIdByName(s); }
    const char* getDocName ( uint32_t id ) const
        { return loader->getDocNameById( id ); }
    
    void init( const barzer::StoredUniverse& u );
    void destroy() { delete loader; delete index; loader = 0; index = 0;}

    void addAllFilesAtPath( const char* path ) { loader->addAllFilesAtPath(path); }
};

struct BarzerTokenizerCB_data {
    barzer::QParser& parser;
    barzer::Barz& barz;
    const barzer::QuestionParm& qparm;

    size_t count;
    std::string queryBuf; 

    BarzerTokenizerCB_data( barzer::QParser& p, barzer::Barz& b, const barzer::QuestionParm& qp ) : 
        parser(p), barz(b), qparm(qp), count(0) {}
};

// CB must have operator()( Barz& )
struct BarzTokPrintCB {
    std::ostream& fp;
    size_t count;
    BarzTokPrintCB(std::ostream& f ) : fp(f), count(0) {}
    void operator() ( BarzerTokenizerCB_data& dta, PhraseBreaker& phraser, barzer::Barz& barz, size_t, const char*, size_t );
};

template <typename CB>
struct BarzerTokenizerCB {
    BarzerTokenizerCB_data dta;
    CB& callback;
	
	size_t currentPos;

    BarzerTokenizerCB( CB& cb, barzer::QParser& p, barzer::Barz& b, const barzer::QuestionParm& qp ) : 
        dta(p,b,qp), callback(cb), currentPos(0) {}

    void operator()( PhraseBreaker& phraser, const char* s, size_t s_len )
	{
		++dta.count;
		dta.queryBuf.assign( s, s_len );
		dta.barz.clear();
		dta.parser.tokenize_only( dta.barz, dta.queryBuf.c_str(), dta.qparm );
		callback( dta, phraser, dta.barz, currentPos, s, s_len );

		currentPos += s_len;
	}
};
typedef BarzerTokenizerCB<BarzTokPrintCB> PrintStringCB;

} // namespace zurch 
