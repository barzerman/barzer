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
	d_date.setTime_t(mktime(&tmdate));
}

void BarzerDate_calc::setFuture(int8_t fut) {
	d_defaultFuture = fut;
}


void BarzerDate_calc::setToday() {
	d_date.setToday();
}


void BarzerDate_calc::dayOffset(int offset) {
	set(d_date.year, d_date.month, d_date.day - offset);
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

	uint8_t todayWeekDay = tmdate.tm_wday + 1;

	if (todayWeekDay == weekDay) {
		d_date.setTime_t(t);
		return;
	}

	switch(d_defaultFuture) {
	case PAST:
		if (weekDay > todayWeekDay)
			dayOffset( -(todayWeekDay + (7 - weekDay)) );
		else
			dayOffset( -(todayWeekDay - weekDay) );
		break;
	case PRESENT: // this <weekday>
		dayOffset( weekDay - todayWeekDay );
		break;
	case FUTURE:
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
