/*
 * barzer_date_util.h
 *
 *  Created on: May 2, 2011
 *      Author: polter
 */

#ifndef BARZER_DATE_UTIL_H_
#define BARZER_DATE_UTIL_H_


#include <boost/unordered_map.hpp>

namespace barzer {

class StoredUniverse;
class DateLookup {
public:
	typedef void (*DateLookupFunc)( DateLookup& );
	typedef std::pair<uint32_t,uint8_t> DateLookupRec;
	typedef boost::unordered_map<uint32_t,uint8_t> DateLookupMap;
private:
	StoredUniverse &universe;
	DateLookupMap monthMap;
	DateLookupMap weekdayMap;
public:
	DateLookup(StoredUniverse &u) : universe(u), monthMap(12*4), weekdayMap(7*4) {
		init();
	}

	void init();

	void addMonths(const char*[]);
	void addWeekdays(const char*[]);

	void addMonth(const char*, const uint8_t);
	void addMonth(const uint32_t, const uint8_t);
	void addWeekday(const char*, const uint8_t);
	void addWeekday(const uint32_t, const uint8_t);


	const uint8_t lookupMonth(const char*) const;
	const uint8_t lookupMonth(const uint32_t) const;
	const uint8_t lookupWeekday(const char*) const;
	const uint8_t lookupWeekday(const uint32_t) const;

};

} // namespace barzer

#endif /* BARZER_DATE_UTIL_H_ */
