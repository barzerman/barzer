#pragma once

#include <barzer_entity.h>
#include <boost/range/iterator_range.hpp>
#include <zurch_docidx_types.h>
#include <ay_util_time.h>
#include <boost/regex.hpp>
#include <boost/function.hpp>


namespace barzer {

class StoredUniverse;

template <typename T> class NGramStorage;
typedef NGramStorage<BarzerEntity> BarzerEntityNgramStorage;

class Barz;
struct QuestionParm;


typedef boost::function<bool (BarzerEntity)> BENIFilter_f;

class BENI {
    ay::UniqueCharPool d_charPool;
	
    BarzerEntityNgramStorage * d_storage;
	std::map<BarzerEntity, std::string> d_backIdx;

	BarzerEntityNgramStorage & storage() { return *d_storage; }
public:
    ay::UniqueCharPool& charPool() { return d_charPool; }
    const ay::UniqueCharPool& charPool() const { return d_charPool; }

    StoredUniverse& d_universe;
	
	const BarzerEntityNgramStorage & getStorage() const { return *d_storage; }
	
	void addWord(const std::string& str, const BarzerEntity&);

    /// add all entities by name for given class 
    void addEntityClass( const StoredEntityClass& ec );
    /// returns max coverage
    double search( BENIFindResults_t&, const char* str, double minCov, const BENIFilter_f& = BENIFilter_f ()) const;

    BENI( StoredUniverse& u );
    ~BENI();
    void clear();

	BENI(const BENI&) = delete;
	BENI& operator=(const BENI&) = delete;
    
    static bool normalize_old( std::string& out, const std::string& in, const StoredUniverse* ) ;
    static bool normalize( std::string& out, const std::string& in, const StoredUniverse* ) ;
    void setSL( bool );
};

class SmartBENI {
    BENI d_beniStraight,  // beni without soundslike normalization
         d_beniSl;        // beni with sounds like normalization
    /// if d_isSL == true d_beniSl has sounds like processed ngrams .. otherwise it's blank
    bool d_isSL;
    StoredUniverse& d_universe;
    StoredUniverse* d_zurchUniverse;

    std::vector< std::pair<boost::regex, std::string> > d_mandatoryRegex;

    void search_single_query(
        BENIFindResults_t& out, 
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

    bool hasMandatoryRegex() const {return !d_mandatoryRegex.empty(); }
    void applyMandatoryRegex( std::string& dest ) const;
    void addMandatoryRegex( const std::string& find, const std::string& replace );

    void search( BENIFindResults_t&, const char* str, double minCov, Barz* barz, const BENIFilter_f& = BENIFilter_f (), size_t maxCount=128) const;
	
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
    int search( BENIFindResults_t& out, const char* query, const StoredEntityClass& sc, double minCov=.3 ) const;  
};

} // namespace barzer
