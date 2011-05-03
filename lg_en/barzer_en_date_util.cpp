/*
 * barzer_en_date_util.cpp
 *
 *  Created on: May 3, 2011
 *      Author: polter
 */

#include <lg_en/barzer_en_date_util.h>

namespace barzer {
	void fillDateInfo_en(DateLookup &dl) {
		static const char *months[] =
			{"january", "february", "march", "april", "may", "june", "july",
			 "august", "september", "october", "november", "december"};
		static const char *monthsShort[] =
			{"jan", "feb", "mar", "apr", "may", "jun",
			 "jul", "aug", "sep", "oct", "nov", "dec"};
		static const char *wdays[] =
			{"sunday", "monday", "tuesday", "wednesday",
			 "thursday", "friday", "saturday"};
		static const char *wdaysShort[] =
			{"sun", "mon", "tue", "wed", "thu", "fri", "sat"};
		dl.addMonths(months);
		dl.addMonths(monthsShort);
		dl.addWeekdays(wdays);
		dl.addWeekdays(wdaysShort);
	}
}
