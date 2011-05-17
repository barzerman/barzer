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
public:
	typedef void (*DateLookupFunc)( DateLookup& );
	typedef boost::unordered_map<uint32_t,uint8_t> DateLookupMap;
	typedef DateLookupMap::value_type DateLookupRec;

	typedef std::vector<uint32_t> SymbolVec;
	typedef boost::unordered_map<uint32_t,SymbolVec> SymbolStorage;
	typedef SymbolStorage::value_type SymbolRec;

private:
	GlobalPools &globPools;
	DateLookupMap monthMap;
	DateLookupMap weekdayMap;

	SymbolStorage monthStorage;
	SymbolStorage weekdayStorage;


public:
	DateLookup(GlobalPools &u) : globPools(u), monthMap(12*4), weekdayMap(7*4)
	{
		init();
	}

	void init();

	void addMonths(const char*[], const char*);
	void addWeekdays(const char*[], const char*);

	void addMonth(const char*, const uint8_t, const char*);
	void addMonth(const uint32_t, const uint8_t, const char*);
	void addWeekday(const char*, const uint8_t, const char*);
	void addWeekday(const uint32_t, const uint8_t, const char*);


	const uint8_t lookupMonth(const char*) const;
	const uint8_t lookupMonth(const uint32_t) const;
	const uint8_t lookupWeekday(const char*) const;
	const uint8_t lookupWeekday(const uint32_t) const;

	void getMonths(const char*, SymbolVec&) const;
	void getMonths(const uint32_t, SymbolVec&) const;

	void getWeekdays(const char*, SymbolVec&) const;
	void getWeekdays(const uint32_t, SymbolVec&) const;

};

} // namespace barzer

#endif /* BARZER_DATE_UTIL_H_ */
