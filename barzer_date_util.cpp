/*
 * barzer_date_util.cpp
 *
 *  Created on: May 2, 2011
 *      Author: polter
 */

#include <barzer_date_util.h>
#include <barzer_universe.h>
#include <lg_en/barzer_en_date_util.h>
#include <lg_ru/barzer_ru_date_util.h>

namespace barzer {

static const DateLookup::DateLookupFunc lgfunlist[] =
	{ fillDateInfo_en, fillDateInfo_ru };

void DateLookup::init() {
	static const size_t len = sizeof(lgfunlist)/sizeof(lgfunlist[0]);
	for (size_t i = 0; i < len; ++i) lgfunlist[i](*this);
}

void DateLookup::addMonth(const char* mname, const uint8_t mnum) {
	const uint32_t mid = universe.getStringPool().internIt(mname);
	addMonth(mid, mnum);
}
void DateLookup::addMonth(const uint32_t mid, const uint8_t mnum) {
	monthMap.insert(DateLookupRec(mid, mnum));
}

void DateLookup::addWeekday(const char* wdname, const uint8_t wdnum) {
	const uint32_t wdid = universe.getStringPool().internIt(wdname);
	addWeekday(wdid, wdnum);
}

void DateLookup::addWeekday(const uint32_t wdid, const uint8_t wdnum) {
	weekdayMap.insert(DateLookupRec(wdid, wdnum));
}

void DateLookup::addMonths(const char *months[]) {
	for (int i = 0; i < 12; ++i) addMonth(months[i], i+1);
}
void DateLookup::addWeekdays(const char *wdays[]) {
	for (int i = 0; i < 7; ++i) addWeekday(wdays[i], i);
}

const uint8_t DateLookup::lookupMonth(const char* mname) const {
	const uint32_t mid = universe.getStringPool().getId(mname);
	if (mid == ay::UniqueCharPool::ID_NOTFOUND) return 0;
	return lookupMonth(mid);
}
const uint8_t DateLookup::lookupMonth(const uint32_t mid) const {
	const DateLookupMap::const_iterator mrec = monthMap.find(mid);
	return (mrec == monthMap.end()) ? 0 : mrec->second;

}
const uint8_t DateLookup::lookupWeekday(const char* wdname) const {
	const uint32_t wdid = universe.getStringPool().getId(wdname);
	if (wdid == ay::UniqueCharPool::ID_NOTFOUND) return 0;
	return lookupWeekday(wdid);
}

const uint8_t DateLookup::lookupWeekday(const uint32_t wdid) const {
	const DateLookupMap::const_iterator wdrec = weekdayMap.find(wdid);
	return (wdrec == weekdayMap.end()) ? 0 : wdrec->second;
}

}
