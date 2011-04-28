#ifndef BARZER_EL_CHAIN_H  
#define BARZER_EL_CHAIN_H  

#include <barzer_storage_types.h>
#include <barzer_parse_types.h>
#include <barzer_el_btnd.h>
#include <list>

/// Barzel chain is the sequence manipulated during rewriting 
/// Barzer gets a chain of CToken-s (see barzer_parse_types.h) converts it into 
/// the BarzelChain (a doubly linked list of BarzelBead-s)
namespace barzer {

struct BarzelBeadBlank {
	int dummy;
	BarzelBeadBlank() : dummy(0) {}

	std::ostream& print( std::ostream& fp ) const
		 { return ( fp << "<blankbead>" ); }
};

/// combination 
struct BarzelEntityRangeCombo {
	BarzerEntityList ent;
	BarzerRange      range;
	
	std::ostream& print( std::ostream& fp ) const
	{
		return ( ent.print( range.print( fp )<<"(" ) << ")" );
	}
};

typedef boost::variant<
	BarzerLiteral, // constant string literal
	BarzerString, // non constant string (something constructed from the input) 
	BarzerNumber,
	BarzerDate,
	BarzerTimeOfDay,
	BarzerRange,
	BarzerEntityList,
	BarzelEntityRangeCombo
> BarzelBeadAtomic_var;
enum {
	BarzerLiteral_TYPE, 
	BarzerString_TYPE,
	BarzerNumber_TYPE,
	BarzerDate_TYPE,
	BarzerTimeOfDay_TYPE,
	BarzerRange_TYPE,
	BarzerEntityList_TYPE,
	BarzelEntityRangeCombo_TYPE
};
struct BarzelBeadAtomic {
	BarzelBeadAtomic_var dta;
	
	const BarzerLiteral* getLiteral() const { return boost::get<BarzerLiteral>( &dta ); }

	bool isLiteral() const { return dta.which() == BarzerLiteral_TYPE; }
	bool isStopLiteral() const { 
		const BarzerLiteral* bl = getLiteral();
		return( bl && bl->isStop() );
	}
	bool isNumber() const { return dta.which() == BarzerNumber_TYPE; }
	bool isBlankLiteral() const { 
		const BarzerLiteral* bl = getLiteral();
		return( bl && bl->isBlank() );
	}
	int getType() const 
		{ return dta.which(); }

	BarzelBeadAtomic& setStopLiteral( )
		{ BarzerLiteral lrl; lrl.setStop(); dta = lrl; return *this; }

	template <typename T> BarzelBeadAtomic& setData( const T& t ) 
	{ dta = t; return *this; }
	std::ostream& print( std::ostream& fp ) const;
};

/// this is a tree type. beads of this type are expresions on top of whatever 
/// was discerned from the input
/// for future implementation - 
// 
struct BarzelBeadExpression {
	struct Data  {
	} dta;
	
	typedef boost::variant< 
		BarzelBeadAtomic,
		BarzelBeadExpression
	> SubExpr;

	typedef std::list< SubExpr >  SubExprList;

	SubExprList child;
	std::ostream& print( std::ostream& fp ) const 
	{
		return ( fp << "<expression>" );
	}
};
typedef boost::variant <
	BarzelBeadBlank,
	BarzelBeadAtomic,
	BarzelBeadExpression
> BarzelBeadData;

enum {
	BarzelBeadBlank_TYPE,
	BarzelBeadAtomic_TYPE,
	BarzelBeadExpression_TYPE
};

/// this class when implemented will keep matching trace information structured as a tree 
/// to reflect the matching process
struct BarzelBeadTraceInfo {
};
//// Barzel bead starts as a CToken 
/// Barzel matcher matches the beads sequences and rewrites until no rewrites were made
///  Barzel bead ma be a constant or an expression 
class BarzelBead {
	CTWPVec ctokOrigVec; // pre-barzel CToken list participating in this bead 
	/// types 
	BarzelBeadData dta;
public:
	BarzelBead() {}
	void init(const CTWPVec::value_type&) ;
	BarzelBead(const CTWPVec::value_type& ct) 
		{ init(ct); }
	/// implement:
	void absorbBead( const BarzelBead& bead )
	{ ctokOrigVec.insert( ctokOrigVec.end(), bead.ctokOrigVec.begin(), bead.ctokOrigVec.end() ); }

	template <typename T> void become( const T& t ) { dta = t; }

	std::ostream& print( std::ostream& ) const;

	template <typename T> void setData( const T& t ) { dta = t; }
	void setStopLiteral() { 
		dta= BarzelBeadAtomic().setStopLiteral();
	}

	bool isBlank( ) const { return (dta.which() == BarzelBeadBlank_TYPE); }
	bool isAtomic() const { return (dta.which() == BarzelBeadAtomic_TYPE); }
	bool isExpression() const { return (dta.which() == BarzelBeadExpression_TYPE); }

	const BarzelBeadAtomic* getAtomic() const { return  boost::get<BarzelBeadAtomic>( &dta ); }
	const BarzelBeadExpression* getExpression() const { return  boost::get<BarzelBeadExpression>( &dta ); }

	size_t getFullNumTokens() const
	{
		size_t n = 1;
		bool wasBlank = false;
		for( CTWPVec::const_iterator i = ctokOrigVec.begin(); i!= ctokOrigVec.end(); ++i ) {
			if( i->first.isBlank() ) {
				if( !wasBlank ) {
					++n;
				}
			} else {
				wasBlank = false;
			}
		}
		return ( wasBlank ? n-1: n );
	}
}; 

typedef std::list< BarzelBead > 	BeadList;

struct BarzelBeadChain {
	
	typedef std::pair< BeadList::iterator, BeadList::iterator > Range;
	
	BeadList lst;
	/// implement 
	/// - fold ( BeadList::iterator from, to )
	/// - externally Matcher will provide these iterator ranges for fold 
	
	BeadList::iterator getLstBegin() { return lst.begin(); }
	BeadList::const_iterator getLstBegin() const { return lst.begin(); }

	BeadList::const_iterator getLstEnd() const { return lst.end(); }
	BeadList::iterator getLstEnd() { return lst.end(); }

	bool isIterEnd( BeadList::iterator i ) const { return i == lst.end(); }
	bool isIterEnd( BeadList::const_iterator i ) const { return i == lst.end(); }

	bool isIterNotEnd( BeadList::iterator i ) const { return i != lst.end(); }
	bool isIterNotEnd( BeadList::const_iterator i ) const { return i != lst.end(); }

	void init( const CTWPVec& cv );
	void clear() { lst.clear(); }

	const Range getFullRange() 
		{ return Range( lst.begin(), lst.end() ); }
	void collapseRangeLeft( Range r );
};

std::ostream& operator <<( std::ostream& fp, const BarzelBeadChain::Range& rng ) ;
}
#endif // BARZER_EL_CHAIN_H
