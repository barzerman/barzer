#ifndef BARZER_DATEMANIP_H
#define BARZER_DATEMANIP_H
#include <barzer_basic_types.h>
/// barzer date calculators (BDM) for relative date overrides
/// there are 2 types of calculators: 
/// 1) Boundary calculator - computes boundary of time range 
/// 2) duration calculator	
namespace barzer {

/// DATE 
/// operator date + days 


/// single date calculaator assuming today as a reference point
struct BarzerDate_calc {
	/// date being calculated 
	BarzerDate d_date;

	/// when true week days, months are assumed in the future 
	/// otherwise in the past. For example 
	/// "on thursday" could mean next thursday or last thursday depending on this 
	/// compare "what sitcoms are playing on thursday"  to "what sitcoms were playing on thursday"
	///
	bool d_defaultFuture; 

	BarzerDate_calc( const BarzerDate& d, bool fut ) : d_date(d), d_defaultFuture(fut) {}
	BarzerDate_calc( ) : d_defaultFuture(true) {}
	BarzerDate_calc( bool fut ) : d_defaultFuture(fut) {}

	void setTomorrow( );
	void setYesterday( );
	void setToday( );
	
	// sets weekend depending on the weekCOunt 
	// 0 - this or past (depending 
	void setWeekend( int weekCount=0 );

	/// same weekday for next/last week deopending on defaultFuture
	void setWeek( );
	// weekDay 1,7 - same as struct tm 
	void setWeekday( int weekDay );
	/// same weekday ona diff week depending on defaultFuture
	void setWeek( int weekOffset );
	
	/// N months ago/from now
	void setMonth( int monthOffset );
	void setYear( int yearOffset );

	/// relative intra-month periods
	enum {
		PER_BEGINNING, // beginning of the month 
		PER_END, // end of the month 
		PER_MIDDLE // middle of the month 
	};
	
	void setAbsoluteMonthPoint( int month, int monPer );

	void addBizDays( int n, uint32_t calendarId );
};

/// date range calculator 
struct BarzerDate_range_calc {
	BarerDate d_lo, d_hi;
	bool d_defaultFuture; 

	BarzerDate_range_calc( const BarerDate& l, const BarerDate& r) : d_defaultFuture(true) {}
	BarzerDate_range_calc() : d_defaultFuture(true) {}
	BarzerDate_range_calc(bool fut ) : d_defaultFuture(fut) {}

	/// period references
	/// month periods 
	enum {
		PER_BEGINNING, // beginning of the month 
		PER_END, // end of the month 
		PER_MIDDLE, // middle of the month 

		// beginning of the month 
		RNG_WEEK1,
		RNG_WEEK2, 
		RNG_WEEK3, 
		RNG_WEEK4,


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

struct BarzerTime_range_calc {
	BarzerTime d_lo, d_hi;
	enum {
		DAYPER_MORNING,
		DAYPER_EVENING,
		DAYPER_AFTERNOON,
		DAYPER_NIGHT
	};
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

}
#endif // BARZER_DATEMANIP_H
