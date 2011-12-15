#ifndef BARZER_ENTITY_H 
#define BARZER_ENTITY_H 
#include <map>
// #include <boost/unordered_map.hpp> 
#include <ay/ay_logger.h>

namespace barzer {

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

	void setClass( uint16_t c ) { eclass.setClass( c ); }
	void setSubclass( uint16_t c ) { eclass.setSubclass( c ); }
	void setTokenId( uint32_t i ) { tokId = i; }

	StoredEntityUniqId() : tokId(0xffffffff) {}
	StoredEntityUniqId(uint32_t tid, uint16_t cl, uint16_t sc ) : 
		tokId(tid),
		eclass(cl,sc)
	{}
	explicit StoredEntityUniqId( const StoredEntityClass& ec ) : 
		tokId(0xffffffff),
		eclass(ec)
	{}

	inline bool isTokIdValid() const { return (tokId!=0xffffffff); }
	inline bool isValid() const { return ( isTokIdValid() && eclass.isValid() ); }
	inline bool isValidForMatching() const { return ( eclass.ec ); }
	void print( std::ostream& fp ) const { fp << eclass << "," << tokId ; }
	
	bool matchOther( const StoredEntityUniqId& other ) const
		{ return( eclass.matchOther( other.eclass ) && ( !isTokIdValid() || tokId == other.tokId ) ); }

	bool matchOther( const StoredEntityUniqId& other, bool matchInvalid ) const
		{ return( eclass.matchOther( other.eclass ) && (tokId == other.tokId || !(matchInvalid || isTokIdValid()) ) ); }

    const StoredEntityClass& getClass()  const { return eclass; }
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
	return( l.tokId< r.tokId ?
		true :
		(r.tokId< l.tokId ? false: (l.eclass < r.eclass)) 
	);
		
}

typedef StoredEntityUniqId BarzerEntity;

//// generic type used by Barzel among other things
class BarzerEntityList {
public:
	typedef std::vector< BarzerEntity > EList;
private:
	EList d_lst;
	StoredEntityClass d_class;
	
public:
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
	const EList& getList() const 
		{ return d_lst; }

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
	std::ostream& print( std::ostream& fp ) const;
};

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
    return ( l.pathLen < r.pathLen ?
        true :
        l.relevance < r.relevance 
    );
}

class BestEntities {
    uint32_t d_maxEnt; 

public: 
    enum { DEFAULT_MAX_ENT = 128 };
    typedef std::multimap< BestEntities_EntWeight, StoredEntityUniqId > EntWeightMap;
    
private:
    EntWeightMap d_weightMap;
    typedef std::map< StoredEntityUniqId, EntWeightMap::iterator > EntIdMap;
    EntIdMap d_entMap;
public:
    BestEntities( uint32_t mxe = DEFAULT_MAX_ENT ) : d_maxEnt(mxe) {}
    void addEntity( const StoredEntityUniqId& euid, uint32_t pathLen, uint32_t relevance=0 )
    {
        EntIdMap::iterator i = d_entMap.find( euid );

        BestEntities_EntWeight wght( pathLen, relevance );
        if( i == d_entMap.end() ) { // this entity is not stored 
            EntWeightMap::iterator wi = d_weightMap.insert( EntWeightMap::value_type(wght, euid) );
            d_entMap[ euid ] = wi;
        } else {  // entity is already here
            if( wght < i->second->first )  { // if this occurence has higher weight 
                d_weightMap.erase( i->second );
                EntWeightMap::iterator wi = d_weightMap.insert( EntWeightMap::value_type(wght, euid) );
                i->second = wi;
            } else { // else we skip it
                return;
            }
            
        }
        if( d_weightMap.size() > d_maxEnt ) {
            EntWeightMap::iterator fwdIter = d_weightMap.end();
            --fwdIter;
            EntIdMap::iterator idMapIter = d_entMap.find( fwdIter->second );
            if( idMapIter->second != fwdIter ) { // should never happen
                AYLOG(ERROR) << "BestEntities inconsistency";
            } else {
                d_entMap.erase(idMapIter);
                d_weightMap.erase(fwdIter);
            }
        }
    }
    const EntWeightMap& getEntitiesAndWeights() const 
        { return d_weightMap; }
    void clear() { 
        d_entMap.clear();
        d_weightMap.clear();
    }
};
/// end of best entities 

};

#endif // BARZER_ENTITY_H 
