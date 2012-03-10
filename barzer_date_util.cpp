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

void DateLookup::addMonth(const char* mname, const uint8_t mnum, const char* lang) {
	const uint32_t mid = globPools.stringPool.internIt(mname);
	addMonth(mid, mnum, lang);
}
void DateLookup::addMonth(const uint32_t mid, const uint8_t mnum, const char* lang) {
	monthMap.insert(DateLookupRec(mid, mnum));
	uint32_t langId = globPools.stringPool.internIt(lang);
	monthStorage[langId].push_back(mid);
}

void DateLookup::addWeekday(const char* wdname, const uint8_t wdnum, const char* lang) {
	const uint32_t wdid = globPools.stringPool.internIt(wdname);
	addWeekday(wdid, wdnum, lang);
}

void DateLookup::addWeekday(const uint32_t wdid, const uint8_t wdnum, const char* lang) {
	weekdayMap.insert(DateLookupRec(wdid, wdnum));
	uint32_t langId = globPools.stringPool.internIt(lang);
	weekdayStorage[langId].push_back(wdid);
}

void DateLookup::addMonths(const char *months[], const char* lang) {
	for (int i = 0; i < 12; ++i) addMonth(months[i], i+1, lang);
}
void DateLookup::addWeekdays(const char *wdays[], const char* lang) {
	for (int i = 0; i < 7; ++i) addWeekday(wdays[i], i+1, lang);
}


 const uint8_t DateLookup::lookupMonth(const char* mname) const {
    if (mname == 0) return 0;
    switch(Lang::getLang(mname, strlen(mname))){
        case (LANG_ENGLISH):
            return en::lookupMonth(mname);break;
        case(LANG_RUSSIAN):
            return ru::lookupMonth(mname);break;
        default: return 0;
    }
}

const uint8_t DateLookup::lookupMonth(const uint32_t mid) const {
    const char* month = globPools.getStringPool().resolveId(mid);
	return (month == 0) ? 0 : lookupMonth(month);

}
const uint8_t DateLookup::lookupWeekday(const char* wdname) const {
    if (wdname == 0) return 0;
    switch(Lang::getLang(wdname, strlen(wdname))){
        case (LANG_ENGLISH):
            return en::lookupWeekday(wdname);break;
        case(LANG_RUSSIAN):
            return ru::lookupWeekday(wdname);break;
        default: return 0;
    }
}

const uint8_t DateLookup::lookupWeekday(const uint32_t wdid) const {
    const char* wday = globPools.getStringPool().resolveId(wdid);
	return (wday == 0) ? 0 : lookupWeekday(wday);
}

void DateLookup::getMonths(const uint32_t langId, SymbolVec &vec) const {
	SymbolStorage::const_iterator iter = monthStorage.find(langId);
	if (iter == monthStorage.end()) return;
	const SymbolVec &src = iter->second;
	vec.insert(vec.end(), src.begin(), src.end());

}

void DateLookup::getMonths(const char* lang, SymbolVec &vec) const {
	getMonths(globPools.stringPool.getId(lang), vec);
}


void DateLookup::getWeekdays(const uint32_t langId, SymbolVec &vec) const {
	SymbolStorage::const_iterator iter = weekdayStorage.find(langId);
	if (iter == weekdayStorage.end()) return;
	const SymbolVec &src = iter->second;
	vec.insert(vec.end(), src.begin(), src.end());

}

void DateLookup::getWeekdays(const char* lang, SymbolVec &vec) const {
	getWeekdays(globPools.stringPool.getId(lang), vec);
}

}
