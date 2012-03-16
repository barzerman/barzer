/*
 * barzer_ru_date_util.h
 *
 *  Created on: May 3, 2011
 *      Author: polter
 */

#ifndef BARZER_RU_DATE_UTIL_H_
#define BARZER_RU_DATE_UTIL_H_

#include <barzer_date_util.h>
#include <ay_char.h>
namespace barzer {
namespace ru {
    const uint8_t lookupMonth(const char* mname);
    const uint8_t lookupWeekday(const char* wdname);

//	void fillDateInfo(DateLookup &dl);
}
}

#endif /* BARZER_RU_DATE_UTIL_H_ */
