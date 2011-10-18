#ifndef BARZER_ENTITY_H 
#define BARZER_ENTITY_H 

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
{
	return (l.ec == r.ec && l.subclass == r.subclass );
}
inline bool operator<( const StoredEntityClass& l, const StoredEntityClass& r )
{
	if( l.ec < r.ec ) return true;
	else if( r.ec < l.ec ) return false;
	else return ( l.subclass < r.subclass );
}

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

	void filterEntList( BarzerEntityList& lst, const StoredEntityUniqId& euid ) const
	{
		for( EList::const_iterator i = d_lst.begin(); i!= d_lst.end(); ++i ) {
			if( euid.matchOther( *i ) ) 
				lst.d_lst.push_back( *i );
		}
	}
	std::ostream& print( std::ostream& fp ) const;
};


};

#endif // BARZER_ENTITY_H 
