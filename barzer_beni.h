#pragma once
#include <barzer_entity.h>
// #include <barzer_spell_features.h>
#include <zurch_docidx_types.h>
#include <ay_util_time.h>
#include <boost/function.hpp>
#include <barzer_beni_storage.h>

namespace barzer {

struct BeniEntFormat {
      enum : size_t {
          TOK_CLASS=0,
          TOK_SUBCLASS,
          TOK_ID,
          TOK_RELEVANCE,
          TOK_NAME,
          ///
          TOK_TOPIC_CLASS,
          TOK_TOPIC_SUBCLASS,
          TOK_TOPIC_ID,
          TOK_MAX
      };
      char d_sep;
      static const char* tokName(size_t t);
      std::vector<size_t> d_extraNameTok; // when not blank values of offsets these tokens will be also indexed as name
      size_t d_tokTranslation[TOK_MAX];
      static const char* getTokName( size_t t );
      size_t getTokOffset( size_t tok ) const { return d_tokTranslation[tok];}
      // argument format of the benient tag
      // format: field name - one of the (id, name, extra, relevance)
      void initFormat( const std::string& formatStr );
      BeniEntFormat();
      std::ostream& print( std::ostream& fp ) const;
      void clear();
};
class StoredUniverse;

typedef boost::function<bool (BarzerEntity)> BENIFilter_f;

class BENI {
    ay::UniqueCharPool d_charPool;

    NGramStorage d_storage;
	std::map<BarzerEntity, std::string> d_backIdx;
public:
    StoredUniverse& d_universe;

	const NGramStorage& getStorage() const { return d_storage; }

	void addWord(const std::string& str, const BarzerEntity&);

    /// add all entities by name for given class
    void addEntityClass( const StoredEntityClass& ec );
    /// returns max coverage
    double search( BENIFindResults_t&, const QuestionParm& qparm, const char* str, double minCov, const BENIFilter_f& = BENIFilter_f ()) const;

    void clear() { d_storage.clear(); }
    BENI( StoredUniverse& u );

	BENI(const BENI&) = delete;
	BENI& operator=(const BENI&) = delete;

    static bool normalize_old( std::string& out, const std::string& in, const StoredUniverse* ) ;
    static bool normalize( std::string& out, const std::string& in, const StoredUniverse* ) ;
    void setSL( bool );
};

class SmartBENI {
    enum {
        NAME_MODE_OVERRIDE,
        NAME_MODE_SKIP
    };
    BENI d_beniStraight,  // beni without soundslike normalization
         d_beniSl;        // beni with sounds like normalization
    /// if d_isSL == true d_beniSl has sounds like processed ngrams .. otherwise it's blank
    bool d_isSL;
    StoredUniverse& d_universe;
    StoredUniverse* d_zurchUniverse;

    std::vector< std::pair<boost::regex, std::string> > d_mandatoryRegex;

    void search_single_query(
        BENIFindResults_t& out,
        const QuestionParm& qparm,
        const char* query,
        double minCov,
        Barz* barz,
        const BENIFilter_f& filter,
        size_t maxCount
        ) const;
    void search_post_processing(
        BENIFindResults_t& out,
        double minCov,
        Barz* barz,
        const BENIFilter_f& filter,
        size_t maxCount
        ) const;
public:
    bool hasSoundslike() const { return d_isSL; }
    BENI& beniStraight() { return  d_beniStraight; }
    BENI& beniSl()       { return d_beniSl; }

    typedef boost::unordered_multimap< uint32_t, BarzerEntity > Doc2EntMap;
    Doc2EntMap d_zurchDoc2Ent;

    void clear();
    SmartBENI( StoredUniverse& u );

    void addEntityClass( const StoredEntityClass& ec );
    /// reads info from path . if path ==0 - from stdin
    /// assumes the format as class|subclass|id|searchable name[|topic class|topic subclass|topic id]
    /// when class or subclass are blank the value is taken from defaults
    /// one netity per line. line cant be longer than 1024 bytes
    /// lines with leading # are skipped as comments
    /// if searchable name is blank this line will be used only for entity topic linkage
    /// if topic id is blank nothing will be done with topics
    size_t addEntityFile( const char* path, const char* modeStr, const StoredEntityClass& /*entity defaults*/, const StoredEntityClass& /* topic defaults */ );

    size_t readFormattedEntityFile( const char* path, const BeniEntFormat& format, const char* modeStr, const StoredEntityClass& /*entity defaults*/, const StoredEntityClass& /* topic defaults */ );

    bool hasMandatoryRegex() const {return !d_mandatoryRegex.empty(); }
    void applyMandatoryRegex( std::string& dest ) const;
    void addMandatoryRegex( const std::string& find, const std::string& replace );

    void search( BENIFindResults_t&, const QuestionParm& qparm, const char* str, double minCov, Barz* barz, const BENIFilter_f& = BENIFilter_f (), size_t maxCount=128) const;

	BENI& getPrimaryBENI();

    void linkEntToZurch( const BarzerEntity& ent, uint32_t docId )
        { d_zurchDoc2Ent.insert( { docId, ent } ); }
    void clearZurchLinks()
        { d_zurchDoc2Ent.clear(); }

    template <typename CB>
    void getEntLinkedToZurchDoc( const CB& cb, uint32_t docId ) const
    {
        auto r = d_zurchDoc2Ent.equal_range(docId);
        for( auto i = r.first; i != r.second; ++i )
            cb( i->second );
    }

    // returns the number of entities added to out
    size_t getZurchEntities( BENIFindResults_t& out, const zurch::DocWithScoreVec_t& vec ) const;

    void setZurchUniverse( StoredUniverse* u ) { d_zurchUniverse= u; }

    StoredUniverse* getZurchUniverse() { return d_zurchUniverse; }
    const StoredUniverse* getZurchUniverse() const { return d_zurchUniverse; }
    void zurchEntities( BENIFindResults_t& out, const char* str, const QuestionParm& qparm );
};

class SubclassBENI {
	StoredUniverse& m_universe;

	std::map<StoredEntityClass, BENI*> m_benies;
public:
	SubclassBENI(StoredUniverse& uni)
	: m_universe (uni)
	{
	}

	SubclassBENI(const SubclassBENI&) = delete;
	SubclassBENI& operator=(const SubclassBENI&) = delete;

	~SubclassBENI();

	void clear();

	void addSubclassIds(const StoredEntityClass&, const char *pattern = 0, const char *replace = 0);
	const BENI* getBENI(const StoredEntityClass& x) const
    {
	    const auto pos = m_benies.find(x);
	    return pos == m_benies.end() ? nullptr : pos->second;
    }

    enum {
        ERR_OK,
        ERR_NO_BENI, /// beni for subclass not found

        ERR_MAX
    };
    // returns ERR_XXX
    int search( BENIFindResults_t& out, const QuestionParm& qparm, const char* query, const StoredEntityClass& sc, double minCov=.3 ) const;
};

} // namespace barzer
