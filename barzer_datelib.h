#ifndef BARZER_DATEMANIP_H
#define BARZER_DATEMANIP_H
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

	void setFuture(int8_t);

	void setToday( );
	void dayOffset(int);
	void setTomorrow( );
	void setYesterday( );
	

	// sets weekend depending on the weekCOunt 
	// 0 - this or past (depending 
	void setWeekend( int weekCount=0 );

	/// same weekday for next/last week deopending on defaultFuture
	void setWeek( );

	// weekDay 1,7 - same as struct tm 
	void setWeekday( uint8_t weekDay );
	/// same weekday ona diff week depending on defaultFuture
	void setWeek( uint8_t weekOffset );
	
	/// N months ago/from now
	void monthOffset( int monthOffset );
	void yearOffset( int yearOffset );

    void setMonth( uint8_t month );

	/// relative intra-month point
	enum {
		MON_BEGINNING, // beginning of the month 
		MON_END, // end of the month 
		MON_MIDDLE, // middle of the month 

		MON_WEEK1_BEG,
		MON_WEEK2_BEG,
		MON_WEEK3_BEG,
		MON_WEEK4_BEG
		
	};
	
	enum {
		PAST = -1,
		PRESENT,
		FUTURE
	};

	void setAbsoluteMonthPoint( int month, int monPer );

	void addBizDays( int n, uint32_t calendarId );
};

/// date range calculator 
struct BarzerDate_range_calc {
	BarzerDate d_lo, d_hi;
	bool d_defaultFuture; 

	BarzerDate_range_calc( const BarzerDate& l, const BarzerDate& r) : d_defaultFuture(true) {}
	BarzerDate_range_calc() : d_defaultFuture(true) {}
	BarzerDate_range_calc(bool fut ) : d_defaultFuture(fut) {}

	/// period references
	/// month periods 
	enum {
		// beginning of the month 
		MON_RNG_HALF1, // first half of the month
		MON_RNG_HALF2, // second half of the month

		MON_RNG_BEG, // beginning of the month range (first week)
		MON_RNG_END, // end of the month range (last week)
		MON_RNG_MID, // middle of the month range

		/// 1,2,3,4th weeks of the month
		MON_RNG_WEEK1,
		MON_RNG_WEEK2, 
		MON_RNG_WEEK3, 
		MON_RNG_WEEK4
	};
	/// monPer - one of PER_XXXX  constants 
	/// month - 1-12, year YYYY 
	void setAbsoluteMonthPeriod( int month, int monPer, int year );
	/// month -0/1 - 0 - this month 1 - next or last depending on default future
	void setRelativeMonthPeriod( int month, int monPer );

	/// middle of next april
	/// year - 0|1 - for this or next/past
	/// month 1-12
	void setRelativeMonthPeriodWithYear( int year, int month, int monPer  );
	void setRange( const BarzerDate& fromDt, int days, bool bizDays );
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

};

#endif // BARZER_DATEMANIP_H
