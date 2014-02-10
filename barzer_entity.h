
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#pragma once 
#include <map>
#include <boost/unordered_map.hpp>
#include <ay/ay_logger.h>
#include <ay/ay_string_pool.h>
#include <ay/ay_util.h>
#include <barzer_entity_basic.h>

namespace barzer {

struct BENIFindResult {
	BarzerEntity ent;
	int popRank=0; // entity popularity rank
	double coverage = 0;
	double relevance = 0;
    size_t nameLen=0;

    BENIFindResult(const BarzerEntity& e ) : ent(e) {}
    BENIFindResult() = default;
    BENIFindResult( const BarzerEntity& e, const BENIFindResult& r ) :
        ent(e), 
        popRank(r.popRank),
        coverage(r.coverage),
        relevance(r.relevance),
        nameLen(r.nameLen)
    {}
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
typedef std::vector< StoredEntityClass > StoredEntityClassVec;
} // namespace barzer
