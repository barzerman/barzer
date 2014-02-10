#pragma once

#include <stdint.h>
#include <iostream>

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
    { return (t.eclass.ec << 24) + (t.eclass.subclass << 16) + t.tokId; }

typedef StoredEntityUniqId BarzerEntity;

} // namespace barzer 
