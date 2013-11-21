/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#pragma once
#include <stdint.h>
#include <barzer_el_pattern.h>
#include <barzer_entity.h>
#include <barzer_basic_types.h>
#include <barzer_basic_types_range.h>

namespace barzer {

/// this pattern is used to match BarzerEntityList as well as BarzerERC
class BTND_Pattern_Entity : public BTND_Pattern_Base {
	BarzerEntity d_ent;
	BarzerRange d_range;
	uint8_t     d_rangeIsValid; 
public:
	std::ostream& printXML( std::ostream& fp, const GlobalPools& ) const;
	std::ostream& print( std::ostream& fp, const BELPrintContext& ctxt ) const
	{
		d_ent.print( fp ) ; 
		if( d_rangeIsValid ) 
			fp << '[' << d_range << ']';
		return fp;
	}
	BTND_Pattern_Entity()  : d_rangeIsValid(0) {}

	void setRange() ;
	void setRange( const BarzerRange& r ) { if( d_rangeIsValid==0 ) d_rangeIsValid=1; d_range=r; }

	bool isRangeValid( ) const { return d_rangeIsValid; } 

	BarzerRange& getRange() { return d_range; }
	const BarzerRange& getRange() const { return d_range; }

	void setEntity( const BarzerEntity& e ) { d_ent=e; }

	void setEntityClass( const StoredEntityClass& ec ) { d_ent.setClass( ec ); }
	void setEntityClass( int c ) { d_ent.setClass( c ); }
	void setEntitySubclass( int c ) { d_ent.setSubclass( c ); }
	void setTokenId( uint32_t c ) { d_ent.setTokenId( c ); }

	const BarzerEntity& getEntity() const { return d_ent; }
	BarzerEntity& getEntity() { return d_ent; }

	bool operator() ( const BarzerERC& erc ) const {
		return ( d_ent.matchOther( erc.getEntity() ) );
	}
	bool operator() ( const BarzerEntity& ent ) const { return ( d_ent.matchOther( ent ) ); }

	bool operator() ( const BarzerEntityList& elist ) const { 
		const BarzerEntityList::EList& lst = elist.getList();
		for( BarzerEntityList::EList::const_iterator i = lst.begin(); i!= lst.end(); ++i ) {
			if( (*this)(*i) )
				return true;
		}
		return false;
	}
	bool lessThan( const BTND_Pattern_Entity& r ) const 
	{
		return ay::range_comp().less_than(
			d_ent, d_range, d_rangeIsValid, 
			r.d_ent, r.d_range, r.d_rangeIsValid
		 );
	}
};
inline bool operator< ( const BTND_Pattern_Entity& l, const BTND_Pattern_Entity& r ) 
{
	return l.lessThan( r );
}


class BTND_Pattern_ERC: public BTND_Pattern_Base {
	BarzerERC d_erc;

	uint8_t d_matchBlankRange, d_matchBlankEntity;
public:
	BTND_Pattern_ERC() : d_matchBlankRange(0), d_matchBlankEntity(0) {}	

	bool isRangeValid() const { return d_erc.getRange().isValid(); }
	bool isEntityValid() const { return d_erc.getEntity().isValidForMatching(); }
	bool isUnitEntityValid() const { return d_erc.getUnitEntity().isValidForMatching(); }

	bool isMatchBlankRange() const { return d_matchBlankRange; }
	bool isMatchBlankEntity() const { return d_matchBlankEntity; }

	
	void setMatchBlankRange() { d_matchBlankRange= 1; }
	void setMatchBlankEntity() { d_matchBlankEntity = 1; }

	const BarzerERC& getERC() const { return d_erc; }
	BarzerERC& getERC() { return d_erc; }
	bool operator()( const BarzerERC& e) const 
		{ 
			return d_erc.matchOtherWithBlanks(e, isMatchBlankRange(), isMatchBlankEntity() );
		} 
	
	std::ostream& printXML( std::ostream& fp, const GlobalPools&  ) const ;
	std::ostream& print( std::ostream& fp ) const 
	{ 
		d_erc.print(fp);
		if( !d_erc.getRange().isBlank() ) {
			fp << " range type " << d_erc.getRange().getType() << "\n";
		}
		return fp;
	}
	bool lessThan( const BTND_Pattern_ERC& r) const
		{ 
			if(  d_erc.lessThan(r.getERC()) )
				return true;
			else if( r.getERC().lessThan(d_erc) ) 
				return false;
			else
				return (ay::range_comp().less_than(
					d_matchBlankRange, d_matchBlankEntity,
					r.d_matchBlankRange, r.d_matchBlankEntity
				) );
		}
};

inline bool operator < ( const BTND_Pattern_ERC& l, const BTND_Pattern_ERC& r ) 
{ return l.lessThan(r); }
inline std::ostream& operator <<( std::ostream& fp, const BTND_Pattern_ERC& e ) 
{ return e.print(fp); }

/// ERC Expression
class BTND_Pattern_ERCExpr : public BTND_Pattern_Base {
	uint16_t d_exprType, d_exprEclass;
public:
	BTND_Pattern_ERCExpr() : d_exprType(BarzerERCExpr::T_LOGIC_AND),d_exprEclass(0) {} 
	

	void setExprType(uint16_t t) { d_exprType= t; }
	uint16_t getExprType() const { return d_exprType; }
	uint16_t getExprSubtype() const { return d_exprEclass; }
	void setEclass(uint16_t t) { d_exprEclass=t; }

	bool operator()( const BarzerERCExpr& e) const { return ( d_exprEclass== BarzerERCExpr::EC_ARBITRARY || d_exprEclass == e.getEclass() ); } 
	bool lessThan( const BTND_Pattern_ERCExpr& r ) const {
		return (d_exprEclass < r.d_exprEclass);

	}
	std::ostream& printXML( std::ostream& fp, const GlobalPools&  ) const ;
	std::ostream& print( std::ostream& fp ) const 
		{ return (fp << d_exprEclass << ":" << d_exprType ); }
};
inline bool operator<( const BTND_Pattern_ERCExpr& l, const BTND_Pattern_ERCExpr& r ) 
{ return l.lessThan(r); }

/// Entity Attachement pattern is similar to ERC - there's an entity with a list of Beads and or things
struct BTND_Pattern_EVR : public BTND_Pattern_Base {
	BarzerEntity d_ent;

    bool isLessThan( const BTND_Pattern_EVR& o ) const { return ( d_ent < o.d_ent ); }
    bool isEqualTo( const BTND_Pattern_EVR& o )  const { return ( d_ent == o.d_ent ); }
    bool operator()( const BarzerEVR& evr ) const
        { return d_ent.matchOther( evr.getEntity() ); }
};

inline std::ostream& operator <<( std::ostream& fp, const BTND_Pattern_EVR& x )
	{ return( fp << x.d_ent ); }
inline bool operator <( const BTND_Pattern_EVR& l, const BTND_Pattern_EVR& r ) { return l.isLessThan( r ); }
inline bool operator ==( const BTND_Pattern_EVR& l, const BTND_Pattern_EVR& r ) { return l.isLessThan( r ); }

} // namespace barzer
