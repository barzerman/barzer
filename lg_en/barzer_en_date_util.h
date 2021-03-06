/*
 * barzer_en_date_util.h
 *
 *  Created on: May 3, 2011
 *      Author: polter
 */

#ifndef BARZER_EN_DATE_UTIL_H_
#define BARZER_EN_DATE_UTIL_H_

#include <barzer_date_util.h>
#include <string.h>
#include <stdint.h>
namespace barzer {
namespace en {
    const uint8_t lookupMonth(const char* mname);
    const uint8_t lookupWeekday(const char* wdname);
}
}


#endif /* BARZER_EN_DATE_UTIL_H_ */
