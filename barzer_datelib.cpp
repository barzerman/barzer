/*
 * barzer_datelib.cpp
 *
 *  Created on: May 17, 2011
 *      Author: polter
 */

#include <barzer_datelib.h>
#include <time.h>

namespace barzer {

void BarzerDate_calc::set(int year, int month, int day) {
	time_t t = time(0);
	struct tm tmdate;
	localtime_r(&t, &tmdate);
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
	time_t t = d_date.getTime_t();
	struct tm tmdate;
	localtime_r(&t, &tmdate);

	// tm stored weekdays starting from sunday(0)
	// connverting
	uint8_t todayWeekDay = (tmdate.tm_wday + 6) % 7 + 1;

	//AYLOG(DEBUG) << "todayWeekDay: " << (int)todayWeekDay
//				<< "weekDay: " << (int)weekDay;

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


void BarzerDate_calc::setMonth(int month) {
	set(d_date.year, d_date.month + month, d_date.day);
}



}
