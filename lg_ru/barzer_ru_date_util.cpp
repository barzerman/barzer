/*
 * barzer_ru_date_util.cpp
 *
 *  Created on: May 3, 2011
 *      Author: polter
 */

#include <lg_ru/barzer_ru_date_util.h>

namespace barzer {
	void fillDateInfo_ru(DateLookup &dl) {
		static const char* months[] =
			{"январь", "февраль", "март", "апрель", "май", "июнь",
			 "июль", "август", "сентябрь", "октябрь", "декабрь"};
		static const char* monthsShort[] =
			{"янв", "фев", "мар", "апр", "май", "июн",
			 "июл", "авг", "сен", "окт", "ноя", "дек"};
		static const char* wdays[] =
			{"понедельник", "вторник", "среда", "четверг",
			 "пятница", "суббота", "воскресенье"};
		static const char* wdaysShort[] =
			{"пнд", "втр", "срд", "чтв",
			 "птн", "сбт", "вск"};

		dl.addMonths(months);
		dl.addMonths(monthsShort);
		dl.addWeekdays(wdays);
		dl.addWeekdays(wdaysShort);
	}
}
