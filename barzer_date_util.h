/*
 * barzer_date_util.h
 *
 *  Created on: May 2, 2011
 *      Author: polter
 */

#ifndef BARZER_DATE_UTIL_H_
#define BARZER_DATE_UTIL_H_


#include <boost/unordered_map.hpp>
#include <vector>

namespace barzer {

class GlobalPools;
class DateLookup {
private:
	GlobalPools &globPools;
    
public:
	DateLookup(GlobalPools &u) : globPools(u)
	{
		init();
	}

	void init();

    const uint32_t getMonthID(const char* month_name) const;
    const uint32_t getMonthID(const uint8_t month_num) const;
    
    const uint32_t getWdayID(const char* month_name) const;
    const uint32_t getWdayID(const uint8_t month_num) const;
    
    const uint8_t resolveMonthID(const uint32_t mid) const;
    const uint8_t resolveWdayID(const uint32_t mid) const;
    
	const uint8_t lookupMonth(const char*) const;
	const uint8_t lookupMonth(const uint32_t) const;
	const uint8_t lookupWeekday(const char*) const;
	const uint8_t lookupWeekday(const uint32_t) const;



};

} // namespace barzer

#endif /* BARZER_DATE_UTIL_H_ */
