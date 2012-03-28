#ifndef BARZER_EL_CHAIN_H  
#define BARZER_EL_CHAIN_H  

#include <barzer_storage_types.h>
#include <barzer_parse_types.h>
#include <barzer_basic_types.h>
#include <ay/ay_logger.h>
#include <ay/ay_util.h>
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
inline bool operator< ( const BarzelBeadBlank& l, const BarzelBeadBlank& r ) 
{ return false; }

/// combination 
typedef boost::variant<
	BarzerLiteral, // constant string literal
	BarzerString, // non constant string (something constructed from the input) 
	BarzerNumber,
	BarzerDate,
	BarzerTimeOfDay,
	BarzerDateTime,
	BarzerRange,
	BarzerEntityList,
	BarzerEntity,
	BarzerEntityRangeCombo,
	BarzerERCExpr
> BarzelBeadAtomic_var;
enum {
	BarzerLiteral_TYPE, 
	BarzerString_TYPE,
	BarzerNumber_TYPE,
	BarzerDate_TYPE,
	BarzerTimeOfDay_TYPE,
	BarzerDateTime_TYPE,
	BarzerRange_TYPE,
	BarzerEntityList_TYPE,
	BarzerEntity_TYPE,
	BarzerEntityRangeCombo_TYPE,
	BarzerERCExpr_TYPE
};
struct BarzelBeadAtomic {
	BarzelBeadAtomic_var dta;
	
	BarzelBeadAtomic() {}
	BarzelBeadAtomic(const BarzelBeadAtomic_var &d) : dta(d) {}

	const BarzerLiteral* getLiteral() const { return boost::get<BarzerLiteral>( &dta ); }

	BarzerEntity* getEntity() { return boost::get<BarzerEntity>( &dta ); }
	const BarzerEntity* getEntity() const { return boost::get<BarzerEntity>( &dta ); }
	const BarzerEntityList* getEntityList() const { return boost::get<BarzerEntityList>( &dta ); }
	BarzerEntityList* getEntityList() { return boost::get<BarzerEntityList>( &dta ); }

    template <typename T> const T* get() const { return boost::get<T>( &dta ); }
    template <typename T> T* get() { return boost::get<T>( &dta ); }

	const BarzerNumber& getNumber() const { return boost::get<BarzerNumber>(dta); }

	bool isEntity() const { return dta.which() == BarzerEntity_TYPE; }
	bool isLiteral() const { return dta.which() == BarzerLiteral_TYPE; }
	bool isStopLiteral() const { 
		const BarzerLiteral* bl = getLiteral();
		return( bl && bl->isStop() );
	}
	bool isDate() const { return dta.which() == BarzerDate_TYPE; }
	bool isNumber() const { return dta.which() == BarzerNumber_TYPE; }
	bool isBlankLiteral() const { 
		const BarzerLiteral* bl = getLiteral();
		return( bl && bl->isBlank() );
	}
	bool isString() const {  return (dta.which() == BarzerString_TYPE); }

	bool isStringLiteral() const { 
		const BarzerLiteral* bl = getLiteral();
		return( bl && bl->isString() );
	}
	int getType() const 
		{ return dta.which(); }

	BarzelBeadAtomic& setStopLiteral( )
		{ BarzerLiteral lrl; lrl.setStop(); dta = lrl; return *this; }

	const BarzelBeadAtomic_var& getData() const { return dta; }
	      BarzelBeadAtomic_var& getData()       { return dta; }

	template <typename T> BarzelBeadAtomic& setData( const T& t ) 
	{ dta = t; return *this; }
	std::ostream& print( std::ostream& fp ) const;
};

/// this is a tree type. beads of this type are expresions on top of whatever 
/// was discerned from the input
/// for future implementation - 
// 
struct BarzelBeadExpression {
	typedef std::pair<uint32_t, uint32_t> Attr;
	typedef std::vector<Attr> AttrList;

	//struct Data {} dta;
	enum { ATTRLIST = INVALID_STORED_ID };

	uint32_t sid;
	AttrList attrs;

	
	typedef boost::variant< 
		BarzelBeadAtomic,
		BarzelBeadExpression
	> SubExpr;

	typedef std::list< SubExpr >  SubExprList;

	SubExprList child;

	//const Data& getData() const { return dta; }
	uint32_t getSid() const { return sid; }
	void setSid(uint32_t s) { sid = s; }
	const AttrList& getAttrs() const { return attrs; }
	AttrList& getAttrs() { return attrs; }

	void addAttribute(uint32_t k, uint32_t v);

	const SubExprList& getChildren() const { return child; }
	SubExprList& getChildren()  { return child; }

	void addChild(const BarzelBeadExpression&);
	void addChild(const SubExpr&);

	/*
	void addChild(const BarzelBeadAtomic&);
	void addChild(const BarzelBeadExpression&);
	*/

	std::ostream& print( std::ostream& fp ) const 
	{
		return ( fp << "<expression>" );
	}
};
inline bool operator< ( const BarzelBeadExpression& l, const BarzelBeadExpression& r )
{
	return false;
}

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

inline bool is_BarzelBeadAtomic( const BarzelBeadData& dta ) { return dta.which() == BarzelBeadAtomic_TYPE; }

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
    int d_unmatchable; 
    uint32_t d_stemStringId;
public:
    uint32_t    getStemStringId() const { return d_stemStringId; }
    void        setStemStringId( uint32_t i ) { d_stemStringId= i; }

    int getBeadUnmatchability() const { return d_unmatchable; } 
    void setBeadUnmatchability(int bu=0) { d_unmatchable= bu; } 


	const BarzelBeadData& getBeadData() const { return dta; }
	void init(const CTWPVec::value_type&) ;
    void initFromCTok( const CToken& ct );
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

	BarzelBeadAtomic* getAtomic() { return  boost::get<BarzelBeadAtomic>( &dta ); }
	const BarzelBeadAtomic* getAtomic() const { return  boost::get<BarzelBeadAtomic>( &dta ); }
	const BarzelBeadExpression* getExpression() const { return  boost::get<BarzelBeadExpression>( &dta ); }

	bool isStringLiteralOrString() const { 
		const BarzelBeadAtomic* atomic = getAtomic();
        return( atomic && (atomic->isStringLiteral() || atomic->isString()) );
    }
    const BarzerLiteral* getLiteral() const {
        const BarzelBeadAtomic* atomic = getAtomic();
        if( atomic ) {
		    return atomic->getLiteral();
        } else 
            return 0;
    }
	bool isStringLiteral() const { 
		const BarzelBeadAtomic* atomic = getAtomic();
		return( atomic && atomic->isStringLiteral() )  ;
	}
    const BarzerEntityList* getEntityList() const { 
			const BarzelBeadAtomic* atomic = getAtomic();
            return( atomic ? atomic->getEntityList() : 0 ); 
    }
    BarzerEntityList* getEntityList() { 
			BarzelBeadAtomic* atomic = getAtomic();
            return( atomic ? atomic->getEntityList() : 0 ); 
    }
    BarzerEntityList* becomeEntityList() 
        { 
            dta= BarzelBeadAtomic( BarzerEntityList() );
            return getEntityList();
        }
	bool isBlankLiteral() const { 
			const BarzelBeadAtomic* atomic = getAtomic();
			return( atomic && atomic->isBlankLiteral() )  ;
		}
	bool isEntity(StoredEntityClass& ec) const { 
        const BarzelBeadAtomic* atomic = getAtomic();
        if( atomic ) {
            const BarzerEntity* ent = atomic->getEntity();
            return ( ent ? ( ec=ent->eclass, true): false );
        } else
            return false;
    }

    template <typename T>
    T* get() {
        BarzelBeadAtomic* atomic = getAtomic();
        if( atomic ) { T* t = atomic->get<T>(); return t; } else return 0;
    }
    template <typename T>
    const T* get() const {
        const BarzelBeadAtomic* atomic = getAtomic();
        if( atomic ) { const T* t = atomic->get<T>(); return t; } else return 0;
    }


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
	/// it's a loosely defined property - semantical bead is anything less than trivial 
	/// meaning it's either non-atomic, non-number, fluff if string - meaning something prcoessed
	/// by barzer
	bool isSemantical() const {
		if( isBlank() ) return false;
		// not blank
		const BarzelBeadAtomic* atomic = getAtomic();
		if( !atomic ) return true; // non atomic and non blank - must be semantical
		const BarzerLiteral* ltrl = atomic->getLiteral(); 
		if( ltrl ) return ltrl->isStop();
		// not literal at this point
		if( atomic->isString() ) return false;
		if( atomic->isNumber() ) return false;

		return true;
	}
	const CTWPVec& getCTokens() const 
		{ return ctokOrigVec; }
	CTWPVec& getCTokens() 
		{ return ctokOrigVec; }

	BarzelBead(const BarzelBeadData& d ): dta(d), d_unmatchable(0) {}
	BarzelBead() : d_unmatchable(0){}
    BarzelBead( const BarzerEntity& e ) : dta(BarzelBeadAtomic(e)), d_unmatchable(0) {}
    BarzelBead( const BarzerEntityList& e ) : dta(BarzelBeadAtomic(e)), d_unmatchable(0) {}
	BarzelBead(const CTWPVec::value_type& ct) : d_unmatchable(0) { init(ct); }
}; 

typedef std::list< BarzelBead > 	BeadList;

struct BarzelBeadChain {
	typedef std::pair< BeadList::iterator, BeadList::iterator > Range;

	static inline bool 	iteratorAtBlank( BeadList::iterator i ) 
	{
		const BarzelBeadAtomic* atomic = i->getAtomic();
		return( atomic && atomic->isBlankLiteral() );
	}
	static inline void 	trimBlanksFromRange( Range& rng ) 
	{
		while( rng.first != rng.second ) {
			BeadList::iterator lastElem = rng.second;
			--lastElem;
			if( lastElem == rng.first ) 
				break;
			if( BarzelBeadChain::iteratorAtBlank(lastElem) ) 
				rng.second= lastElem;
			else if( BarzelBeadChain::iteratorAtBlank(rng.first) ) 
				++rng.first;
			else
				break;
		}
	}

	
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

    BeadList& getList() { return lst; }
    const BeadList& getList() const { return lst; }

	void init( const CTWPVec& cv );
	void clear() { lst.clear(); }
	
	BeadList::iterator insertBead( BeadList::iterator at, const BarzelBeadData& d ) 
	{
		return lst.insert( at, d );
	}
	const Range getFullRange() 
		{ return Range( lst.begin(), lst.end() ); }
	void collapseRangeLeft( Range r );
	void collapseRangeLeft( BeadList::iterator origin, Range r );
	
	/// will try to print rng.first through rng.second including second, unless it equals end
	static Range makeInclusiveRange( const Range& rng, BeadList::const_iterator end ) 
	{
		Range r = rng;
		if( r.second != end ) 
			++(r.second);

		return r;
	}
    /// runs through the list and sets unmathabilities for all beads to 0  
    void clearUnmatchable() {
		for( BeadList::iterator i = lst.begin(); i!= lst.end(); ++i ) 
            i->setBeadUnmatchability();
    }
    
    template <typename T>
    T* appendBlankAtomicVal( ) { 
        lst.push_back( BarzelBead(T()) );
        return lst.back().get<T>();
    }

    void adjustStemIds( const StoredUniverse& u, const BELTrie& trie );
};


typedef BarzelBeadChain::Range BarzelBeadRange;

std::ostream& operator <<( std::ostream& fp, const BarzelBeadChain::Range& rng ) ;


//bool operator==(const BarzelBeadData &left, const BarzelBeadData &right);

struct BarzelBeadData_EQ : public boost::static_visitor<bool> {
	bool operator()(const BarzerEntityList &left, const BarzerEntityList &right) const
	{
		return false;
	}
	bool operator()(const BarzerString &left, const BarzerString &right) const
	    { return left.getStr() == right.getStr(); }
	bool operator()(const BarzelBeadAtomic &left, const BarzelBeadAtomic &right) const
		{ return boost::apply_visitor(*this, left.getData(), right.getData()); }
	template<class T> bool operator()(const T &l, const T &r) const
		{ return( l ==r ); }
		// { return ay::bcs::equal(left, right); }
	template<class T, class U> bool operator()(const T&, const U&) const { return false; }
};


inline bool beadsEqual(const BarzelBeadData &left, const BarzelBeadData &right)
{
	return boost::apply_visitor(BarzelBeadData_EQ(), left, right);
}
inline bool operator==(const BarzelBeadData &left, const BarzelBeadData &right)
{
	return beadsEqual( left, right );
}





}
#endif // BARZER_EL_CHAIN_H
