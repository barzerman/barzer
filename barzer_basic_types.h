#ifndef BARZER_BASIC_TYPES_H
#define BARZER_BASIC_TYPES_H
#include <iostream>
#include <limits>
#include <stdlib.h>
#include <stdint.h>

namespace barzer {

/// pure numbers
class BarzerNumber {
public:
	typedef enum {
		NTYPE_NAN, //  not a number
		NTYPE_INT,
		NTYPE_REAL
	} Type_t;
	
private:
	Type_t type;
	union {
		int i;
		double  real;
	} n;
public:
	BarzerNumber() : type(NTYPE_NAN) {n.i = 0;}
	BarzerNumber( int i ) : type(NTYPE_INT) {n.i = i;}
	BarzerNumber( double i ) : type(NTYPE_REAL) {n.real = i;}

	int setInt( const char* s) { return( type= NTYPE_INT, n.i = atoi(s)); }
	int setReal( const char* s) { return( type= NTYPE_REAL, n.i = atof(s)); }
	inline bool isNan() const { return type == NTYPE_NAN; }
	inline bool isInt() const { return type == NTYPE_INT; }
	inline bool isReal() const { return type == NTYPE_REAL; }

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
		{ return ( fp << month << '/' << day << '/' << year ); }
};
inline std::ostream&  operator<<( std::ostream& fp, const BarzerDate& d )
{ return ( d.print(fp) ); }

inline bool operator<( const BarzerDate& l, const BarzerDate& r )
{
	if( l.year < r.year )
		return true;
	else if( r.year < l.year ) 
		return false;
	else if( l.month < r.month )
		return true;
	else if( r.month < l.month )
		return true;
	else
		return ( l.day < r.day );
}

}
#endif // BARZER_BASIC_TYPES_H
