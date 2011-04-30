#ifndef BARZER_BASIC_TYPES_H
#define BARZER_BASIC_TYPES_H
#include <iostream>
#include <limits>
#include <stdlib.h>
#include <stdint.h>
#include <ay/ay_bitflags.h>
//#include <barzer_parse_types.h>
//#include <barzer_dtaindex.h>
#include <boost/variant.hpp>
//#include <barzer_universe.h>
//#include <barzer_storage_types.h>
#include <ay_util.h>

namespace barzer {
struct BELPrintContext;
struct Universe;

/// pure numbers
class BarzerNumber {
public:
	enum {
		NTYPE_NAN, //  not a number
		NTYPE_INT,
		NTYPE_REAL
	} ;
	
private:
	union {
		int i;
		double  real;
	} n;
	uint8_t type;
public:
	BarzerNumber() : type(NTYPE_NAN) {n.i = 0;}
	BarzerNumber( int i ) : type(NTYPE_INT) {n.i = i;}
	BarzerNumber( double i ) : type(NTYPE_REAL) {n.real = i;}

	void set( int i ) { type= NTYPE_INT; n.i = i; }
	void set( double x ) { type= NTYPE_REAL; n.real = x; }

	void clear() { type = NTYPE_NAN; }
	int setInt( const char* s) { return( type= NTYPE_INT, n.i = atoi(s)); }
	int setReal( const char* s) { return( type= NTYPE_REAL, n.i = atof(s)); }
	inline bool isNan() const { return type == NTYPE_NAN; }
	inline bool isInt() const { return type == NTYPE_INT; }
	inline bool isReal() const { return type == NTYPE_REAL; }

	inline int 		getInt_unsafe() const { return( n.i  ); }
	inline double 	getReal_unsafe() const { return( n.real  ); }

	inline int 		getInt() const { return( isInt() ? n.i : 0 ); }
	inline double 	getReal() const { return( isReal() ? n.real : 0. ); }
	
	inline bool isInt_Nonnegative() const { return ( isInt() && n.i >= 0 );  }
	inline bool isReal_Nonnegative() const { return ( isReal() && n.i >= 0. );  }
	inline bool is_Nonnegative() const { 
		return ( isInt() ?  n.i>=0 : ( isReal() ? n.real>=0. : false ) ); }

	inline bool isInt_Positive() const { return ( isInt() && n.i > 0 );  }
	inline bool isReal_Positive() const { return ( isReal() && n.i >0. );  }
	inline bool is_Positive() const { 
		return ( isInt() ?  n.i>0 : ( isReal() ? n.real>0. : false ) ); }

	inline bool isInt_inRange( int lo, int hi ) const
		{ return ( isInt() && n.i> lo && n.i < hi ); }
	inline bool isReal_inRange( double lo, double hi ) const
		{ return ( isReal() && n.real> lo && n.real < hi ); }

	inline bool is_inRange( double lo, double hi ) const
	{ 
		return ( isReal() ?  (n.real> lo && n.real < hi) : (isInt() ? n.i> lo && n.i < hi: false ) );
	}
	
	// date specific checks
	inline bool cal_canbe_Month() const { return( isInt() ? (n.i>=1 && n.i<=12): false ) ; }
	inline bool cal_canbe_Quarter() const { return( isInt() ? (n.i>=1 && n.i<=4): false ) ; }
	/// rough check for day of the month
	inline bool cal_canbe_Day() const { return( isInt() ? (n.i>=1 && n.i<=31): false ) ; }
	inline std::ostream& print( std::ostream& fp ) const
	{
		if( isInt() ) 
			return fp << n.i ;
		else  if( isReal() ) 
			return fp << n.real ;
		else  
			return fp << "NaN";
	}
};

inline std::ostream& operator <<( std::ostream& fp, const BarzerNumber& n )
{ return n.print(fp); }

/// absolute date
struct BarzerDate {
	// 0 means invalid 
	uint8_t month, day; 
	int16_t year; 

	static uint8_t thisMonth, thisDay; 
	static int16_t thisYear;

	enum {
		INVALID_YEAR = 0x7fff,
		BIZSANE_OLDEST_YEAR  = 1900,
		BIZSANE_LATEST_YEAR  = 2100,
	};

	BarzerDate() : month(0), day(0), year(INVALID_YEAR) {}
	bool isFull() const { return((year!=INVALID_YEAR) && month && day); }
	
	/// returns true if y represents a year which can be reasonably assumed by a business application
	/// (ecommerce etc)  
	inline static bool isSaneBusinessYear( const BarzerNumber& y )
		{ return y.isInt_inRange( BIZSANE_OLDEST_YEAR,BIZSANE_LATEST_YEAR ); }
	inline static bool isSaneBusinessPastYear( const BarzerNumber& y )
		{ return y.isInt_inRange( BIZSANE_OLDEST_YEAR,thisYear ); }
	inline static bool isSaneBusinessFutureYear( const BarzerNumber& y )
		{ return y.isInt_inRange( thisYear,BIZSANE_LATEST_YEAR ); }
	inline static bool validDayMonth( const BarzerNumber& d, const BarzerNumber& m ) 
		{ return( d.cal_canbe_Day() && m.cal_canbe_Month() ); }

	/// must be called first thing in the program
	static void initToday();
	/// will try to set year to current year 
	void setDayMonth(const BarzerNumber& d, const BarzerNumber& m) ;
	void setDayMonthYear(const BarzerNumber& d, const BarzerNumber& m, const BarzerNumber& y ) {
		day = (typeof day) d.getInt();
		month = (typeof month) m.getInt();
		year = (typeof year) y.getInt();
	}
	std::ostream& print( std::ostream& fp ) const 
		{ return ( fp << (int)month << '/' << (int)day << '/' << year ); }
	bool lessThan( const BarzerDate& r ) const
	{
		return ay::range_comp( ).less_than(
			year,	month, day,
			r.year,	r.month, r.day
		);
	}
};
inline std::ostream&  operator<<( std::ostream& fp, const BarzerDate& d )
{ return ( d.print(fp) ); }

inline bool operator<( const BarzerDate& l, const BarzerDate& r )
{ return l.lessThan(r); }

struct BarzerTimeOfDay {
	uint8_t hh, mm, ss; // hours minutes seconds 
	BarzerTimeOfDay( ) : hh(0), mm(0), ss(0) {}
	std::ostream& print( std::ostream& fp ) const 
		{ return ( fp << hh << ':' << mm << ':' << ss ); }
	inline bool lessThan( const BarzerTimeOfDay& r ) const
	{
		return ay::range_comp().less_than( 
			hh, 	mm,   ss,
			r.hh, r.mm, r.ss
		);
	}
};
inline bool operator <( const BarzerTimeOfDay& l, const BarzerTimeOfDay& r )
{ return l.lessThan(r); }

inline std::ostream& operator <<( std::ostream& fp, const BarzerTimeOfDay& x )
	{ return( x.print(fp) ); }

struct BarzerDateTime {
	BarzerDate date;
	BarzerTimeOfDay timeOfDay;
	inline bool lessThan( const BarzerDateTime& r ) const
	{
		return ay::range_comp().less_than(
			date,		timeOfDay,
			r.date,		r.timeOfDay
		);
	}
	std::ostream& print( std::ostream& fp ) const
	{
		return( fp << date << "-" << timeOfDay );
	}
};
inline bool operator< ( const BarzerDateTime& l, const BarzerDateTime& r )
{ return l.lessThan( r ); }
inline std::ostream& operator<< ( std::ostream& fp, const BarzerDateTime& x )
{ return x.print(fp); }

//// barzer literal 

class BarzerLiteral {
public:
	enum {
		T_STRING,
		T_COMPOUND,
		T_STOP, /// rewrites into a blank yet unmatcheable token
		T_PUNCT,
		T_BLANK,

		T_MAX
	};
private:
	uint32_t theId;
	uint8_t  type;
public:
	BarzerLiteral() : 
		theId(0xffffffff),
		type(T_STRING)
	{}

	/// never returns 0, type should be one of the T_XXX constants
	static const char* getTypeName(int t);
		
	std::ostream& print( std::ostream& ) const;
	std::ostream& print( std::ostream&, const BELPrintContext& ) const;
	std::ostream& print( std::ostream&, const Universe& ) const;

	void setCompound(uint32_t id ) 
		{ type = T_COMPOUND; theId = id; }

	void setCompound()
		{ setCompound(0xffffffff); }
	void setString(uint32_t id) 
		{ type = T_STRING; theId = id;  }
	void setBlank( ) { type = T_BLANK; theId = 0xffffffff; }
	void setStop( ) { type = T_STOP; theId = 0xffffffff; }
	void setPunct(int c) { type = T_PUNCT; theId = c; }
	void setNull() { type = T_STRING; theId = 0xffffffff; }

	uint32_t getId() const { return theId; }
	//uint32_t getType() const { return theId; } // pfft
	uint8_t getType() const { return type; }

	bool isNull() const { return ( type == T_STRING && theId == 0xffffffff ); }
	bool isBlank() const { return type == T_BLANK; }
	bool isStop() const { return type == T_STOP; }
	bool isPunct() const { return type == T_PUNCT; }
};
/// range of continuous values (int,real,date,time ...)
struct BarzerRange {
	typedef std::pair< int, int > Integer;
	typedef std::pair< float, float > Real;
	typedef std::pair< BarzerTimeOfDay, BarzerTimeOfDay > TimeOfDay;
	typedef std::pair< BarzerDate, BarzerDate > Date;

	typedef boost::variant<
		Integer,
		Real,
		TimeOfDay,
		Date
	> Data;
	
	Data dta;
	std::ostream& print( std::ostream& fp ) const;
};
inline std::ostream& operator <<( std::ostream& fp, const BarzerRange& x )
	{ return( x.print(fp) ); }

} // namespace barzer ends

#endif // BARZER_BASIC_TYPES_H
