/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#pragma once
#include <barzer_el_pattern.h>

namespace barzer {

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

} // namespace barzer
