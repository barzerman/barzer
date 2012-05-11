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
class GlobalPools;
struct BELPrintContext;
class BELTrie;

//// BarzelTrieNodeData - PATTERN types 
/// all pattern classes must inherit from this 
struct BTND_Pattern_Base {
    enum {
        MATCH_MODE_NORMAL=0, 
        MATCH_MODE_NEGATIVE,
        MATCH_MODE_FUZZY,
        MATCH_MODE_FUZZY_NEGATIVE

        /// cant be greater than 7
    };
    uint8_t d_matchMode; // 0 - straight equality match, 1 - negat
    /// match operator 
    template <typename T> bool operator()( const T& t ) const { return false; }
    BTND_Pattern_Base() : d_matchMode(MATCH_MODE_NORMAL) {}
    
    void mkPatternMatch_Normal() { d_matchMode= MATCH_MODE_NORMAL; }
    void mkPatternMatch_Negative() { d_matchMode= MATCH_MODE_NEGATIVE; }
    void mkPatternMatch_Fuzzy() { d_matchMode= MATCH_MODE_FUZZY; }
    void mkPatternMatch_FuzzyNegative() { d_matchMode= MATCH_MODE_FUZZY_NEGATIVE; }

    void setMatchModeFromAttribute( const char* v ) 
    {
        switch(*v) {
        case 'n': mkPatternMatch_Negative(); break;
        case 'f': mkPatternMatch_Fuzzy(); break;
        case 'g': mkPatternMatch_FuzzyNegative(); break;
        }
    }
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

	uint8_t d_asciiLen;
	uint8_t type; // one of T_XXXX 
	// for EXACT_XXX types only lo is used
	union {
		struct { float lo,hi; } real;
		struct { int lo,hi; } integer;
	} range;
	
	uint8_t getAsciiLen() const { return d_asciiLen; }
	void 	setAsciiLen( uint8_t i ) { d_asciiLen= i; }
    
    bool isTrivialInt( ) const 
        { return ( (type == T_RANGE_INT && range.integer.lo == range.integer.hi && (uint32_t)(range.integer.lo)< 0xffffffff) ? true: false); }
    bool isTrivialInt( uint32_t& id ) const 
        { return ( (type == T_RANGE_INT && range.integer.lo == range.integer.hi && (uint32_t)(range.integer.lo)< 0xffffffff) ? (id=range.integer.lo,true): false); }
    
	inline bool lessThan( const BTND_Pattern_Number& r ) const
	{
		if( type < r.type ) 
			return true;
		else if( r.type < type ) 
			return false;
		else {
			switch( type ) {
			case T_ANY_NUMBER: 
				return ( getAsciiLen() < r.getAsciiLen() );
			case T_ANY_INT: 
				return ( getAsciiLen() < r.getAsciiLen() );
			case T_ANY_REAL: 
				return ( getAsciiLen() < r.getAsciiLen() );
			case T_RANGE_INT: 
				return (ay::range_comp().less_than(
						  range.integer.lo,   range.integer.hi,   getAsciiLen() ,
						r.range.integer.lo, r.range.integer.hi, r.getAsciiLen() 
					));
			case T_RANGE_REAL: 
				return ay::range_comp().less_than(
						  range.real.lo,   range.real.hi,   getAsciiLen(),
						r.range.real.lo, r.range.real.hi, r.getAsciiLen()
					);
			default:
				return false;
			}
		}
	}

	bool isReal() const { return (type == T_ANY_REAL || type == T_RANGE_REAL); }

	BTND_Pattern_Number() : d_asciiLen(0), type(T_ANY_INT) { range.integer.lo = range.integer.hi = 0; }

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
	std::ostream& printXML( std::ostream& fp, const GlobalPools&  ) const;
	bool operator()( const BarzerNumber& num ) const;

    bool isNumberInRange( double x ) const
        { return( type== T_RANGE_REAL ? 
            range.real.lo <= x && range.real.hi >= x :
            range.integer.lo <= x && range.integer.hi >= x );
        }
    bool isNumberInRange( const BarzerNumber& x ) const
    {
        return( x.isReal() ? 
            isNumberInRange( x.getReal_unsafe()) :
            isNumberInRange( x.getInt_unsafe()) );
    }
};
inline bool operator <( const BTND_Pattern_Number& l, const BTND_Pattern_Number& r )
	{ return l.lessThan( r ); }
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
    int getChar() const { return theChar; }
};
inline std::ostream& operator <<( std::ostream& fp, const BTND_Pattern_Punct& x )
	{ return( fp << "'" << std::hex << x.theChar << "'" ); }

// simple token wildcard data
/// no attributes - matches anything
struct BTND_Pattern_Wildcard : public BTND_Pattern_Base {
    enum { 
        WT_ANY, // match any bead (default)
        WT_ENT  // entity related stuff (erc, ent ...)
    };
    uint32_t d_type;

    BTND_Pattern_Wildcard() : d_type(WT_ANY) {}

    uint32_t getType() const { return d_type; } 

    bool isType( uint32_t x ) const {
        return (d_type==x);
    }
	bool isLessThan( const BTND_Pattern_Wildcard& r ) const
        { return (d_type< r.d_type); }
    void setFromAttr( const char* v ) {
        if(v) {
            switch( *v ) {
            case 'a': d_type= WT_ANY; break; 
            case 'e': d_type= WT_ENT; break; 
            }
        }
    }
    std::ostream& print( std::ostream& fp ) const { return (fp << d_type); }
    std::ostream& print( std::ostream& fp, const BELPrintContext&   ) const 
    { return print(fp); }
};
inline std::ostream& operator <<( std::ostream& fp, const BTND_Pattern_Wildcard& x )
{ 
        return x.print(fp);
}
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
	
	bool isLoSet() const { return (lo !=MIN_LONG_DATE); }
	bool isHiSet() const { return (hi !=MAX_LONG_DATE); }
	bool operator()( const BarzerDate& dt ) const
		{ return isDateValid( dt.getLongDate(), dt.longToday ); }
	bool isLessThan( const BTND_Pattern_Date& r ) const
		{ return ay::range_comp().less_than( type, lo, hi, r.type, r.lo, r.hi ); }
	std::ostream& printXML( std::ostream& fp, const GlobalPools&  ) const;
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

	bool isLoSet() const { return (lo.isValid() && lo.getSeconds()) ; }
	bool isHiSet() const { return (hi.isValid() && hi.getSeconds() != BarzerTimeOfDay::MAX_TIMEOFDAY) ; }
	void setFuture() { type = T_ANY_FUTURE_TIME; }
	void setPast() { type = T_ANY_PAST_TIME; }

	void setLo( int x ) { lo.setLong( x ); }
	void setHi( int x ) { hi.setLong( x ); }
	uint32_t getLoLong() const { return lo.getLong(); }
	uint32_t getHiLong() const { return hi.getLong(); }
	bool isLessThan( const BTND_Pattern_Time& r ) const
		{ return ay::range_comp().less_than( type,lo, hi, r.type, r.lo, r.hi ); }
	std::ostream& printXML( std::ostream& fp, const GlobalPools&  ) const;
};
inline std::ostream& operator <<( std::ostream& fp, const BTND_Pattern_Time& x )
	{ return( fp << "time." << x.type << "[" << x.lo << "," << x.hi << "]" ); }
inline bool operator <( const BTND_Pattern_Time& l, const BTND_Pattern_Time& r )
{
	return l.isLessThan( r );
}

struct BTND_Pattern_DateTime : public BTND_Pattern_Base {
	std::ostream& printXML( std::ostream&, const GlobalPools&  ) const;
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
	bool doStem;

	BTND_Pattern_Token() : 
		stringId(0xffffffff), doStem(false)
	{}
	BTND_Pattern_Token(ay::UniqueCharPool::StrId id) : 
		stringId(id),
		doStem(false)
	{}
    ay::UniqueCharPool::StrId getStringId() const { return stringId; }
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
		MODE_VAL // compares the range value
	};
	BarzerRange d_range;
	uint8_t d_mode; 

    enum {
        FLAVOR_NORMAL,
        FLAVOR_NUMERIC // when this flavor is set match operator will looks at int and real
    };
	uint8_t d_rangeFlavor; 
public:
	void setModeToType() { d_mode = MODE_TYPE; }
	void setModeToVal() { d_mode = MODE_VAL; }

	bool isModeVal() const { return( MODE_VAL == d_mode) ; }

	BarzerRange& range() { return d_range; }
	const BarzerRange& range() const { return d_range; }

	BTND_Pattern_Range( ) : 
        d_mode(MODE_TYPE) ,
        d_rangeFlavor(FLAVOR_NORMAL)
    {}
	BTND_Pattern_Range( const BarzerRange& r) : 
		d_range(r),
		d_mode(MODE_TYPE),
        d_rangeFlavor(FLAVOR_NORMAL)
	{}
    bool isFlavorNumeric() const { return(d_rangeFlavor == FLAVOR_NORMAL); }
    void setFlavorNumeric() { d_rangeFlavor = FLAVOR_NUMERIC; }

	bool isEntity() const { return d_range.isEntity(); }
	void setEntityClass( const StoredEntityClass& c ) 
	{ d_range.setEntityClass(c); }
	bool lessThan( const BTND_Pattern_Range& r ) const { 
        return( d_mode< r.d_mode ? true :
            (r.d_mode < d_mode ? false : d_range< r.d_range) 
        );
        /*
		return ay::range_comp().less_than(
			d_mode, d_range,
			r.d_mode, r.d_range
		);
        */
	}
	bool operator()( const BarzerRange&) const;
	std::ostream& printXML( std::ostream& fp, const GlobalPools&  ) const ;
	std::ostream& print( std::ostream& fp,const BELPrintContext& ) const 
	{ return (fp << '[' << d_range) << ']'; }
};
inline bool operator< ( const BTND_Pattern_Range& l, const BTND_Pattern_Range& r ) 
{ return l.lessThan(r); }

class BTND_Pattern_ERC: public BTND_Pattern_Base {
	BarzerEntityRangeCombo d_erc;

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

	const BarzerEntityRangeCombo& getERC() const { return d_erc; }
	BarzerEntityRangeCombo& getERC() { return d_erc; }
	bool operator()( const BarzerEntityRangeCombo& e) const 
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

	BTND_Rewrite_Number( int x ) : BarzerNumber((int64_t)x), isConst(0) {}
	BTND_Rewrite_Number( double x ) : BarzerNumber(x), isConst(0) {}

	std::ostream& print( std::ostream& fp, const BELPrintContext& ) const
		{ return BarzerNumber::print( fp ); }

	void set( int64_t i ) { isConst = (uint8_t)1; BarzerNumber::set(i); }
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
		MODE_POSARG // positional argument number
	};
	uint8_t idMode; 
	uint32_t varId; /// when idMode is MODE_VARNAME this is a variable path id (see el_variables.h)
		// otherwise as $1/$2 

	bool isValid() const { return varId != 0xffffffff; }

	uint8_t getIdMode() const { return idMode; }
	uint32_t getVarId() const { return varId; }
		
	bool isPosArg() const { return idMode == MODE_POSARG; }
	bool isVarId() const { return idMode == MODE_VARNAME; }
	bool isWildcardNum() const { return idMode == MODE_WC_NUMBER; }
	bool isPatternElemNumber() const { return idMode == MODE_PATEL_NUMBER; }
	bool isWildcardGapNumber() const { return idMode == MODE_WCGAP_NUMBER; }

	void setPosArg( uint32_t vid ){ varId = vid; idMode= MODE_POSARG; }
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
	uint32_t argStrId; // string id for the optional arg attribute
    uint32_t varId;    // string id for a variable (optional)

	void setNameId( uint32_t i ) { nameId = i ; }
	uint32_t getNameId() const { return nameId; }

    void setArgStrId( uint32_t i ) { argStrId = i; }
    uint32_t getArgStrId( ) const { return argStrId; }

	BTND_Rewrite_Function() : 
        nameId(ay::UniqueCharPool::ID_NOTFOUND),
        argStrId(ay::UniqueCharPool::ID_NOTFOUND) ,
        varId(ay::UniqueCharPool::ID_NOTFOUND) 
    {}

	BTND_Rewrite_Function(ay::UniqueCharPool::StrId id) : 
        nameId(id), 
        argStrId(ay::UniqueCharPool::ID_NOTFOUND)  ,
        varId(ay::UniqueCharPool::ID_NOTFOUND)  
    {}

	BTND_Rewrite_Function(ay::UniqueCharPool::StrId id, ay::UniqueCharPool::StrId argId) : 
        nameId(id), argStrId(argId) , varId(ay::UniqueCharPool::ID_NOTFOUND)
    {}
    
    void        setVarId( uint32_t v ) { varId= v; }
    uint32_t    getVarId() const {return varId; }
    bool        isValidVar() const { return (varId != ay::UniqueCharPool::ID_NOTFOUND); }
};

struct BTND_Rewrite_Select {
    uint32_t varId;
    void setVarId(uint32_t id) { varId = id; }
    uint32_t getVarId() const { return varId; }

    BTND_Rewrite_Select(uint32_t vid = 0xffffffff)
        : varId(vid) {}

    std::ostream& print( std::ostream&, const BELPrintContext& ) const;
};


struct BTND_Rewrite_Case {
    uint32_t ltrlId;
    BTND_Rewrite_Case(uint32_t id = 0xffffffff) : ltrlId(id) {}

    std::ostream& print( std::ostream&, const BELPrintContext& ) const;
};

struct BTND_Rewrite_Logic {
    enum Type { AND, OR, NOT  };
    Type type;
    Type getType() const { return type; }
    void setType(Type t)
        { type = t; }
    BTND_Rewrite_Logic(): type(AND) {}
    std::ostream& print( std::ostream&, const BELPrintContext& ) const;
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

/// control structure - BLOCK/LOOP etc 
/// implementation will start with block 
/// var=control(...) 
struct BTND_Rewrite_Control {
    enum { 
        RWCTLT_COMMA,  // evaluates all children returns the vlue of the last
        RWCTLT_LIST,  // evaluates all children returns everything every child produces
        // init is like a scoped declaration in C++ 
        // let is like X=y in javascript - if no surrounding context has X , X will be created 
        RWCTLT_VAR_BIND, // assigns (binds) variable - tries to find it in current context and if fails - creates
        RWCTLT_VAR_GET,  // extracts variable - tries all ascending contexts 
        
        /// add new ones above this line
        RWCTLT_MAX
    }; 
    uint16_t d_rwctlt; // RWCTLT_XXXX
    uint32_t d_varId;  //  default 0xffffffff - when set results will be also added to the variable 
    BTND_Rewrite_Control() : d_rwctlt(RWCTLT_COMMA), d_varId(0xffffffff) {}
    /// here 4 more bytes of something could be used  
	std::ostream& print( std::ostream& fp ) const
		{ return fp << "Control(" << d_rwctlt << "," << d_varId << ")"; }
	std::ostream& print( std::ostream& fp, const BELPrintContext& ) const
		{ return print(fp); }
    
    bool isValidVar() const { return d_varId != 0xffffffff; }

    void setVarId( uint32_t i ) { d_varId= i; }
    uint32_t getVarId() const { return d_varId; }

    uint16_t getCtrl() const { return d_rwctlt; }
    void setCtrl( uint32_t m )
    {
        if( m>= RWCTLT_COMMA && m< RWCTLT_MAX ) 
            d_rwctlt = static_cast<uint16_t>( m );
    }
    bool isCtrl( uint32_t m ) 
        { return ( m == d_rwctlt ); }
    bool isComma() const { return( d_rwctlt == RWCTLT_COMMA ); }
};

typedef boost::variant< 
	BTND_Rewrite_None,
	BTND_Rewrite_Literal,
	BTND_Rewrite_Number,
	BTND_Rewrite_Variable,
	BTND_Rewrite_Function,
	BTND_Rewrite_DateTime, // needs to be removed
	BTND_Rewrite_Range, // needs to be removed
	BTND_Rewrite_EntitySearch, // needs to be removed
	BTND_Rewrite_MkEnt,
	BTND_Rewrite_Select,
	BTND_Rewrite_Case,
	BTND_Rewrite_Logic,
    BTND_Rewrite_Control
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
	BTND_Rewrite_MkEnt_TYPE,
	BTND_Rewrite_Select_TYPE,
	BTND_Rewrite_Case_TYPE,
	BTND_Rewrite_Logic_TYPE,
	BTND_Rewrite_Control_TYPE
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
	const char* getXMLTag( ) const { 
		switch( type ) {
		case T_LIST: return "list";
		case T_ANY: return "any";
		case T_OPT: return "opt";
		case T_PERM: return "perm";
		case T_TAIL: return "tail";
		case T_SUBSET: return "subs";
		default: return "badstruct";
		}
	}


	std::ostream& printXML( std::ostream&, const BELTrie& ) const;
	std::ostream& print( std::ostream&, const BELPrintContext& ) const;

	int getType() const { return type; }
	void setType( int t ) { type = t; }

	uint32_t getVarId() const { return varId; }
		void setVarId( uint32_t vi ) { varId = vi; }
	
	const inline bool hasVar() const { return varId != 0xffffffff; }

	BTND_StructData() : varId(0xffffffff), type(T_LIST) {}
	BTND_StructData(int t) : varId(0xffffffff), type(t) {}
	BTND_StructData(int t, uint32_t vi ) : varId(vi), type(t) {}

    bool isANY() const { return type == T_ANY; }
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

class BELTrie;

//// tree of these nodes is built during parsing
//// a the next step the tree gets interpreted into BrzelTrie path and added to a trie 
struct BELParseTreeNode {
	BTNDVariant btndVar;  // node data

	typedef std::vector<BELParseTreeNode> ChildrenVec;

    uint8_t noTextToNum;

	ChildrenVec child;
    bool isStruct() const { return btndVar.which() == BTND_StructData_TYPE; }
	/// translation nodes may have things such as <var name="a.b.c"/>
	/// they get encoded during parsing and stored in BarzelVariableIndex::d_pathInterner
    
	BELParseTreeNode():noTextToNum(0) {}

	template <typename T>
	BELParseTreeNode& addChild( const T& t)
	{
		child.resize( child.size() +1 );
		child.back().btndVar = t;
		return child.back();
	}

	BELParseTreeNode& addChild( const BELParseTreeNode& node ) 
	{
		child.resize( child.size() +1 );
		return( child.back() = node, child.back() );
	}

	const BTNDVariant& getVar() const { return btndVar; }
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
	      BTNDVariant& getNodeData()
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

	/// streams out XML recursively. the result should look like regular barzel xml 
	std::ostream& printBarzelXML( std::ostream& fp, const BELTrie&  ) const;

    // tries to produce one of the names from pattern 
    // treats every struct element as a list except for ANY from which only the first element will be extracted  
    // this is *likely* to produce a decent name. This is guaranteed to be very fast
    void getDescriptiveNameFromPattern_simple( std::string& , const GlobalPools& u );
};

/// barzel macros 
class BarzelMacros {
	typedef std::map< uint32_t, BELParseTreeNode >  MacroMap;
	MacroMap d_macroMap;
	
public:
	BELParseTreeNode& addMacro(  uint32_t  macro ) 
		{ return d_macroMap[ macro ]; }
	const BELParseTreeNode* getMacro( uint32_t macro ) const
	{
		MacroMap::const_iterator i = d_macroMap.find( macro );
		return ( i == d_macroMap.end() ? 0: &(i->second) );
	}
    void clear() { d_macroMap.clear(); }
};

} // barzer namespace

#endif // BARZER_EL_BTND_H
