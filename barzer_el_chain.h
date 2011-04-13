#ifndef BARZER_EL_CHAIN_H  
#define BARZER_EL_CHAIN_H  

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
	BarzelEntityRangeCombo
> BarzelBeadAtomic;

/// this is a tree type. beads of this type do not 
/// for future implementation
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
};
typedef boost::variant <
	BarzelBeadBlank,
	BarzelBeadAtomic,
	BarzelBeadExpression
> BarzelBeadData;

//// Barzel bead starts as a CToken 
/// Barzel matcher matches the beads sequences and rewrites until no rewrites were made
///  Barzel bead ma be a constant or an expression 
class BarzelBead {
	CTWPVec ctokOrigVec; // pre-barzel CToken list participating in this bead 
	size_t tokenSpan;   // full token span (including non-participating beads)
	size_t tokenCount;   // token count of everything participating in this bead
	/// types 
	BarzelBeadData dta;
	
public:
	BarzelBead() : tokenSpan(0), tokenCount(0) {}
	BarzelBead(const CToken&) ;
	/// implement:
	//// - constructor from CToken (initialization)
	//// - bead absorption (folding a bead into this one)
	//// - templated initializer from types participating in BarzelBeadData 
	
}; 

struct BarzelBeadChain {
	std::list< BarzelBead > 	BeadList;

	/// implement 
	/// - fold ( BeadList::iterator from, to )
	/// - externally Matcher will provide these iterator ranges for fold 
};




}

#endif // BARZER_EL_CHAIN_H
