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
#include <ay/ay_util.h>

namespace barzer {
namespace {
const char *g_MONTHS[] = 
    {"NUL","JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
}

void DateLookup::init() {
    for (size_t i = 0; i < ARR_SZ(g_MONTHS); ++i)
        globPools.internString_internal(g_MONTHS[i]);
}


const uint32_t DateLookup::getMonthID(const uint8_t month_num) const{
    if (month_num < 0 || month_num > 12) 
        return globPools.internalString_getId(g_MONTHS[0]);
    else
        return globPools.internalString_getId(g_MONTHS[month_num]);
}

const uint32_t DateLookup::getMonthID(const char* month_name) const{
    return globPools.internalString_getId(g_MONTHS[lookupMonth(month_name)]);
}

const uint8_t DateLookup::resolveMonthID(const uint32_t mid) const {
    return lookupMonth(globPools.internalString_resolve(mid));
}

/// case sensitive, suppose input to be lowercased
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

}
