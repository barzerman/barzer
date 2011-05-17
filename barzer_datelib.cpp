/*
 * barzer_datelib.cpp
 *
 *  Created on: May 17, 2011
 *      Author: polter
 */

#include <barzer_datelib.h>
#include <time.h>

namespace barzer {


void BarzerDate_calc::setFuture(bool fut) {
	d_defaultFuture = fut;
}


void BarzerDate_calc::setToday() {
	d_date.setToday();
}


void BarzerDate_calc::setTodayOffset(int offset) {
	time_t t = time(0);
	struct tm tmdate;
	localtime_r(&t, &tmdate);
	tmdate.tm_mday += offset;
	t = mktime(&tmdate);
	d_date.setTime_t(t);
}

void BarzerDate_calc::setTomorrow() { setTodayOffset(1); }
void BarzerDate_calc::setYesterday() { setTodayOffset(-1); }

void BarzerDate_calc::setWeekday(uint8_t weekDay) {
	time_t t = time(0);
	struct tm tmdate;
	localtime_r(&t, &tmdate);

	uint8_t todayWeekDay = tmdate.tm_wday + 1;

	if (todayWeekDay == weekDay) {
		d_date.setTime_t(t);
		return;
	}

	if (d_defaultFuture) {
		if (weekDay > todayWeekDay)
			setTodayOffset(weekDay - toodayWeekDay);
		else
			setTodayOffset( weekDay + (7 - todayWeekDay) );
	} else {
		if (weekDay > todayWeekDay)
			setTodayOffset( -(todayWeekDay + (7 - weekDay)) );
		else
			setTodayOffset( -(todayWeekDay - weekDay) );
	}
}





}
