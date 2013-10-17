#pragma once
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
class StoredUniverse;

/// AmbiguousTraceXXX is needed for rules deletion in cases when the same pattern in different statements maps 
/// into different translations. This type of ambiguity is resolved in one of the two ways:
///  - entity list is formed for simple mkent translations (TYPE_ENTITY)
///  - extra nodes are added for all other kinds (especially mkERC) (TYPE_SUBTREE)
/// for both ways 32 bit unsigned ingteger ids are used
struct AmbiguousTraceId {
    typedef enum : uint8_t{ 
        TYPE_ENTITY,
        TYPE_SUBTREE
    } Type;
    uint8_t  type; // 
    uint32_t id;

    AmbiguousTraceId() : type(TYPE_ENTITY), id(0xffffffff) {}
    AmbiguousTraceId( uint32_t i) : type(TYPE_ENTITY), id(i) {}
    AmbiguousTraceId( uint32_t i, Type t ) : type(t), id(i) {}
        
    AmbiguousTraceId& setEntity( uint32_t i ) { 
        id = i;
        type=TYPE_ENTITY;
        return ( *this ); 
    }
    AmbiguousTraceId& setSubtree( uint32_t  i ) { 
        id = i;
        type=TYPE_SUBTREE;
        return ( *this ); 
    }
    bool isEntity() const { return type==TYPE_ENTITY; }
    bool isSubtree() const { return type==TYPE_SUBTREE; }
    uint32_t getId() const { return id; }
};
struct BarzelTranslationTraceInfo {
	// string id of the source name
	// its not stoed in the maain string pool but in the per trie pool for internal names only
	uint32_t source;

	// statement number  and the order in which the rule was emitted
	uint32_t statementNum, emitterSeqNo;

	BarzelTranslationTraceInfo( ) : source(0xffffffff), statementNum(0xffffffff),emitterSeqNo(0xffffffff) {}
	BarzelTranslationTraceInfo( uint32_t s, uint32_t st, uint32_t em =0xffffffff) 
        : source(s), statementNum(st),emitterSeqNo(em) {}
	void set( uint32_t s, uint32_t st, uint32_t em )
	{ source = s; statementNum = st; emitterSeqNo = em; }

    bool eq( const BarzelTranslationTraceInfo& o ) const
        { return ( source == o.source && statementNum == o.statementNum && emitterSeqNo == o.emitterSeqNo ); }
    bool sameStatementAs( const BarzelTranslationTraceInfo& o ) const
        { return ( source == o.source && statementNum == o.statementNum ); }

    typedef std::pair<BarzelTranslationTraceInfo,AmbiguousTraceId> TraceDataUnit;
    typedef std::vector<TraceDataUnit> Vec;
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
	void setDayMonthYear(const BarzerNumber& d, const BarzerNumber& m, const BarzerNumber& y );
	void setYYYYMMDD( int x );
	std::ostream& print( std::ostream& fp ) const;
    
    std::istream& deserialize( std::istream& fp ) ;
	bool lessThan( const BarzerDate& r ) const;
	bool isEqual( const BarzerDate& r ) const;
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
    std::istream& deserialize( std::istream& fp ) 
    {
        char c;
        int hour=0, minute=0, sec=0;
        fp>> hour >> c >> minute >> c >> sec;
        setHHMMSS( hour, minute, sec );
        return fp;
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
    
    std::istream& deserialize( std::istream& fp ) 
    {
        return( timeOfDay.deserialize( date.deserialize(fp) ) ) ;
    }
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
	// std::ostream& print( std::ostream&, const Universe& ) const;

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
	bool isFluff() const { return isStop(); }

	bool isPunct() const { return type == T_PUNCT; }
	bool isCompound() const { return type == T_COMPOUND; }
	bool isMeaning() const { return type == T_MEANING; }

	inline bool isEqual( const BarzerLiteral& r ) const
		{  return( type == r.type && theId == r.theId ); }
	inline bool isLess( const BarzerLiteral& r ) const
		{  
            if( type < r.type ) 
                return true;
            else if( r.type < type )
                return false;
            else 
                return theId < r.theId;
        }

    bool isMysteryString()  const   { return (stringType & 1);  }
    void setMysteryString()         { stringType |= (1); }
    void setInternalString()        { stringType |= (2); }
    bool isInternalString() const   { return (stringType & (2)); }

    std::pair< const char*, size_t>  toString( const StoredUniverse& u ) const;
};
inline std::ostream& operator<< ( std::ostream& fp, const BarzerLiteral& ltrl ) { return ltrl.print( fp ); }
inline bool operator < ( const BarzerLiteral& l, const BarzerLiteral& r )
    { return l.isLess(r); }
inline bool operator == ( const BarzerLiteral& l, const BarzerLiteral& r )
{ return l.isEqual(r); }


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

