
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
/*
 * barzer_datelib.cpp
 *
 *  Created on: May 17, 2011
 *      Author: polter
 */

#include <barzer_datelib.h>
#include <time.h>
#include <boost/date_time/gregorian/gregorian_types.hpp>
#include <arch/barzer_arch.h>

namespace barzer {

void BarzerDate_calc::set(int year, int month, int day) {
	time_t t = time(0);
	struct tm tmdate;
	LOCALTIME_R(&t, &tmdate);
	tmdate.tm_year = year - 1900;
	tmdate.tm_mon = month - 1;
	tmdate.tm_mday = day;
	mktime(&tmdate);

	d_date.day = tmdate.tm_mday;
	d_date.month = tmdate.tm_mon + 1;
	d_date.year = tmdate.tm_year + 1900;
}

void BarzerDate_calc::setFuture(int8_t fut) {
	d_defaultFuture = fut;
}


void BarzerDate_calc::setToday() {
    if( d_now.getDate().isValid() ) 
        d_date = d_now.getDate();
    else
	    d_date.setToday();
}


void BarzerDate_calc::dayOffset(int offset) {
	//AYLOG(DEBUG) << "Setting offset: " << offset;
	set(d_date.year, d_date.month, d_date.day + offset);
}

void BarzerDate_calc::setTomorrow() {
	setToday();
	dayOffset(1);
}
void BarzerDate_calc::setYesterday() {
	setToday();
	dayOffset(-1);
}


void BarzerDate_calc::setWeekday(uint8_t weekDay) {

	uint8_t todayWeekDay = d_date.getWeekday();

	if (todayWeekDay == weekDay && !d_defaultFuture) return;

	switch(d_defaultFuture) {
	case PAST:
		//AYLOG(DEBUG) << "Past";
		if (weekDay < todayWeekDay)
			dayOffset( -(todayWeekDay - weekDay) );
		else
			dayOffset( -(todayWeekDay + (7 - weekDay)) );

		break;
	case PRESENT: // this <weekday>
		//AYLOG(DEBUG) << "Present";
		dayOffset( weekDay - todayWeekDay );
		break;
	case FUTURE:
		//AYLOG(DEBUG) << "Future";
		if (weekDay > todayWeekDay)
			dayOffset(weekDay - todayWeekDay);
		else
			dayOffset( weekDay + (7 - todayWeekDay) );
		break;
	}
}

void BarzerDate_calc::monthOffset(int monthOffset) {
    set(d_date.year, d_date.month + monthOffset, d_date.day);
}

void BarzerDate_calc::setMonth(uint8_t month) {
//	set(d_date.year, d_date.month + month, d_date.day);
    uint8_t currentMonth = d_date.getMonth();

    if (currentMonth == month && !d_defaultFuture) return;

    switch(d_defaultFuture) {
    case PAST:
        //AYLOG(DEBUG) << "Past";
        if (month < currentMonth)
            monthOffset( -(currentMonth - month) );
        else
            monthOffset( -(currentMonth + (12 - month)) );

        break;
    case PRESENT: // this <weekday>
        //AYLOG(DEBUG) << "Present";
        monthOffset( month - currentMonth );
        break;
    case FUTURE:
        //AYLOG(DEBUG) << "Future";
        if (month > currentMonth)
            monthOffset(month - currentMonth);
        else
            monthOffset( month + (12 - currentMonth) );
        break;
    }
}

namespace bg = boost::gregorian;

namespace
{
	void boost2barzer(const bg::date& d, BarzerDate& out)
	{
		out.setDayMonthYear(d.day().as_number(), d.month().as_number(), BarzerNumber(d.year()));
	}
	
	template<typename DurationType>
	void updateOffset(bg::date& start, bg::date& end, int offset)
	{
		if (!offset)
			return;
		
		const DurationType diff(offset);
		
		start += diff;
		end += diff;
	}
}

void BarzerDate_calc::getWeek(std::pair<BarzerDate, BarzerDate>& out, int offset) const
{
	bg::date d(d_date.getYear(), d_date.getMonth(), d_date.getDay());
	auto start = bg::previous_weekday(d, bg::greg_weekday(bg::Monday));
	auto end = start + bg::days(6);
	
	updateOffset<bg::weeks>(start, end, offset);
	
	boost2barzer(start, out.first);
	boost2barzer(end, out.second);
}

void BarzerDate_calc::getMonth(std::pair<BarzerDate, BarzerDate>& out, int offset) const
{
	bg::date start(d_date.getYear(), d_date.getMonth(), 1);
	bg::date end(d_date.getYear(), d_date.getMonth(),
			bg::gregorian_calendar::end_of_month_day(d_date.getYear(), d_date.getMonth()));
	
	updateOffset<bg::months>(start, end, offset);
	
	boost2barzer(start, out.first);
	boost2barzer(end, out.second);
}

}
