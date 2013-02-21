
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#pragma once 
#include <barzer_number.h>
#include <barzer_el_pattern.h>
#include <ay/ay_util.h>


namespace barzer {
struct BELPrintContext;
class GlobalPools;

/// simple number wildcard data 
struct BTND_Pattern_Number : public BTND_Pattern_Base {
	std::ostream& print( std::ostream&, const BELPrintContext& ) const;
	enum : uint8_t {
		T_ANY_INT, // any integer 
		T_ANY_REAL, // any real

		T_RANGE_INT, // range is always inclusive as in <= <=
		T_RANGE_REAL, // same as above - always inclusive
		T_ANY_NUMBER, // any number at all real or integer

		T_MAX
	};

	enum : uint8_t {
        RT_FULL,
        RT_NOHI,
        RT_NOLO
    };

	// for EXACT_XXX types only lo is used
	union {
		struct { float lo,hi; } real;
		struct { int lo,hi; } integer;
	} range;

	uint8_t d_asciiLen;
	uint8_t type; // one of T_XXXX 
	uint8_t rangeType; // RT_XXX default RT_FULL
	
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

	BTND_Pattern_Number() : 
        d_asciiLen(0), 
        type(T_ANY_INT) ,
        rangeType(RT_FULL)
    { range.integer.lo = range.integer.hi = 0; }

	void setAnyNumber() { type = T_ANY_NUMBER; }
	void setAnyInt() { type = T_ANY_INT; }
	void setAnyReal() { type = T_ANY_REAL; }

	int getType() const { return type; }
	void setIntRange_Partial( int x, bool noHi ) 
    {
		type = T_RANGE_INT;
        if( noHi ) {
		    range.integer.lo = x;
		    rangeType = RT_NOHI;
        } else {
		    range.integer.hi = x;
		    rangeType = RT_NOLO;
        }
    }
	void setIntRange( int lo, int hi )
	{
		type = T_RANGE_INT;
        rangeType = RT_FULL;

		range.integer.lo = lo;
		range.integer.hi = hi;
	}
	void setRealRange_Partial( float x, bool noHi )
    {
		type = T_RANGE_REAL;
        if( noHi ) {
		    range.real.lo = x;
		    rangeType = RT_NOHI;
        } else { 
		    range.real.hi = x;
		    rangeType = RT_NOLO;
        }
    }
	void setRealRange( float lo, float hi )
	{
		type = T_RANGE_REAL;
        rangeType = RT_FULL;
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
} // namespace barzer

