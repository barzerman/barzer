
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#pragma once 
#include <map>
#include <boost/unordered_map.hpp>
#include <ay/ay_logger.h>
#include <ay/ay_string_pool.h>
#include <ay/ay_util.h>

namespace barzer {
class StoredUniverse;
/// class of the stored entity . for now we assume the hierarchy is 
/// 1-2 levels deep 
struct StoredEntityClass {
	uint32_t ec, subclass;
	
	StoredEntityClass() : ec(0), subclass(0) {}
	explicit StoredEntityClass(uint32_t c) : ec(c), subclass(0) 
		{}
	explicit StoredEntityClass(uint32_t c, uint32_t es) : ec(c), subclass(es) 
		{}
	
	void setClass( uint32_t c ) { ec = c; }
	void setSubclass( uint32_t c ) { subclass = c; }
	void set( uint32_t c, uint32_t sc )
		{ ec = c; subclass = sc; }
	void reset() { ec = subclass = 0; }

	bool isValid() const { return (ec!=0); }

	void print( std::ostream& fp ) const
		{ fp << ec << ":" << subclass ; }
	
	bool matchOther( const StoredEntityClass& other ) const {
		if( !ec ) return true;
		else return ( (ec == other.ec) && (!subclass || subclass == other.subclass) );
	}
    void init( const char* str, char delim =',' )
    {
        if( !str ) { 
            ec = subclass = 0;
            return;
        }
        ec = atoi(str);
        if( const char *s = strchr(str,delim) ) {
            subclass = atoi( s+1 );
        } else
            subclass = 0;
    }
    void init( const char* c, const char* sc ) 
    {
        ec = ( c ? atoi(c) : 0 ) ;
        subclass = ( sc ? atoi(sc) : 0 ) ;
    }
    bool lowerClass( uint32_t c ) const { return ( ec< c); }
};

inline std::ostream& operator <<( std::ostream& fp, const StoredEntityClass& x )
{
	x.print(fp);
	return fp;
}

inline bool operator ==( const StoredEntityClass& l, const StoredEntityClass& r )
{ return (l.ec == r.ec && l.subclass == r.subclass ); }
inline bool operator !=( const StoredEntityClass& l, const StoredEntityClass& r )
{ return (l.ec != r.ec || l.subclass != r.subclass ); }
inline bool operator<( const StoredEntityClass& l, const StoredEntityClass& r )
{
	if( l.ec < r.ec ) return true;
	else if( r.ec < l.ec ) return false;
	else return ( l.subclass < r.subclass );
}
typedef std::vector< StoredEntityClass > StoredEntityClassVec;

/// unique id for an entity within the given StoredEntityClass
struct StoredEntityUniqId {
	uint32_t     tokId;
	StoredEntityClass eclass;
    
    uint32_t getEntityClass() const { return eclass.ec; }
    void setClass( const StoredEntityClass& e ) { eclass= e; }
	void setClass( uint32_t c ) { eclass.setClass( c ); }
	void setSubclass( uint32_t c ) { eclass.setSubclass( c ); }
	void setTokenId( uint32_t i ) { tokId = i; }

	StoredEntityUniqId() : tokId(0xffffffff) {}
	StoredEntityUniqId(uint32_t tid, uint32_t  cl, uint32_t  sc ) : 
		tokId(tid),
		eclass(cl,sc)
	{}
	explicit StoredEntityUniqId( const StoredEntityClass& ec ) : 
		tokId(0xffffffff),
		eclass(ec)
	{}
	explicit StoredEntityUniqId( const StoredEntityClass& ec, uint32_t tid  ) : 
        tokId(tid),
        eclass(ec)
    {}
    
	inline bool isTokIdValid() const { return (tokId!=0xffffffff); }
	inline bool isValid() const { return ( isTokIdValid() && eclass.isValid() ); }
	inline bool isValidForMatching() const { return ( eclass.ec ); }
	void print( std::ostream& fp ) const { fp << eclass << "," << tokId ; }

    enum { ENT_PRINT_FMT_PIPE /*pipe separated*/, ENT_PRINT_FMT_XML /*xml*/};
	void print( std::ostream& fp, const StoredUniverse& universe, int fmt = ENT_PRINT_FMT_XML ) const;
	
	bool matchOther( const StoredEntityUniqId& other ) const
		{ return( eclass.matchOther( other.eclass ) && ( !isTokIdValid() || tokId == other.tokId ) ); }

	bool matchOther( const StoredEntityUniqId& other, bool matchInvalid ) const
		{ return( eclass.matchOther( other.eclass ) && (tokId == other.tokId || !(matchInvalid || isTokIdValid()) ) ); }

    const StoredEntityClass& getClass()  const { return eclass; }
    uint32_t getTokId() const { return tokId; }
    uint32_t getId() const { return tokId ; }
    void     setId(uint32_t id=0xffffffff) { tokId = id; }

    bool     lowerClass( uint32_t c ) const { return eclass.lowerClass(c); }
};
inline std::ostream& operator <<(std::ostream& fp, const StoredEntityUniqId& x )
{
	x.print(fp);
	return fp;
}

inline bool operator ==(const StoredEntityUniqId& l, const StoredEntityUniqId& r ) 
{
	return ( l.tokId == r.tokId && l.eclass == r.eclass );
}

inline bool operator <(const StoredEntityUniqId& l, const StoredEntityUniqId& r ) 
{
	return( l.eclass< r.eclass ?
		true :
		(r.eclass< l.eclass ? false: (l.tokId < r.tokId)) 
	);
		
}

inline size_t hash_value(const StoredEntityUniqId& t)
{
	return (t.eclass.ec << 24) + (t.eclass.subclass << 16) + t.tokId;
}

typedef StoredEntityUniqId BarzerEntity;

struct BENIFindResult {
	BarzerEntity ent;
	int popRank=0; // entity popularity rank
	double coverage = 0;
	double relevance = 0;
    size_t nameLen=0;

    BENIFindResult(const BarzerEntity& e ) : ent(e) {}
    BENIFindResult() = default;
    BENIFindResult( const BarzerEntity& e, int pr, double c, double r, size_t nl ) : 
        ent(e), 
        popRank(pr),
        coverage(c),
        relevance(r),
        nameLen(nl)
    {}

};

inline bool operator<( const BENIFindResult& l, const BENIFindResult& r ) 
    { return l.ent< r.ent ; }
typedef std::vector<BENIFindResult> BENIFindResults_t; 

//// generic type used by Barzel among other things
class BarzerEntityList {
public:
	typedef std::vector< BarzerEntity > EList;
private:
	EList d_lst;
	StoredEntityClass d_class;
	
public:
    bool isEqual( const BarzerEntityList& o ) const { return ( d_class == o.d_class && d_lst == o.d_lst ) ; } 
    bool isLessThan( const BarzerEntityList& o ) const { return ( d_class < o.d_class && d_lst < o.d_lst ) ; } 

    const StoredEntityClass& getClass() const { return d_class; }

    void setClass( const StoredEntityClass& cl ) 
        { d_class = cl; }
	struct comp_ent_less {
		bool operator() ( const BarzerEntity& l, const BarzerEntity& r ) const 
			{ return ay::range_comp().less_than( l.eclass, r.tokId, r.eclass, l.tokId); }
	};
	struct comp_eclass_less {
		bool operator() ( const BarzerEntity& l, const BarzerEntity& r ) const 
			{ return ( l.eclass < r.eclass ); }
	};
	EList& theList() { return d_lst; }

	const EList& getList() const 
		{ return d_lst; }
    size_t size() const { return d_lst.size(); }
    const BarzerEntity& operator[]( size_t i ) const { return d_lst[i]; }

	bool hasClass( const StoredEntityClass& ec ) const
		{ return std::binary_search( d_lst.begin(), d_lst.end(), BarzerEntity(ec), comp_ent_less() ); }

	bool hasEntity( const BarzerEntity& ent ) const
		{ return std::binary_search( d_lst.begin(), d_lst.end(), ent, comp_ent_less() ); }

	void addEntity(const BarzerEntity &e) 
		{ 
			EList::iterator i = std::lower_bound( d_lst.begin(), d_lst.end(), e, comp_ent_less() );
			if( i == d_lst.end() || !( *i == e ) )
				d_lst.insert(i,e);
		}

	void appendEList( const EList& lst )
    {
        d_lst.insert( d_lst.end(), lst.begin(), lst.end() );
    }
	void filterEntList( BarzerEntityList& lst, const StoredEntityUniqId& euid ) const
	{
		for( EList::const_iterator i = d_lst.begin(); i!= d_lst.end(); ++i ) {
			if( euid.matchOther( *i ) ) 
				lst.d_lst.push_back( *i );
		}
	}
    void clear() { 
        d_class = StoredEntityClass();
        d_lst.clear(); 
    }
	std::ostream& print( std::ostream& fp ) const;
};

inline std::ostream& operator << ( std::ostream& fp, const BarzerEntityList& e ) 
    { return e.print( fp ); }
inline bool operator< ( const BarzerEntityList& l, const BarzerEntityList& r ) { return l.isLessThan(r); }
inline bool operator== ( const BarzerEntityList& l, const BarzerEntityList& r ) { return l.isEqual(r); }
inline bool operator!= ( const BarzerEntityList& l, const BarzerEntityList& r ) { return !l.isEqual(r); }

typedef std::pair< BarzerEntity*, BarzerEntityList* >  EntityOrListPair;
typedef std::pair< const BarzerEntity*, const BarzerEntityList* >  const_EntityOrListPair;

//// Best entities is a group of up to N entities sorted by BestEntities_EntWeight
struct BestEntities_EntWeight {
    uint32_t pathLen;
    uint32_t relevance;
    
    BestEntities_EntWeight( ) : 
        pathLen(0), relevance(0)
    {}
    BestEntities_EntWeight( uint32_t pl, uint32_t rel=0 ) : 
        pathLen(pl), relevance(rel)
    {}
};

inline bool operator <( const BestEntities_EntWeight& l, const BestEntities_EntWeight& r ) 
{
    return ( l.pathLen+r.relevance < r.pathLen + l.relevance );
}

struct StoredEntityUniqId_Hash {
    long operator() ( const StoredEntityUniqId& euid ) const 
    {
        return( euid.tokId + euid.eclass.ec + euid.eclass.subclass );
    }
};

class BestEntities {
    uint32_t d_maxEnt; 

public: 
    enum { DEFAULT_MAX_ENT = 64 };
    typedef std::multimap< BestEntities_EntWeight, StoredEntityUniqId > EntWeightMap;
    
private:
    EntWeightMap d_weightMap;
    // typedef std::map< StoredEntityUniqId, EntWeightMap::iterator > EntIdMap;
    typedef boost::unordered_map< StoredEntityUniqId, EntWeightMap::iterator, StoredEntityUniqId_Hash > EntIdMap;
    EntIdMap d_entMap;
public:
    BestEntities( uint32_t mxe = DEFAULT_MAX_ENT ) : d_maxEnt(mxe) {}
    void addEntity( const StoredEntityUniqId& euid, uint32_t pathLen, uint32_t relevance=0 );
    bool isFull() const
        { return (d_weightMap.size() >= d_maxEnt); }
    const EntWeightMap& getEntitiesAndWeights() const 
        { return d_weightMap; }

    uint32_t getNumEnt() const { return d_weightMap.size(); }
    uint32_t getMaxNumEnt() const { return d_maxEnt; }

    void clear() { 
        d_entMap.clear();
        d_weightMap.clear();
    }
};
/// end of best entities 
class GlobalPools;
/// entity names, relevance scores etc
class EntityData {
    ay::CharPool d_namePool;
public:
    struct EntProp {
        std::string  canonicName;
        uint32_t     relevance;
        uint8_t      nameIsExplicit; // when != 0 means the name was hand set and needs to be honored more 

        EntProp() : relevance(0), nameIsExplicit(0) {}
        EntProp( const char* n, uint32_t r, uint8_t ne ) : canonicName(n), relevance(r), nameIsExplicit(ne) {}
        bool  is_nameExplicit() const { return (nameIsExplicit!=0); }
        void  set_nameExplicit() { nameIsExplicit = 1; }
    };
    typedef boost::unordered_map< StoredEntityUniqId, EntProp, StoredEntityUniqId_Hash > EntPropDtaMap;
private:
    EntPropDtaMap d_autocDtaMap;
public:
    /// if overrideName is false then if entity exists its name will only be altered if the new name is longer than the old one
    /// otherwise the exact name will be set
    EntProp*  setEntPropData( const StoredEntityUniqId& euid, const char* name, uint32_t rel, bool overrideName=false ) ;
    const EntProp* getEntPropData( const StoredEntityUniqId& euid ) const 
        { EntPropDtaMap::const_iterator i = d_autocDtaMap.find( euid ); return ( i==d_autocDtaMap.end()? 0 : &(i->second) ); }
    EntProp* getEntPropData( const StoredEntityUniqId& euid ) 
        { EntPropDtaMap::iterator i = d_autocDtaMap.find( euid ); return ( i==d_autocDtaMap.end()? 0 : &(i->second) ); }
    size_t readFromFile( GlobalPools& gp, const char* fname ) ;
};
} // namespace barzer
