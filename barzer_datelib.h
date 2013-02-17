
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#pragma once
#include <barzer_basic_types.h>
#include <ay/ay_logger.h>
/// barzer date calculators (BDM) for relative date overrides
/// there are 2 types of calculators: 
/// 1) Boundary calculator - computes boundary of time range 
/// 2) duration calculator	
namespace barzer {

/// DATE 
/// operator date + days 

/// single date calculator assuming today as a reference point
struct BarzerDate_calc {
    BarzerDateTime d_now; // if d_now.getDate() is valid the value will be used otherwise current time computed

	/// date being calculated 
	BarzerDate d_date;

	/// when true week days, months are assumed in the future 
	/// otherwise in the past. For example 
	/// "on thursday" could mean next thursday or last thursday depending on this 
	/// compare "what sitcoms are playing on thursday"  to "what sitcoms were playing on thursday"
	///
	int8_t d_defaultFuture;

	BarzerDate_calc( const BarzerDate& d, int8_t fut ) : d_date(d), d_defaultFuture(fut) {}
	BarzerDate_calc( ) : d_defaultFuture(1) {}
	BarzerDate_calc( int8_t fut ) : d_defaultFuture(fut) {}


    const BarzerDate& getDate() const { return d_date; }
	void set(int year, int month, int day);

          BarzerDateTime& getNow() { return d_now; }
    const BarzerDateTime& getNow() const { return d_now; }

    void setNow( const BarzerDateTime& n ) {d_now = n; }
    bool isNowSet() { return d_now.isValid();}

    void setNowPtr( const BarzerDateTime* n ) { if( n ) d_now= *n; }
	void setFuture(int8_t);

	void setToday( );
	void dayOffset(int);
	void setTomorrow( );
	void setYesterday( );
	

	// sets weekend depending on the weekCOunt 
	// 0 - this or past (depending 
	void setWeekend( int weekCount=0 );

	// weekDay 1,7 - same as struct tm 
	void setWeekday( uint8_t weekDay );
	
	/// N months ago/from now
	void monthOffset( int monthOffset );
	void yearOffset( int yearOffset );

    void setMonth( uint8_t month );

	enum {
		PAST = -1,
		PRESENT,
		FUTURE
	};

	/** It's better to surround this in try block, especially if source date
	 * isn't validated on input.
	 */
	void getWeek(std::pair<BarzerDate, BarzerDate>& out, int offset = 0) const;
	
	/** It's better to surround this in try block, especially if source date
	 * isn't validated on input.
	 */
	void getMonth(std::pair<BarzerDate, BarzerDate>& out, int offset = 0) const;
};

struct BarzerTime_calc {
	BarzerTimeOfDay d_time;
	enum {
		PER_BEG_OF_HOUR,
		PER_END_OF_HOUR,

		PER_MORNING,
		PER_EVENING,
		PER_AFTERNOON,
		PER_NIGHT
	};
	BarzerTime_calc( const BarzerTimeOfDay &t ) : d_time(t) {}
	/// per - one of the PER constnats
	void setPeriod( int per ); 
};

/// TIME 
/// offsets from time
/// time of day references 
/// evening/morning/noon 


/// month periods references 
/// 
/// seasonal references 
/// - middle/end summer/fall/...

} //namespace barzer

