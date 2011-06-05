#ifndef BARZER_BASIC_TYPES_H
#define BARZER_BASIC_TYPES_H
#include <iostream>
#include <limits>
#include <list>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#include <ay/ay_bitflags.h>
//#include <barzer_parse_types.h>
//#include <barzer_dtaindex.h>
#include <boost/variant.hpp>
//#include <barzer_universe.h>
//#include <barzer_storage_types.h>
#include <ay_util.h>
#include <barzer_storage_types.h>

namespace barzer {
struct BELPrintContext;
struct Universe;

struct BarzerNone {
};
inline bool operator<( const BarzerNone& l, const BarzerNone& r ) { return false; }
inline bool operator ==( const BarzerNone& l, const BarzerNone& r ) { return true; }
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

	inline uint32_t 	getUint32() const { return( isInt() ? (uint32_t)(n.i) : 0xffffffff ); }

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
	bool isEqual( const BarzerNumber& r ) const
	{
		if( type != r.type )
			return false;
		else {
			switch(type) {
			case NTYPE_NAN: return true;
			case NTYPE_INT: return ( n.i == r.n.i );
			case NTYPE_REAL: return ( n.real == r.n.real );
			default: return false; // should never be the case 
			}
		}
	}
};

inline bool operator == ( const BarzerNumber& l, const BarzerNumber& r ) 
	{ return l.isEqual(r); }

inline std::ostream& operator <<( std::ostream& fp, const BarzerNumber& n )
{ return n.print(fp); }

/// absolute date
struct BarzerDate {
	// 0 means invalid 
	uint8_t month, day; 
	int16_t year; 

	static uint8_t thisMonth, thisDay; 
	static int16_t thisYear;
	static int32_t longToday;

	enum {
		INVALID_YEAR = 0x7fff,
		MAX_YEAR = 0x7ffe,

		INVALID_MONTH = 0xff,
		MAX_MONTH = 0xfe,
		MAX_DAY = 0xfe,

		INVALID_DAY = 0xff,
		BIZSANE_OLDEST_YEAR  = 1900,
		BIZSANE_LATEST_YEAR  = 2100,
	};

	BarzerDate() : month(INVALID_MONTH), day(INVALID_DAY), year(INVALID_YEAR) {}
	
	bool isValid() const { return( year != INVALID_YEAR && day != INVALID_DAY && month != INVALID_MONTH ); }


	void setToday();
	void setMax( ) 
	{ 
		year = MAX_YEAR;
		month = MAX_MONTH;
		day = MAX_DAY;
	}
	void setMin( ) 
	{ 
		year = -MAX_YEAR;
		month = 0;
		day = 0;
	}
	
	int32_t getLongDate() const {
		return ( 10000*(int32_t)year + 100*(int32_t)month  + (int32_t)day );
	}

	// 1-7 staring from monday
	uint8_t getWeekday() const;

	time_t getTime_t() const;
	void setTime_t(time_t time);

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
	void setYYYYMMDD( int x ) 
	{
		year  = x/10000;
		month = (x%10000)/100;
		day = x%100;
	}
	std::ostream& print( std::ostream& fp ) const 
		{ return ( fp << std::dec << (int)month << '/' << (int)day << '/' << year ); }

	bool lessThan( const BarzerDate& r ) const
	{
		return ay::range_comp( ).less_than(
			year,	month, day,
			r.year,	r.month, r.day
		);
	}
	bool isEqual( const BarzerDate& r ) const
	{ return( (year == r.year) && (month == r.month) && (day  == r.day )); }
};
inline std::ostream&  operator<<( std::ostream& fp, const BarzerDate& d )
{ return ( d.print(fp) ); }

inline bool operator ==( const BarzerDate& l, const BarzerDate& r )
{ return l.isEqual(r); }
inline bool operator<( const BarzerDate& l, const BarzerDate& r )
{ return l.lessThan(r); }

struct BarzerTimeOfDay {
	uint32_t secSinceMidnight; 
	enum {
		MAX_TIMEOFDAY = 24*3600
	};
	/// military time
	BarzerTimeOfDay( uint32_t  x ) : secSinceMidnight(x) {}
	BarzerTimeOfDay( int hh, int mm, int ss ) :
		secSinceMidnight(
			hh*3600 + mm*60 + ss
		)
	{}

	void setHHMMSS( int hh, int mm, int ss ) 
	{ secSinceMidnight = hh*3600 + mm*60 + ss; }
	/// x is long number in HHMMSS format
	void setLong( int32_t x ) 
	{ setHHMMSS( x/10000, (x%10000)/100, x%100 ); }

	// american time hh:mm:ss , pm
	BarzerTimeOfDay( int hh, int mm, int ss, bool isPm ) :
		secSinceMidnight(
			( isPm ? (hh+12): hh) *3600 + mm*60 + ss
		)
	{}
	/// constructe invalid by default
	BarzerTimeOfDay( ) : secSinceMidnight(0xffffffff) {}

	short getHH() const { return (secSinceMidnight/3600); }
	short getMM() const { return ((secSinceMidnight%3600)/60); }
	short getSS() const { return (secSinceMidnight%60); }

	uint32_t getSeconds() const { return secSinceMidnight;	}

	void setMin() { secSinceMidnight= 0; }
	void setMax() { secSinceMidnight= 0xfffffffe; }

	bool isValid() const { return(secSinceMidnight != 0xffffffff); }

	std::ostream& print( std::ostream& fp ) const 
		{ return ( fp << getHH() << ':' << getMM() << ':' << getSS() ); }
	inline bool lessThan( const BarzerTimeOfDay& r ) const
	{
		return (secSinceMidnight< r.secSinceMidnight);
	}
	inline bool isEqual( const BarzerTimeOfDay& r ) const
	{
		return (secSinceMidnight == r.secSinceMidnight);
	}
};
inline bool operator ==( const BarzerTimeOfDay& l, const BarzerTimeOfDay& r )
{ return l.isEqual(r); }
inline bool operator <( const BarzerTimeOfDay& l, const BarzerTimeOfDay& r )
{ return l.lessThan(r); }

inline std::ostream& operator <<( std::ostream& fp, const BarzerTimeOfDay& x )
	{ return( x.print(fp) ); }

struct BarzerDateTime {
	BarzerDate date;
	BarzerTimeOfDay timeOfDay;

	void setMax() { date.setMax(); timeOfDay.setMax(); }
	void setMin() { date.setMin(); timeOfDay.setMin(); }

	bool hasDate() const { return date.isValid(); }
	bool hasTime() const { return timeOfDay.isValid(); }
	bool isValid() const { return ( hasDate() || hasTime() ); }

	inline bool lessThan( const BarzerDateTime& r ) const
	{
		return ay::range_comp().less_than(
			date,		timeOfDay,
			r.date,		r.timeOfDay
		);
	}
	inline bool isEqual( const BarzerDateTime& r ) const
		{ return( (date == r.date) && (timeOfDay == r.timeOfDay) ); }
	std::ostream& print( std::ostream& fp ) const
	{
		return( fp << date << "-" << timeOfDay );
	}
	const BarzerDate& getDate() const { return date; }
	const BarzerTimeOfDay& getTime() const { return timeOfDay; }
	
	void setDate( int x ) 
		{ date.setYYYYMMDD(x); }
	void setDate( const BarzerDate &d )
		{ date = d; }
	void setTime( int x ) 
		{ timeOfDay.setHHMMSS((x/10000),(x%10000)/100,x%100); }
	void setTime( const BarzerTimeOfDay &t )
		{ timeOfDay = t; }
};


inline bool operator == ( const BarzerDateTime& l, const BarzerDateTime& r )
	{ return l.isEqual( r ); }
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
	void setId(uint32_t id) 
	{ theId = id; }
	void setString(uint32_t id) 
		{ type = T_STRING; theId = id;  }
	void setBlank( ) { type = T_BLANK; theId = 0xffffffff; }
	void setStop( ) { type = T_STOP; theId = 0xffffffff; }
	void setStop( uint32_t id ) { type = T_STOP; theId = id; }
	void setPunct(int c) { type = T_PUNCT; theId = c; }
	void setNull() { type = T_STRING; theId = 0xffffffff; }

	uint32_t getId() const { return theId; }
	//uint32_t getType() const { return theId; } // pfft
	uint8_t getType() const { return type; }

	bool isNull() const { return ( type == T_STRING && theId == 0xffffffff ); }
	bool isBlank() const { return type == T_BLANK; }
	bool isStop() const { return type == T_STOP; }
	bool isPunct() const { return type == T_PUNCT; }
	bool isCompound() const { return type == T_COMPOUND; }
	inline bool isEqual( const BarzerLiteral& r ) const 
		{  return( type == r.type && theId == r.theId ); }
};
inline bool operator == ( const BarzerLiteral& l, const BarzerLiteral& r ) 
{ return l.isEqual(r); }

/// range of continuous values (int,real,date,time ...)
struct BarzerRange {
	typedef std::pair< BarzerNone, BarzerNone > None;
	typedef std::pair< int, int > Integer;
	typedef std::pair< float, float > Real;
	typedef std::pair< BarzerTimeOfDay, BarzerTimeOfDay > TimeOfDay;
	typedef std::pair< BarzerDate, BarzerDate > Date;
	typedef std::pair< BarzerEntity, BarzerEntity > Entity;

	typedef boost::variant<
		None,
		Integer,
		Real,
		TimeOfDay,
		Date,
		Entity
	> Data;

	enum {
		None_TYPE,
		Integer_TYPE,
		Real_TYPE,
		TimeOfDay_TYPE,
		Date_TYPE,
		Entity_TYPE
	};

	Data dta;

	enum {
		ORDER_ASC,
		ORDER_DESC
	};

	uint8_t order;  // ASC/DESC

	std::ostream& print( std::ostream& fp ) const;


	void setData(const Data &d) { dta = d; }
	const Data& getData() const { return dta; }
	Data& getData() { return dta; }
	uint32_t getType() const { return dta.which(); }

	bool isBlank() const { return !dta.which(); }
	void setAsc() { order = ORDER_ASC; }
	void setDesc() { order = ORDER_DESC; }
	bool isAsc() const { return ORDER_ASC== order; }
	bool isDesc() const { return ORDER_DESC== order; }

	BarzerRange( ) : order(ORDER_ASC) {}

	struct Less_visitor : public boost::static_visitor<bool> {
		typedef BarzerRange::Data Data;
		const Data& leftVal;
		Less_visitor( const Data& l ) : leftVal(l) {}
		template <typename T> bool operator()( const T& rVal ) const { return (boost::get<T>( leftVal ) < rVal); }
	};
	struct Equal_visitor : public boost::static_visitor<bool> {
		typedef BarzerRange::Data Data;
		const Data& leftVal;
		Equal_visitor( const Data& l ) : leftVal(l) {}
		template <typename T> bool operator()( const T& rVal ) const { return (boost::get<T>( leftVal ) == rVal); }
	};

	bool lessThan( const BarzerRange& r ) const
	{
		if( dta.which() < r.dta.which() ) 
			return true;
		else if( r.dta.which() < dta.which() ) 
			return false;
		else  /// this guarantees that the left and right types are the same 
			return boost::apply_visitor( Less_visitor(dta), r.dta );
	}
	bool isEqual( const BarzerRange& r )  const
	{
		if( dta.which() != r.dta.which() ) 
			return false;
		else  /// this guarantees that the left and right types are the same 
			return boost::apply_visitor( Equal_visitor(dta), r.dta );
	}
	void setEntityClass( const StoredEntityClass& c ) 
		{ dta = Entity( StoredEntityUniqId(c), StoredEntityUniqId(c)); }
};
inline bool operator== ( const BarzerRange& l, const BarzerRange& r ) { return l.isEqual(r); }
inline bool operator< ( const BarzerRange& l, const BarzerRange& r ) { return l.lessThan(r); }
inline std::ostream& operator <<( std::ostream& fp, const BarzerRange& x )
	{ return( x.print(fp) ); }

/// combination 
struct BarzerEntityRangeCombo {
	BarzerEntity d_entId; // main entity id
	BarzerEntity d_unitEntId; // unit entity id 
	BarzerRange  d_range;
	
	const BarzerEntity& getEntity() const { return d_entId; }
	const BarzerEntity& getUnitEntity() const { return d_unitEntId; }
	const BarzerRange&  getRange() const { return d_range; }

	BarzerRange&  getRange() { return d_range; }

	void  setEntity( const BarzerEntity& e ) { d_entId = e; }
	void  setUnitEntity( const BarzerEntity& e ) { d_unitEntId = e; }
	void  setRange( const BarzerRange& r ) { d_range = r; }

	std::ostream& print( std::ostream& fp ) const
		{ return ( d_range.print( fp )<<"("  << d_entId << ":" << d_unitEntId << ")" ); }
	bool isEqual( const BarzerEntityRangeCombo& r ) const  
	{ return( (d_entId == r.d_entId) && (d_unitEntId == r.d_unitEntId) && (d_range == r.d_range) ); }
	bool lessThan( const BarzerEntityRangeCombo& r ) const
	{
		return ay::range_comp( ).less_than(
			d_entId, d_unitEntId, d_range,
			r.d_entId, r.d_unitEntId, r.d_range
		);
	}
};

inline bool operator ==( const BarzerEntityRangeCombo& l, const BarzerEntityRangeCombo& r )
{ return l.isEqual(r); }

inline bool operator <( const BarzerEntityRangeCombo& l, const BarzerEntityRangeCombo& r )
{ return l.lessThan(r); }
/// ERC expression for example (ERC and ERC ... ) 
struct BarzerERCExpr {
	typedef boost::variant<
		BarzerEntityRangeCombo,
		BarzerERCExpr
	> Data;

	typedef std::list< Data > DataList;
	DataList d_data;

	enum {
		T_LOGIC_AND,
		T_LOGIC_OR,
		// add hardcoded expression types above this line 
		T_LOGIC_CUSTOM
	};
	uint16_t d_type; // one of T_XXX values	
	enum {
		EC_LOGIC,
		EC_ARBITRARY=0xffff
	};
	uint16_t d_eclass; // for custom types only currently unused. default 0 

	BarzerERCExpr( const BarzerERCExpr& x ) : d_data(x.d_data), d_type(x.d_type), d_eclass(x.d_eclass) {}
	BarzerERCExpr( ) : d_type(T_LOGIC_AND ), d_eclass(EC_LOGIC) {}
	//BarzerERCExpr( uint16_t t = T_LOGIC_AND ) : d_type(t), d_eclass(EC_LOGIC) {}
	BarzerERCExpr( uint16_t t ) : d_type(t), d_eclass(EC_LOGIC) {}

	const char* getTypeName() const ;

	uint16_t getEclass() const { return d_eclass; }
	uint16_t getType() const { return d_type; }

	const DataList& getData() const { return d_data; }

	void setEclass(uint16_t t ) { d_eclass=t; }
	void setType( uint16_t t ) { d_type=t; }

	void addToExpr( const BarzerEntityRangeCombo& erc, uint16_t type = T_LOGIC_AND, uint16_t eclass = EC_LOGIC) 
	{
		if( eclass != EC_LOGIC ) {
			d_data.push_back( Data(erc) );
			return;
		}
		if( type == d_type ) {
			d_data.push_back( Data(erc) );
		} else {
			if( d_data.empty() ) {
				d_type = type;
				d_data.push_back( Data(erc) );
			} else {
				DataList::iterator end = d_data.end();

				d_data.push_back( Data() );
				d_data.back() = *this;
				d_type = type;
				d_data.push_back( Data() );
				d_data.back() = erc;
	
				d_data.erase( d_data.begin(), end );
			}
		}
	}
	std::ostream& print( std::ostream& fp ) const;

	bool lessThan( const BarzerERCExpr& r ) const 
	{
			return ay::range_comp( ).less_than(
				d_data, d_type, d_eclass ,
				r.d_data, r.d_type, r.d_eclass 
			);
	}
	bool isEqual( const BarzerERCExpr& r ) const 
	{ return ( (d_data == r.d_data) && (d_type == r.d_type) && (d_eclass == r.d_eclass) ); }
};
inline bool operator< ( const BarzerERCExpr& l, const BarzerERCExpr& r )
	{ return l.lessThan(r); }
inline bool operator== ( const BarzerERCExpr& l, const BarzerERCExpr& r )
	{ return l.isEqual(r);}

} // namespace barzer ends

#endif // BARZER_BASIC_TYPES_H
