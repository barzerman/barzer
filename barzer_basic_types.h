#ifndef BARZER_BASIC_TYPES_H
#define BARZER_BASIC_TYPES_H
#include <iostream>
#include <limits>
#include <list>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <math.h>

#include <ay/ay_bitflags.h>
#include <boost/variant.hpp>
#include <ay_util.h>
#include <barzer_entity.h>
#include <barzer_number.h>

namespace barzer {
struct BELPrintContext;
struct Universe;

struct BarzelTranslationTraceInfo {
	// string id of the source name
	// its not stoed in the maain string pool but in the per trie pool for internal names only
	uint32_t source;

	// statement number  and the order in which the rule was emitted
	uint32_t statementNum, emitterSeqNo;

	BarzelTranslationTraceInfo( ) : source(0xffffffff), statementNum(0xffffffff),emitterSeqNo(0xffffffff) {}
	void set( uint32_t s, uint32_t st, uint32_t em )
	{ source = s; statementNum = st; emitterSeqNo = em; }

    bool eq( const BarzelTranslationTraceInfo& o ) const
    {
        return ( source == o.source && statementNum == o.statementNum && emitterSeqNo == o.emitterSeqNo );
    }
    typedef std::vector< BarzelTranslationTraceInfo > Vec;
    bool sameStatement(const BarzelTranslationTraceInfo& o ) const
    { return (source==o.source && statementNum==o.statementNum ); }
};
inline bool operator== ( const BarzelTranslationTraceInfo& l, const BarzelTranslationTraceInfo& r )
    { return ( l.source == r.source && l.statementNum == r.statementNum && l.emitterSeqNo == r.emitterSeqNo); }
inline bool operator!= ( const BarzelTranslationTraceInfo& l, const BarzelTranslationTraceInfo& r )
    { return !( l==r); }

inline bool operator< ( const BarzelTranslationTraceInfo& l, const BarzelTranslationTraceInfo& r )
{
    return ay::range_comp( ).less_than(
        l.source,	l.statementNum, l.emitterSeqNo,
        r.source,	r.statementNum, r.emitterSeqNo
    );
}

struct BarzerNone {
};
inline bool operator<( const BarzerNone& l, const BarzerNone& r ) { return false; }
inline bool operator ==( const BarzerNone& l, const BarzerNone& r ) { return true; }

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

	bool isValidYear() const { return( year != INVALID_YEAR ); }
	bool isValidMonth() const { return( month != INVALID_MONTH ); }
	bool isValidDay() const { return( day != INVALID_DAY ); }

	bool isValid() const { return( isValidYear() && isValidMonth() && isValidDay() ); }


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
	int32_t getLong() const { return getLongDate(); }

	// 1-7 staring from monday
	uint8_t getWeekday() const;
	uint8_t getMonth() const { return month; }
	int  getDay() const { return day; }
	int getYear() const { return year; }


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
	void setDay(const BarzerNumber& d) 
        { day = (uint8_t) d.getInt(); }
	void setDay( int day ) { day = (uint8_t)day; }

    void setMonth(const BarzerNumber& d) { month = (uint8_t) d.getInt(); }
    void setMonth(int m) { month = (uint8_t) m; }

    void setYear(const BarzerNumber& y) { year = (int16_t)y.getInt(); }
    void setYear(int y) { year = (int16_t)y; }

	void setDayMonth(const BarzerNumber& d, const BarzerNumber& m) ;
	void setDayMonthYear(const BarzerNumber& d, const BarzerNumber& m, const BarzerNumber& y ) {
		day = (uint8_t) d.getInt();
		month = (uint8_t) m.getInt();
		year = (int16_t) y.getInt();
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

	uint32_t getLong( ) const
		{ return ( 10000 * getHH() + 100 * getMM() + getSS() ); }

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
		T_MEANING,

		T_MAX
	};

    enum {
        STRT_LITERAL = 0x0,          // literal known to system
        STRT_MYSTERY_STRING = 0x1,   // something that is unknown to system yet has been classified as a string
        STRT_INTERNAL       = 0x2    // literal should be looked up in internal pool 
    };
private:
	uint32_t theId; // string id (interned)
    union    { int32_t i4; float r4; } d_num;

	uint8_t  type; // one of the T_ constants
    
    
	uint8_t  stringType; // type of the string (normal token, string etc) - distinction
public:
    enum { NUMERAL_TYPE_NONE, NUMERAL_TYPE_INT, NUMERAL_TYPE_REAL };
private:
    uint8_t  d_numeralType; ///


public:
    int getNumeralType() const { return d_numeralType; }
    bool isNumeral() const { return ( d_numeralType != NUMERAL_TYPE_NONE ); }
    bool isNumeralInt() const { return ( d_numeralType == NUMERAL_TYPE_INT ); }
    bool isNumeralFloat() const { return ( d_numeralType == NUMERAL_TYPE_REAL ); }

    void setNumeral( float x ) { d_numeralType=NUMERAL_TYPE_REAL; d_num.r4= x; }
    void setNumeral( int32_t x ) { d_numeralType=NUMERAL_TYPE_INT; d_num.i4= x; }

    bool toBNumber ( BarzerNumber& n ) const;

	BarzerLiteral() :
		theId(0xffffffff),
		type(T_STRING),
        stringType(STRT_LITERAL),
        d_numeralType(NUMERAL_TYPE_NONE)
	{}
    BarzerLiteral( uint32_t id ) : theId(id), type(T_STRING), stringType(STRT_LITERAL),d_numeralType(NUMERAL_TYPE_NONE)
    {}
    BarzerLiteral( uint32_t id, uint8_t t ) : theId(id), type(t), stringType(STRT_LITERAL), d_numeralType(NUMERAL_TYPE_NONE) {}

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
	BarzerLiteral& setStop( uint32_t id = 0xffffffff ) { type = T_STOP; theId = id; return *this; }
	// void setStop( uint32_t id ) { type = T_STOP; theId = id; }
	BarzerLiteral&  setPunct(int c) { type = T_PUNCT; theId = c; return *this; }
	void setNull() { type = T_STRING; theId = 0xffffffff; }

	void setMeaning(uint32_t id) { type = T_MEANING; theId = id;  }

	uint32_t getId() const { return theId; }
	//uint32_t getType() const { return theId; } // pfft
	uint8_t getType() const { return type; }

	bool isAnyString() const { return ( type == T_STRING || type == T_STOP ); }
	bool isString() const { return ( type == T_STRING && theId != 0xffffffff ); }
	bool isNull() const { return ( type == T_STRING && theId == 0xffffffff ); }
	bool isBlank() const { return type == T_BLANK; }
	bool isStop() const { return type == T_STOP; }
	bool isPunct() const { return type == T_PUNCT; }
	bool isCompound() const { return type == T_COMPOUND; }
	bool isMeaning() const { return type == T_MEANING; }

	inline bool isEqual( const BarzerLiteral& r ) const
		{  return( type == r.type && theId == r.theId ); }

    bool isMysteryString()  const   { return (stringType & 1);  }
    void setMysteryString()         { stringType |= (1); }
    void setInternalString()        { stringType |= (2); }
    bool isInternalString() const   { return (stringType & (2)); }
};
inline bool operator == ( const BarzerLiteral& l, const BarzerLiteral& r )
{ return l.isEqual(r); }

/// range of continuous values (int,real,date,time ...)
struct BarzerRange {
	typedef std::pair< BarzerNone, BarzerNone > None;
	typedef std::pair< int64_t, int64_t > Integer;
	typedef std::pair< double, double > Real;
	typedef std::pair< BarzerTimeOfDay, BarzerTimeOfDay > TimeOfDay;
	typedef std::pair< BarzerDate, BarzerDate > Date;
	typedef std::pair< BarzerDateTime, BarzerDateTime > DateTime;
	typedef std::pair< BarzerEntity, BarzerEntity > Entity;

	typedef boost::variant<
		None,
		Integer,
		Real,
		TimeOfDay,
		Date,
		DateTime,
		Entity
	> Data;

	enum {
		None_TYPE,
		Integer_TYPE,
		Real_TYPE,
		TimeOfDay_TYPE,
		Date_TYPE,
		DateTime_TYPE,
		Entity_TYPE
	};

	Data dta;

	enum {
		ORDER_ASC,
		ORDER_DESC
	};

	bool isNone( ) const {return (dta.which() ==None_TYPE); }
	bool isValid( ) const {return dta.which(); }
	bool isInteger( ) const {return (dta.which() ==Integer_TYPE); }
	bool isReal( ) const {return (dta.which() ==Real_TYPE); }
    bool isNumeric() const { return (isInteger() || isReal()); }
	bool isTimeOfDay( ) const {return (dta.which() ==TimeOfDay_TYPE); }
	bool isDate( ) const {return (dta.which() ==Date_TYPE); }
	bool isDateTime( ) const {return (dta.which() ==DateTime_TYPE); }
	bool isEntity( ) const {return (dta.which() ==Entity_TYPE); }

    const BarzerRange& promote_toReal( );

	const None* getNone( ) const { return boost::get<None>( &dta ); }
	const Integer*  getInteger( ) const {return boost::get<Integer>( &dta ); }
	const Real*  getReal( ) const {return boost::get<Real>( &dta ); }
	const TimeOfDay*  getTimeOfDay( ) const {return boost::get<TimeOfDay>( &dta ); }
	const Date* getDate( ) const {return boost::get<Date>( &dta ); }
	const DateTime* getDateTime( ) const {return boost::get<DateTime>( &dta ); }
	const Entity* getEntity( ) const {return boost::get<Entity>( &dta ); }

	uint8_t order;  // ASC/DESC
	enum {
		RNG_MODE_FULL, // low to high
		RNG_MODE_NO_HI, // above low (no hi limit)
		RNG_MODE_NO_LO // below hi (no low limit)
	};
	uint8_t rng_mode;  // one of the RNG_MODE_XXX constants
	int getRngMode() const { return rng_mode; }

	std::ostream& print( std::ostream& fp ) const;
	bool hasLo() const { return ( rng_mode == RNG_MODE_FULL || rng_mode == RNG_MODE_NO_HI ); }
	bool hasHi() const { return ( rng_mode == RNG_MODE_FULL || rng_mode == RNG_MODE_NO_LO ); }

	bool isFull() const { return ( rng_mode == RNG_MODE_FULL ); }
	bool isNoLo() const { return (rng_mode == RNG_MODE_NO_LO); }
	bool isNoHi() const { return (rng_mode == RNG_MODE_NO_HI); }

	void setNoHI() { rng_mode= RNG_MODE_NO_HI; }
	void setNoLO() { rng_mode= RNG_MODE_NO_LO; }
	void setFullRange() { rng_mode= RNG_MODE_FULL; }

	
        enum {
            RANGE_MOD_SCALE_MULT,
            RANGE_MOD_SCALE_DIV,
            RANGE_MOD_SCALE_PLUS,
            RANGE_MOD_SCALE_MINUS         
        } ;
    /// scales the range if its numeric . returns false if failed to scale (incompatible types etc)
    bool scale( const BarzerNumber& n, int mode  );
    
    bool scale( const BarzerNumber& n1,const BarzerNumber& n2, int mode) ;
	char geXMLtAttrValueChar( ) const ;

	void setData(const Data &d) { dta = d; }
	const Data& getData() const { return dta; }
    
    template <typename T>  T* get() { return boost::get<T>(&dta); }
    template <typename T>  T* set() { return (dta = T(), boost::get<T>(&dta)); }

	Data& getData() { return dta; }
	uint32_t getType() const { return dta.which(); }

	bool isBlank() const { return !dta.which(); }
	void setAsc() { order = ORDER_ASC; }
	void setDesc() { order = ORDER_DESC; }
	bool isAsc() const { return ORDER_ASC== order; }
	bool isDesc() const { return ORDER_DESC== order; }

	BarzerRange( ) : order(ORDER_ASC), rng_mode(RNG_MODE_FULL) {}

	struct Empty_visitor : public boost::static_visitor<bool> {
        template <typename T> bool operator()( const T& val ) const { return (val.first == val.second); }
        bool operator() ( const BarzerRange::Real& val ) const
            { return ( fabs(val.first - val.second) <  0.0000000000001); }
    };
    bool isEmpty() const
        { return (rng_mode == RNG_MODE_FULL && boost::apply_visitor( Empty_visitor(), dta )); }

	struct Less_visitor : public boost::static_visitor<bool> {
		// typedef BarzerRange::Data Data;
		const BarzerRange& leftVal;
		Less_visitor( const BarzerRange& r ) : leftVal(r) {}

		template <typename T> bool operator()( const T& rVal ) const {
			// here we're guaranteed that rng_modes are the same
			switch( leftVal.rng_mode ) {
			case BarzerRange::RNG_MODE_FULL: return (boost::get<T>( leftVal.dta ) < rVal);
			case BarzerRange::RNG_MODE_NO_HI: return (boost::get<T>( leftVal.dta ).first < rVal.first);
			case BarzerRange::RNG_MODE_NO_LO: return (boost::get<T>( leftVal.dta ).second < rVal.second );
			}
			return false;
		}
	};
	struct Equal_visitor : public boost::static_visitor<bool> {
		typedef BarzerRange::Data Data;
		const BarzerRange& leftVal;
		Equal_visitor( const BarzerRange& l ) : leftVal(l) {}
		template <typename T> bool operator()( const T& rVal ) const {
			switch( leftVal.rng_mode ) {
			case BarzerRange::RNG_MODE_FULL: return (boost::get<T>( leftVal.dta ) == rVal);
			case BarzerRange::RNG_MODE_NO_HI: return (boost::get<T>( leftVal.dta ).first == rVal.first);
			case BarzerRange::RNG_MODE_NO_LO: return (boost::get<T>( leftVal.dta ).second == rVal.second );
			}
			return true;
		}
	};

	bool lessThan( const BarzerRange& r ) const
	{
		if( dta.which() < r.dta.which() )
			return true;
		else if( r.dta.which() < dta.which() )
			return false;
		else { /// this guarantees that the left and right types are the same
			if( getRngMode() < r.getRngMode() )
				return true;
			else if( r.getRngMode()  < getRngMode() )
				return false;
			else
				return boost::apply_visitor( Less_visitor(*this), r.dta );
		}
	}
	bool isEqual( const BarzerRange& r )  const
	{
		if( dta.which() != r.dta.which() )
			return false;
		else  /// this guarantees that the left and right types are the same
			return ( getRngMode() == r.getRngMode()  && boost::apply_visitor( Equal_visitor(*this), r.dta ) );
	}
	Entity& setEntityClass( const StoredEntityClass& c ) { return boost::get<Entity>(dta = Entity( StoredEntityUniqId(c), StoredEntityUniqId(c))); }
	Entity& setEntity( ) { return( boost::get<Entity>(dta = Entity())); }

	Entity& setEntity( const StoredEntityUniqId& euid1, const StoredEntityUniqId& euid2 ) { return boost::get<Entity>( dta = Entity(euid1,euid2)); }
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
	BarzerEntity& getEntity() { return d_entId; }
	BarzerEntity& getUnitEntity() { return d_unitEntId; }
	BarzerRange&  getRange() { return d_range; }

    int64_t getInt( int64_t dfVal ) const
    {
        if( d_range.hasLo() ) {
            const BarzerRange::Integer* rng = d_range.getInteger();
            if( rng ) {
                return rng->first;
            }
        }
        return dfVal;
    }

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
	// only range type is checked if at all
	bool matchOther( const BarzerEntityRangeCombo& other, bool checkRange=false ) const {
		return (
			d_entId.matchOther( other.d_entId )  &&
			d_unitEntId.matchOther( other.d_unitEntId ) &&
			(!checkRange || d_range.getType() == other.getRange().getType() )
		);
	}

	bool matchOtherWithBlanks(
		const BarzerEntityRangeCombo& other, bool matchBlankRange, bool matchBlankEnt
	) const {
		return (
			d_entId.matchOther( other.d_entId, matchBlankEnt )  &&
			d_unitEntId.matchOther( other.d_unitEntId, matchBlankEnt ) &&
			(d_range.getType() == other.getRange().getType() || (!matchBlankRange && !d_range.isValid() ))
		);
	}
    const BarzerEntity& mainEntity() const { return d_entId; }
    const BarzerEntity& unitEntity() const { return d_unitEntId; }
    const BarzerRange& range() const { return d_range; }
};

inline bool operator ==( const BarzerEntityRangeCombo& l, const BarzerEntityRangeCombo& r )
{ return l.isEqual(r); }

inline bool operator <( const BarzerEntityRangeCombo& l, const BarzerEntityRangeCombo& r )
{ return l.lessThan(r); }
/// ERC expression for example (ERC and ERC ... )
struct BarzerERCExpr {
	typedef boost::variant<
		BarzerEntityRangeCombo,
		boost::recursive_wrapper<BarzerERCExpr>
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

	
/// BarzerCast(what_is_casted, output_type)
/// returns true if success
struct BarzerCast {
        ///default case
        template <typename T, typename S>
        bool operator()(const T& in, S& out) {return false;}
        
        bool operator()(const BarzerNumber& in, BarzerNumber& out)
        {
                if (out.isInt())                        
                        out.set(int(in.isInt()? in.getInt() : in.getReal()));
                else 
                        out.set(double(in.isInt()? in.getInt() : in.getReal()));
                return true;
        }
};






} // namespace barzer ends

#endif // BARZER_BASIC_TYPES_H
