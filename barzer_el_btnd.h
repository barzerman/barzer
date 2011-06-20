#ifndef BARZER_EL_BTND_H
#define BARZER_EL_BTND_H

#include <stdint.h>
#include <ay/ay_bitflags.h>
#include <ay/ay_string_pool.h>
#include <ay/ay_util.h>
#include <boost/variant.hpp>
#include <barzer_basic_types.h>
#include <barzer_el_variable.h>

// types defined in this file are used to store information for various node types 
// in the BarzEL trie
namespace barzer {
struct BELPrintContext;

//// BarzelTrieNodeData - PATTERN types 
/// all pattern classes must inherit from this 
struct BTND_Pattern_Base {
/// match operator 
template <typename T> bool operator()( const T& t ) const { return false; }
};
/// simple number wildcard data 
struct BTND_Pattern_Number : public BTND_Pattern_Base {
	std::ostream& print( std::ostream&, const BELPrintContext& ) const;
	enum {
		T_ANY_INT, // any integer 
		T_ANY_REAL, // any real

		T_RANGE_INT, // range is always inclusive as in <= <=
		T_RANGE_REAL, // same as above - always inclusive
		T_ANY_NUMBER, // any number at all real or integer

		T_MAX
	};

	uint8_t type; // one of T_XXXX 
	// for EXACT_XXX types only lo is used
	union {
		struct { float lo,hi; } real;
		struct { int lo,hi; } integer;
	} range;

	inline bool isLessThan( const BTND_Pattern_Number& r ) const
	{
		if( type < r.type ) 
			return true;
		else if( r.type < type ) 
			return false;
		else {
			switch( type ) {
			case T_ANY_NUMBER: return false;
			case T_ANY_INT: return false;
			case T_ANY_REAL: return false;
			case T_RANGE_INT: 
				return (ay::range_comp().less_than(
						range.integer.lo, range.integer.hi,
						r.range.integer.lo, r.range.integer.hi
					));
			case T_RANGE_REAL: 
				return ay::range_comp().less_than(
						range.real.lo, range.real.hi,
						r.range.real.lo, r.range.real.hi
					);
			default:
				return false;
			}
		}
	}

	bool isReal() const 
		{ return (type == T_ANY_REAL || type == T_RANGE_REAL); }

	BTND_Pattern_Number() : 
		type(T_ANY_INT)
	{ range.integer.lo = range.integer.hi = 0; }

	void setAnyNumber() { type = T_ANY_NUMBER; }
	void setAnyInt() { type = T_ANY_INT; }
	void setAnyReal() { type = T_ANY_REAL; }

	int getType() const { return type; }
	void setIntRange( int lo, int hi )
	{
		type = T_RANGE_INT;
		range.integer.lo = lo;
		range.integer.hi = hi;
	}
	void setRealRange( float lo, float hi )
	{
		type = T_RANGE_REAL;
		range.real.lo = lo;
		range.real.hi = hi;
	}
	std::ostream& printRange( std::ostream& fp ) const 
	{
		switch( type ) {
		case T_RANGE_REAL:
			return (fp << range.real.lo << "," << range.real.hi);
		case T_RANGE_INT:
			return (fp << range.integer.lo << "," << range.integer.hi);
		}
		return fp;
	}
	bool operator()( const BarzerNumber& num ) const;
};
inline bool operator <( const BTND_Pattern_Number& l, const BTND_Pattern_Number& r )
	{ return l.isLessThan( r ); }
inline std::ostream& operator <<( std::ostream& fp, const BTND_Pattern_Number& x )
{
	return (x.printRange( fp << "NUM[" ) << "]");
}

// Punctuation and Stop Tokens (theChar is 0 for stops) 
struct BTND_Pattern_Punct : public BTND_Pattern_Base {
	std::ostream& print( std::ostream&, const BELPrintContext& ) const;
	int theChar; // actual punctuation character. 0 - same as stop
	
	BTND_Pattern_Punct() : theChar(0xffffffff) {}
	BTND_Pattern_Punct(char c) : theChar(c) {}

	void setChar( char c ) { theChar = c; }
};
inline std::ostream& operator <<( std::ostream& fp, const BTND_Pattern_Punct& x )
	{ return( fp << "'" << std::hex << x.theChar << "'" ); }

// simple token wildcard data
struct BTND_Pattern_Wildcard {
	std::ostream& print( std::ostream&, const BELPrintContext& ) const;
	uint8_t minTerms, maxTerms;

	size_t getMinTokSpan() const { return minTerms; }
	size_t getMaxTokSpan() const { return maxTerms; }

	BTND_Pattern_Wildcard() : minTerms(0), maxTerms(0) {}
	BTND_Pattern_Wildcard(uint8_t mn, uint8_t mx ) : minTerms(mn), maxTerms(mx) {}

	bool isLessThan( const BTND_Pattern_Wildcard& r ) const
	{
		return( ay::range_comp().less_than( minTerms, maxTerms, r.minTerms, r.maxTerms ) );
	}
};
inline std::ostream& operator <<( std::ostream& fp, const BTND_Pattern_Wildcard& x )
	{ return( fp << "*[" << x.minTerms << "," << x.maxTerms << "]" ); }
inline bool operator <( const BTND_Pattern_Wildcard& l, const BTND_Pattern_Wildcard& r )
	{ return l.isLessThan( r ); }

// date wildcard data
struct BTND_Pattern_Date : public BTND_Pattern_Base {
	std::ostream& print( std::ostream&, const BELPrintContext& ) const;
	enum {
		T_ANY_DATE, 
		T_ANY_FUTUREDATE, 
		T_ANY_PASTDATE, 
		T_TODAY, 

		T_DATERANGE, // range of dates
		T_MAX
	};
	uint8_t type; // T_XXXX values 
	enum {
		MAX_LONG_DATE = 100000000,
		MIN_LONG_DATE = -100000000
	};

	int32_t lo, hi; // low high date in YYYYMMDD format
	
	BTND_Pattern_Date( ) : type(T_ANY_DATE), lo(MIN_LONG_DATE),hi(MAX_LONG_DATE) {}

	// argument is in the form of the long number 
	// fo AD YYYYMMDD for BC -YYYYMMDD
	bool isDateValid( int32_t dt, int32_t today ) const {
		switch( type ) {
		case T_ANY_DATE: return true;
		case T_ANY_FUTUREDATE: return ( dt> today );
		case T_ANY_PASTDATE:   return ( dt< today );
		case T_TODAY:   return ( dt== today );

		case T_DATERANGE: return ( dt>= lo && dt <=hi );
		default: 
			return false;
		}
	}
	void setHi( int32_t d ) { hi = d; }
	void setLo( int32_t d ) { lo = d; }
	void setFuture( ) { lo= MIN_LONG_DATE; hi= MAX_LONG_DATE; type= T_ANY_FUTUREDATE; }
	void setPast( ) { lo= MIN_LONG_DATE; hi= MAX_LONG_DATE; type= T_ANY_PASTDATE; }
	void setAny( ) { lo= MIN_LONG_DATE; hi= MAX_LONG_DATE; type= T_ANY_DATE; }

	bool operator()( const BarzerDate& dt ) const
		{ return isDateValid( dt.getLongDate(), dt.longToday ); }

	bool isLessThan( const BTND_Pattern_Date& r ) const
		{ return ay::range_comp().less_than( type, lo, hi, r.type, r.lo, r.hi ); }
};
inline std::ostream& operator <<( std::ostream& fp, const BTND_Pattern_Date& x )
	{ return( fp << "date." << x.type << "[" << x.lo << "," << x.hi << "]" ); }

inline bool operator <( const BTND_Pattern_Date& l, const BTND_Pattern_Date& r )
	{ return l.isLessThan( r ); }

// time wildcard data. represents time of day 
// seconds since midnight
struct BTND_Pattern_Time : public BTND_Pattern_Base {
	std::ostream& print( std::ostream&, const BELPrintContext& ) const;
	enum {
		T_ANY_TIME,
		T_ANY_FUTURE_TIME,
		T_ANY_PAST_TIME,
		T_TIMERANGE
	};
	uint8_t type; // T_XXXX values 
	BarzerTimeOfDay lo, hi; // seconds since midnight

	BTND_Pattern_Time( ) : type(T_ANY_TIME), lo(0), hi(BarzerTimeOfDay::MAX_TIMEOFDAY) {}

	void setLo( int x ) { lo.setLong( x ); }
	void setHi( int x ) { hi.setLong( x ); }
	bool isLessThan( const BTND_Pattern_Time& r ) const
		{ return ay::range_comp().less_than( type,lo, hi, r.type, r.lo, r.hi ); }
};
inline std::ostream& operator <<( std::ostream& fp, const BTND_Pattern_Time& x )
	{ return( fp << "time." << x.type << "[" << x.lo << "," << x.hi << "]" ); }
inline bool operator <( const BTND_Pattern_Time& l, const BTND_Pattern_Time& r )
{
	return l.isLessThan( r );
}

struct BTND_Pattern_DateTime : public BTND_Pattern_Base {
	std::ostream& print( std::ostream&, const BELPrintContext& ) const;
	enum {
		T_ANY_DATETIME, 
		T_ANY_FUTURE_DATETIME, 
		T_ANY_PAST_DATETIME, 
		T_DATETIME_RANGE,
		T_MAX
	};
	
	uint8_t type; // T_XXXX values 
	BarzerDateTime lo, hi;

	BTND_Pattern_DateTime() : type(T_ANY_DATETIME) {
		lo.setMin();
		hi.setMax();
	}
	
	void setFuture() { type = T_ANY_FUTURE_DATETIME; }
	void setPast() { type = T_ANY_PAST_DATETIME; }

	void setRange() { type = T_DATETIME_RANGE; lo.setMin(); hi.setMax(); }

	void setRange( const BarzerDateTime& l, const BarzerDateTime& h  ) { type = T_DATETIME_RANGE; lo = l; hi = h; }
	void setRangeAbove( const BarzerDateTime& d ) { type = T_DATETIME_RANGE; lo = d; hi.setMax(); }
	void setRangeBelow( const BarzerDateTime& d ) { type = T_DATETIME_RANGE; lo.setMin(); hi = d; }

	void setLoDate( int x ) { if( type != T_DATETIME_RANGE ) type = T_DATETIME_RANGE; lo.setDate( x ); }
	void setLoTime( int x ) { if( type != T_DATETIME_RANGE ) type = T_DATETIME_RANGE; lo.setTime( x ); }
	void setHiDate( int x ) { if( type != T_DATETIME_RANGE ) type = T_DATETIME_RANGE; hi.setDate( x ); }
	void setHiTime( int x ) { if( type != T_DATETIME_RANGE ) type = T_DATETIME_RANGE; hi.setTime( x ); }

	bool isLessThan( const BTND_Pattern_DateTime& r ) const
	{
		if( type < r.type  ) 
			return true;
		else if( r.type < type ) 
			return false;

		if( type == T_DATETIME_RANGE ) 
			return (
				ay::range_comp().less_than(
						lo, hi,
						r.lo, r.hi
				)
			);
		else 
			return false;
	}

	bool operator()( const BarzerDateTime& d ) const {
		if( lo.isValid() && !( lo < d) ) 
			return false;
		if( hi.isValid() && !( d< hi ) ) 
			return false;
		return true;
	}
};
inline std::ostream& operator <<( std::ostream& fp, const BTND_Pattern_DateTime& x )
	{ return( fp << "datetime." << x.type << '[' << x.lo << '-' << x.hi <<']' ); } 

inline bool operator <( const BTND_Pattern_DateTime& l, const BTND_Pattern_DateTime& r )
	{ return l.isLessThan( r ); }

struct BTND_Pattern_CompoundedWord : public BTND_Pattern_Base {
	std::ostream& print( std::ostream&, const BELPrintContext& ) const;
	uint32_t compWordId;  
	BTND_Pattern_CompoundedWord() :
		compWordId(0xffffffff)
	{}
	BTND_Pattern_CompoundedWord(uint32_t cwi ) : compWordId(cwi) {}
};
inline std::ostream& operator <<( std::ostream& fp, const BTND_Pattern_CompoundedWord& x )
	{ return( fp << "compw[" << std::hex << x.compWordId << "]");}

struct BTND_Pattern_Token : public BTND_Pattern_Base {
	std::ostream& print( std::ostream&, const BELPrintContext& ) const;
	ay::UniqueCharPool::StrId stringId;

	BTND_Pattern_Token() : 
		stringId(0xffffffff)
	{}
	BTND_Pattern_Token(ay::UniqueCharPool::StrId id) : 
		stringId(id)
	{}
};
/// stop token is a regular token literal except it will be ignored 
/// by any tag matching algorithm
struct BTND_Pattern_StopToken : public BTND_Pattern_Token {
	std::ostream& print( std::ostream& fp, const BELPrintContext& ctxt ) const
	{
		return BTND_Pattern_Token::print( fp, ctxt ) << "<STOP>";
	}
	//ay::UniqueCharPool::StrId stringId;

	BTND_Pattern_StopToken() : BTND_Pattern_Token(0xffffffff) {}
	BTND_Pattern_StopToken(ay::UniqueCharPool::StrId id) : 
		BTND_Pattern_Token(id)
	{}
};

inline std::ostream& operator <<( std::ostream& fp, const BTND_Pattern_StopToken& x )
	{ return( fp << "stop[" << std::hex << x.stringId << "]");}
inline std::ostream& operator <<( std::ostream& fp, const BTND_Pattern_Token& x )
	{ return( fp << "string[" << std::hex << x.stringId << "]");}

/// this pattern is used to match BarzerEntityList as well as BarzerEntityRangeCombo
class BTND_Pattern_Entity : public BTND_Pattern_Base {
	BarzerEntity d_ent;
	BarzerRange d_range;
	uint8_t     d_rangeIsValid; 
public:
	std::ostream& print( std::ostream& fp, const BELPrintContext& ctxt ) const
	{
		d_ent.print( fp ) ; 
		if( d_rangeIsValid ) 
			fp << '[' << d_range << ']';
		return fp;
	}
	BTND_Pattern_Entity()  : d_rangeIsValid(0) {}

	void setRange() { if( d_rangeIsValid == 0 ) d_rangeIsValid= 1; }
	void setRange( const BarzerRange& r ) { if( d_rangeIsValid==0 ) d_rangeIsValid=1; d_range=r; }

	bool isRangeValid( ) const { return d_rangeIsValid; } 

	BarzerRange& getRange() { return d_range; }
	const BarzerRange& getRange() const { return d_range; }

	void setEntity( const BarzerEntity& e ) { d_ent=e; }
	void setEntityClass( int c ) { d_ent.setClass( c ); }
	void setEntitySubclass( int c ) { d_ent.setSubclass( c ); }
	void setTokenId( uint32_t c ) { d_ent.setTokenId( c ); }

	const BarzerEntity& getEntity() const { return d_ent; }
	BarzerEntity& getEntity() { return d_ent; }

	bool operator() ( const BarzerEntityRangeCombo& erc ) const {
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
class BTND_Pattern_Range : public BTND_Pattern_Base {
protected:
	enum {
		MODE_TYPE, // only compares the range type 
		MODE_VAL // only compares the range type 
	};
	BarzerRange d_range;
	int d_mode; 
public:
	void setModeToType() { d_mode = MODE_TYPE; }
	void setModeToVal() { d_mode = MODE_VAL; }

	BarzerRange& range() { return d_range; }
	const BarzerRange& range() const { return d_range; }

	BTND_Pattern_Range( ) : d_mode(MODE_TYPE) {}
	BTND_Pattern_Range( const BarzerRange& r) : 
		d_range(r),
		d_mode(MODE_TYPE)
	{}
	void setEntityClass( const StoredEntityClass& c ) 
	{ d_range.setEntityClass(c); }
	bool lessThan( const BTND_Pattern_Range& r ) const { 
		return ay::range_comp().less_than(
			d_mode, d_range,
			r.d_mode, r.d_range
		);
	}
	bool operator()( const BarzerRange& e) const 
	{ 
		if( d_mode == MODE_TYPE ) {
			return ( d_range.getType() == e.getType() );
		}
		if( d_mode == MODE_VAL ) {
			if( d_range.getType() == e.getType() ) {
				if( !e.isEntity() ) {
					return ( d_range == e );
				} else {
					const BarzerRange::Entity* otherEntPair = e.getEntity();
					const BarzerRange::Entity* thisEntPair = d_range.getEntity();
					return (
						thisEntPair->first.matchOther( otherEntPair->first ) &&
						thisEntPair->second.matchOther( otherEntPair->second ) 
					);
				}
			} else
				return false;
		} else 
			return false;
	} 
	std::ostream& print( std::ostream& fp,const BELPrintContext& ) const 
	{ return (fp << '[' << d_range) << ']'; }
};
inline bool operator< ( const BTND_Pattern_Range& l, const BTND_Pattern_Range& r ) 
{ return l.lessThan(r); }

class BTND_Pattern_ERC: public BTND_Pattern_Base {
	BarzerEntityRangeCombo d_erc;

	uint8_t d_matchBlankRange, d_matchBalnkEntity;
public:
	BTND_Pattern_ERC() : d_matchBlankRange(0), d_matchBalnkEntity(0) {}	

	bool isMatchBalnkRange() const { return d_matchBlankRange; }
	bool isMatchBalnkEntity() const { return d_matchBalnkEntity; }

	
	void setMatchBalnkRange() { d_matchBlankRange= 1; }
	void setMatchBalnkEntity() { d_matchBalnkEntity = 1; }

	const BarzerEntityRangeCombo& getERC() const { return d_erc; }
	BarzerEntityRangeCombo& getERC() { return d_erc; }
	bool operator()( const BarzerEntityRangeCombo& e) const 
		{ 
			//return d_erc.matchOtherWithBlanks(e, isMatchBalnkRange(), isMatchBalnkEntity() );
			return d_erc.matchOther(e, !d_erc.getRange().isBlank()); 
		} 
	
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
					d_matchBlankRange, d_matchBalnkEntity,
					r.d_matchBlankRange, r.d_matchBalnkEntity
				) );
		}
};

inline bool operator < ( const BTND_Pattern_ERC& l, const BTND_Pattern_ERC& r ) 
{ return l.lessThan(r); }
inline std::ostream& operator <<( std::ostream& fp, const BTND_Pattern_ERC& e ) 
{ return e.print(fp); }

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
	std::ostream& print( std::ostream& fp ) const 
		{ return (fp << d_exprEclass << ":" << d_exprType ); }
};
inline bool operator<( const BTND_Pattern_ERCExpr& l, const BTND_Pattern_ERCExpr& r ) 
{ return l.lessThan(r); }
/// blank data type 
struct BTND_Pattern_None : public BTND_Pattern_Base{
	std::ostream& print( std::ostream&, const BELPrintContext& ) const;
};
inline std::ostream& operator <<( std::ostream& fp, const BTND_Pattern_None& x )
	{ return( fp << "<none>" ); }

typedef boost::variant< 
		BTND_Pattern_None,				// 0
		BTND_Pattern_Token, 			// 1
		BTND_Pattern_Punct,  			// 2
		BTND_Pattern_CompoundedWord, 	// 3
		BTND_Pattern_Number, 			// 4
		BTND_Pattern_Wildcard, 			// 5
		BTND_Pattern_Date,     			// 6
		BTND_Pattern_Time,     			// 7
		BTND_Pattern_DateTime, 			// 8 
		BTND_Pattern_StopToken,		 	// 9
		BTND_Pattern_Entity,		    // 10
		BTND_Pattern_ERCExpr,			// 11
		BTND_Pattern_ERC,			// 12
		BTND_Pattern_Range				// 13
> BTND_PatternData;


/// these enums MUST mirror the order of the types in BTND_PatternData
enum {
	BTND_Pattern_None_TYPE, 			// 0			
	BTND_Pattern_Token_TYPE, 			// 1			
	BTND_Pattern_Punct_TYPE,  		    // 2
	BTND_Pattern_CompoundedWord_TYPE,	// 3
	/// wildcard types 
	BTND_Pattern_Number_TYPE, 			// 4
	BTND_Pattern_Wildcard_TYPE, 		// 5
	BTND_Pattern_Date_TYPE,     		// 6
	BTND_Pattern_Time_TYPE,    			// 7
	BTND_Pattern_DateTime_TYPE,			// 8
	///
	BTND_Pattern_StopToken_TYPE,		// 9
	BTND_Pattern_Entity_TYPE,           // 10
	BTND_Pattern_ERCExpr_TYPE,          // 11
	BTND_Pattern_ERC_TYPE,          // 12
	BTND_Pattern_Range_TYPE,          // 13


	/// end of wildcard types - add new ones ONLY ABOVE THIS LINE
	/// in general there should be no more than 1-2 more types added 
	/// nest them in another variant 
	BTND_Pattern_MAXWILDCARD_TYPE,
	
	BTND_Pattern_TYPE_MAX
};

struct BTND_Pattern_TypeId_Resolve {
	template <typename T> int operator() () const { return BTND_Pattern_None_TYPE; }

};
template <> inline int BTND_Pattern_TypeId_Resolve::operator()< BTND_Pattern_None > () const { return  BTND_Pattern_None_TYPE; }
template <> inline int BTND_Pattern_TypeId_Resolve::operator()< BTND_Pattern_Token > ( ) const { return  BTND_Pattern_Token_TYPE; }
template <>inline  int BTND_Pattern_TypeId_Resolve::operator()< BTND_Pattern_Punct  > ( ) const { return  BTND_Pattern_Punct_TYPE; }
template <>inline  int BTND_Pattern_TypeId_Resolve::operator()< BTND_Pattern_CompoundedWord > ( ) const { return  BTND_Pattern_CompoundedWord_TYPE; }
template <>inline  int BTND_Pattern_TypeId_Resolve::operator()< BTND_Pattern_Number > ( ) const { return  BTND_Pattern_Number_TYPE; }
template <>inline  int BTND_Pattern_TypeId_Resolve::operator()<BTND_Pattern_Wildcard > ( ) const { return  BTND_Pattern_Wildcard_TYPE; }
template <>inline  int BTND_Pattern_TypeId_Resolve::operator()< BTND_Pattern_Date > ( ) const { return  BTND_Pattern_Date_TYPE; }
template <>inline  int BTND_Pattern_TypeId_Resolve::operator()< BTND_Pattern_Time> ( ) const { return  BTND_Pattern_Time_TYPE; }
template <>inline  int BTND_Pattern_TypeId_Resolve::operator()< BTND_Pattern_DateTime> ( ) const { return  BTND_Pattern_DateTime_TYPE; }
template <>inline  int BTND_Pattern_TypeId_Resolve::operator()< BTND_Pattern_StopToken> ( ) const { return  BTND_Pattern_StopToken_TYPE; }
template <>inline  int BTND_Pattern_TypeId_Resolve::operator()< BTND_Pattern_Entity> ( ) const { return  BTND_Pattern_Entity_TYPE; }
template <>inline  int BTND_Pattern_TypeId_Resolve::operator()< BTND_Pattern_Range> ( ) const { return  BTND_Pattern_Range_TYPE; }
template <>inline  int BTND_Pattern_TypeId_Resolve::operator()< BTND_Pattern_ERCExpr> ( ) const { return  BTND_Pattern_ERCExpr_TYPE; }
template <>inline  int BTND_Pattern_TypeId_Resolve::operator()< BTND_Pattern_ERC> ( ) const { return  BTND_Pattern_ERC_TYPE; }

/// pattern tyepe number getter visitor 

/// BTND pattern accessor - ghetto visitor 
struct BTND_PatternData_Access {
	bool isWildcard( const BTND_PatternData& btnd ) const 
	{
		return( btnd.which() >= BTND_Pattern_Number_TYPE && btnd.which() < BTND_Pattern_MAXWILDCARD_TYPE );
	}

	/// returns the maximum tok span for the wildcard 
	/// generally most things except for the 'Wildcard' (this one skips an arbitrary number of terms 
	/// have the span of 1  
	inline size_t getMaxTokSpan(const BTND_PatternData& btnd ) const 
	{
		if( btnd.which() == BTND_Pattern_Wildcard_TYPE ) 
			return boost::get< BTND_Pattern_Wildcard >(btnd).getMaxTokSpan();
		else 
			return 1;
	}
	inline size_t getMinTokSpan(const BTND_PatternData& btnd ) const 
	{
		if( btnd.which() == BTND_Pattern_Wildcard_TYPE ) 
			return boost::get< BTND_Pattern_Wildcard >(btnd).getMinTokSpan();
		else 
			return 1;
	}
};


typedef std::vector< BTND_PatternData > BTND_PatternDataVec;

//// REWRITE Types
/// translation is essentially a tree each node having one and only 
/// one rewrite type 
/// 
struct BTND_Rewrite_Literal : public  BarzerLiteral {
	std::ostream& print( std::ostream& fp, const BELPrintContext& ctxt ) const
		{ return BarzerLiteral::print( fp, ctxt ); }
};

struct BTND_Rewrite_Number : public BarzerNumber {
	
	uint8_t isConst; /// when false we will need to evaluate underlying nodes

	BTND_Rewrite_Number( ) : isConst(0) {}

	BTND_Rewrite_Number( int x ) : BarzerNumber(x), isConst(0) {}
	BTND_Rewrite_Number( double x ) : BarzerNumber(x), isConst(0) {}

	std::ostream& print( std::ostream& fp, const BELPrintContext& ) const
		{ return BarzerNumber::print( fp ); }

	void set( int i ) { isConst = (uint8_t)1; BarzerNumber::set(i); }
	void set_int( int i ) { isConst = (uint8_t)1; BarzerNumber::set(i); }
	void set( double i ) { isConst = (uint8_t)1; BarzerNumber::set(i); }
	void set_double( double i ) { isConst = (uint8_t)1; BarzerNumber::set(i); }

	void setBarzerNumber( BarzerNumber& x ) const
	{
		x = *(static_cast<const BarzerNumber*>(this));
	}
};
struct BTND_Rewrite_MkEnt {
	uint32_t d_entId;
	/// when mode is 0 d_entId is the single entity Id 
	//  when mode is 1 d_entId is id of the pool of entityIds 
	enum {
		MODE_SINGLE_ENT,
		MODE_ENT_LIST
	};
	uint8_t  d_mode;


	BTND_Rewrite_MkEnt() : d_entId(0xffffffff), d_mode(MODE_SINGLE_ENT) {}
	
	bool isValid() const { return (d_entId != 0xffffffff); }
	bool isSingleEnt() const { return d_mode == MODE_SINGLE_ENT; }
	bool isEntList() const { return d_mode == MODE_ENT_LIST; }

	// single entity manipulation
	void setEntId( uint32_t i ) { 
		d_entId = i; 
		d_mode = MODE_SINGLE_ENT;
	}
	uint32_t getRawId( ) const { return d_entId; }
	uint32_t getEntId( ) const { return ( isSingleEnt() ? d_entId : 0xffffffff ); }

	// entity list manipulation
	void setEntGroupId( uint32_t i ) {
		d_entId = i; 
		d_mode = MODE_ENT_LIST;
	}

	uint32_t getEntGroupId( ) const { return isEntList() ? d_entId :0xffffffff; }

	std::ostream& print( std::ostream& fp, const BELPrintContext& ) const
		{ return (fp<< std::hex << d_entId ); }
};

struct BTND_Rewrite_Variable {
	std::ostream& print( std::ostream&, const BELPrintContext& ) const;
	enum {
		MODE_WC_NUMBER, // wildcard number
		MODE_VARNAME,   // variable name 
		MODE_PATEL_NUMBER, // pattern element number
		MODE_WCGAP_NUMBER, // wildcard gap number
	};
	uint8_t idMode; 
	uint32_t varId; /// when idMode is MODE_VARNAME this is a variable path id (see el_variables.h)
		// otherwise as $1/$2 

	bool isValid() const { return varId != 0xffffffff; }

	uint8_t getIdMode() const { return idMode; }
	uint32_t getVarId() const { return varId; }
		
	bool isVarId() const { return idMode == MODE_VARNAME; }
	bool isWildcardNum() const { return idMode == MODE_WC_NUMBER; }
	bool isPatternElemNumber() const { return idMode == MODE_PATEL_NUMBER; }
	bool isWildcardGapNumber() const { return idMode == MODE_WCGAP_NUMBER; }

	void setVarId( uint32_t vid ){ varId = vid; idMode= MODE_VARNAME; }
	void setWildcardNumber( uint32_t vid ){ varId = vid; idMode= MODE_WC_NUMBER; }
	void setPatternElemNumber( uint32_t vid ){ varId = vid; idMode= MODE_PATEL_NUMBER; }
	void setWildcardGapNumber( uint32_t vid ){ varId = vid; idMode= MODE_WCGAP_NUMBER; }

	BTND_Rewrite_Variable() : 
		idMode(MODE_WC_NUMBER), 
		varId(0xffffffff) 
	{}
};

struct BTND_Rewrite_Function {
	std::ostream& print( std::ostream&, const BELPrintContext& ) const;
	uint32_t nameId; // function name id

	void setNameId( uint32_t i ) { nameId = i ; }
	BTND_Rewrite_Function() : nameId(ay::UniqueCharPool::ID_NOTFOUND) {}
	BTND_Rewrite_Function(ay::UniqueCharPool::StrId id) : nameId(id) {}
};
// blank rewrite data type
struct BTND_Rewrite_None {
	std::ostream& print( std::ostream&, const BELPrintContext& ) const;
	int dummy;
	BTND_Rewrite_None() : dummy(0) {}
};
/// absolute date will always default to today
/// if it has child nodes it will be 
struct BTND_Rewrite_AbsoluteDate : public BarzerDate {
	std::ostream& print( std::ostream& fp, const BELPrintContext& ) const
		{ return BarzerDate::print( fp ); }
};

struct BTND_Rewrite_TimeOfDay : public BarzerTimeOfDay {
	std::ostream& print( std::ostream& fp, const BELPrintContext& ) const
		{ return BarzerTimeOfDay::print( fp ); }
};

enum {
	BTND_Rewrite_AbsoluteDate_TYPE,
	BTND_Rewrite_TimeOfDay_TYPE
};

/// they may actually store an explicit constant range
/// if that werent an issue we could make it a blank type
struct BTND_Rewrite_Range : public BarzerRange {
	std::ostream& print( std::ostream& fp, const BELPrintContext& ) const
		{ return BarzerRange::print( fp ); }
};

/// only makes sense when it's underneath BTND_Rewrite_EntitySearch_Srch
/// each TND_Rewrite_EntitySearch_Srch can have multiple class filters 
struct BTND_Rewrite_EntitySearch_ClassFilter {
	// 0xffff is a wilcdard (meaning any class or subclass would do)
	uint16_t d_entClass, d_entSubclass;

	BTND_Rewrite_EntitySearch_ClassFilter() : d_entClass(0xffff), d_entSubclass(0xffff) {}
	BTND_Rewrite_EntitySearch_ClassFilter( uint16_t c, uint16_t sc) : 
		d_entClass(c), d_entSubclass(sc) 
	{}

	std::ostream& print( std::ostream& fp ) const
		{ return fp << std::hex << d_entClass << ":" << std::hex << d_entSubclass; }
	std::ostream& print( std::ostream& fp, const BELPrintContext& ) const
		{ return print(fp); }
};

/// runs entity search based on subordinate BTND_Rewrite_EntitySearch_Arg_XXX nodes
struct BTND_Rewrite_EntitySearch_Srch {
	std::ostream& print( std::ostream& fp ) const
		{ return fp << "EntSearch"; }
	std::ostream& print( std::ostream& fp, const BELPrintContext& ) const
		{ return print(fp); }
};

typedef boost::variant<
	BTND_Rewrite_EntitySearch_ClassFilter,
	BTND_Rewrite_EntitySearch_Srch
> BTND_Rewrite_EntitySearch;

typedef boost::variant<
	BTND_Rewrite_AbsoluteDate,
	BTND_Rewrite_TimeOfDay
> BTND_Rewrite_DateTime;

typedef boost::variant< 
	BTND_Rewrite_None,
	BTND_Rewrite_Literal,
	BTND_Rewrite_Number,
	BTND_Rewrite_Variable,
	BTND_Rewrite_Function,
	BTND_Rewrite_DateTime,
	BTND_Rewrite_Range,
	BTND_Rewrite_EntitySearch,
	BTND_Rewrite_MkEnt
> BTND_RewriteData;

enum {
	BTND_Rewrite_None_TYPE,
	BTND_Rewrite_Literal_TYPE,
	BTND_Rewrite_Number_TYPE,
	BTND_Rewrite_Variable_TYPE,
	BTND_Rewrite_Function_TYPE,
	BTND_Rewrite_DateTime_TYPE,
	BTND_Rewrite_Range_TYPE,
	BTND_Rewrite_EntitySearch_TYPE,
	BTND_Rewrite_MkEnt_TYPE
}; 

/// when updating the enums make sure to sunc up BTNDDecode::typeName_XXX 

/// pattern structure flow data - blank for now
struct BTND_StructData {
	enum {
		T_LIST, // sequence of elements
		T_ANY,  // any element 
		T_OPT,  // optional subtree
		T_PERM, // permutation of children
		T_TAIL, // if children are A,B,C this translates into [A], [A,B] and [A,B,C]

		/// add new types above this line only
		T_SUBSET, // if children are A,B,C this translates into A,B,C,AB,AC,BC,ABC
		BEL_STRUCT_MAX
	};
protected:
	uint32_t varId;  // variable id default 0xffffffff
	uint8_t type;
public:
	std::ostream& print( std::ostream&, const BELPrintContext& ) const;

	int getType() const { return type; }
	void setType( int t ) { type = t; }

	uint32_t getVarId() const { return varId; }
	void setVarId( uint32_t vi ) { varId = vi; }

	const inline bool hasVar() const { return varId != 0xffffffff; }

	BTND_StructData() : varId(0xffffffff), type(T_LIST) {}
	BTND_StructData(int t) : varId(0xffffffff), type(t) {}
	BTND_StructData(int t, uint32_t vi ) : varId(vi), type(t) {}
};

/// blank data type
struct BTND_None {
	std::ostream& print( std::ostream&, const BELPrintContext& ) const;
};


typedef boost::variant<
	BTND_None, 		  // 0
	BTND_StructData,  // 1
	BTND_PatternData, // 2
	BTND_RewriteData  // 3
> BTNDVariant;

enum {
	BTND_None_TYPE, // 0
	BTND_StructData_TYPE,  // 1
	BTND_PatternData_TYPE, // 2
	BTND_RewriteData_TYPE  // 3
};

struct BTNDDecode {
	static const char* typeName_Pattern ( int x ) ; // BTND_Pattern_XXXX_TYPE values 
	static const char* typeName_Rewrite ( int x ) ; // BTND_Rewrite_XXXX_TYPE values 
	static const char* typeName_Struct ( int x ) ; // BTND_Struct_XXXX_TYPE values 
};

//// tree of these nodes is built during parsing
//// a the next step the tree gets interpreted into BrzelTrie path and added to a trie 
struct BELParseTreeNode {
	/// see barzer_el_btnd.h for the structure of the nested variant 
	BTNDVariant btndVar;  // node data

	typedef std::vector<BELParseTreeNode> ChildrenVec;

	ChildrenVec child;
	/// translation nodes may have things such as <var name="a.b.c"/>
	/// they get encoded during parsing and stored in BarzelVariableIndex::d_pathInterner

	BELParseTreeNode() {}

	template <typename T>
	BELParseTreeNode& addChild( const T& t)
	{
		child.resize( child.size() +1 );
		child.back().btndVar = t;
		return child.back();
	}

	BTNDVariant& getVar() { return btndVar; }
	void clear( ) 
	{
		child.clear();
		btndVar = BTND_None();
	}

	template <typename T>
	T& setNodeData( const T& d ) { return(btndVar=d, boost::get<T>(btndVar));}
	
	template <typename T>
	void getNodeDataPtr_Pattern( T*& ptr ) 
	{ 
		if( btndVar.which() == BTND_PatternData_TYPE ) 
			ptr = boost::get<T>(&(boost::get<BTND_PatternData>(btndVar)));
		else 
			ptr = 0;
	}
	template <typename T>
	void getNodeDataPtr_Struct( T*& ptr ) 
		{ 
		if( btndVar.which() == BTND_StructData_TYPE ) 
			ptr = boost::get<T>(&(boost::get<BTND_StructData>(btndVar)));
		else 
			ptr = 0;
		}

	template <typename T>
	void getNodeDataPtr_Rewrite( T*& ptr ) 
		{ 
		if( btndVar.which() == BTND_RewriteData_TYPE ) 
			ptr = boost::get<T>(&(boost::get<BTND_RewriteData>(btndVar)));
		else 
			ptr = 0;
		}
	
	void print( std::ostream& fp, int depth=0 ) const;
	bool isChildless() const
		{ return ( !child.size() ); }
	
	/// if this subtree is effectively a single rewrite we will return a pointer to the node that 
	/// actually performs the rewrite
	const BELParseTreeNode* getTrivialChild() const
	{
		if( !child.size() ) 
			return this;
		else if( child.size() == 1 )
			return (child.front().isChildless() ? &(child.front()) : 0);
		else
			return 0;
	}
	const BTNDVariant& getNodeData() const 
		{ return btndVar; }
	
	const BTND_RewriteData* getRewriteData() const
		{ return( boost::get<BTND_RewriteData>( &btndVar) ); }
	const BTND_StructData* getStructData() const
		{ return( boost::get<BTND_StructData>( &btndVar) ); }
	const BTND_PatternData* getPatternData() const
		{ return( boost::get<BTND_PatternData>( &btndVar) ); }
	
	const BTND_RewriteData* getTrivialRewriteData() const
	{
		const BELParseTreeNode* tn = getTrivialChild();
		return( tn ? tn->getRewriteData() : 0 );
	}
};

typedef BELVarInfo::value_type VarVec;

struct PatternEmitterNode {
    virtual bool step() = 0;
    virtual void yield(BTND_PatternDataVec& vec, BELVarInfo &vinfo) const = 0;
    virtual ~PatternEmitterNode() {}

    static PatternEmitterNode* make(const BELParseTreeNode& node, VarVec &vars);
};

struct BELParseTreeNode_PatternEmitter {
	BTND_PatternDataVec curVec;
	BELVarInfo varVec;

	const BELParseTreeNode& tree;
	
	BELParseTreeNode_PatternEmitter( const BELParseTreeNode& t ) : tree(t)
        { makePatternTree(); }



	// the next 3 functions should only be ever called in the order they
	// are declared. It's very important

	const BTND_PatternDataVec& getCurSequence( )
	{
		patternTree->yield(curVec, varVec);
		return curVec;
	}

	// should only be called after getCurSequence and before produceSequence
	const BELVarInfo& getVarInfo() const { return varVec; }

	/// returns false when fails to produce a sequence
	bool produceSequence();

	
	~BELParseTreeNode_PatternEmitter();
private:
    PatternEmitterNode* patternTree;
    void makePatternTree();
};

} // barzer namespace

#endif // BARZER_EL_BTND_H
