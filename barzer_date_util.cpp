
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
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
const char *g_WDAYS[] = 
    {"NUL","MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN"};    
}

void DateLookup::init(GlobalPools &gp) {
    for (size_t i = 0; i < ARR_SZ(g_MONTHS); ++i)
        gp.internString_internal(g_MONTHS[i]);
    for (size_t i = 0; i < ARR_SZ(g_WDAYS); ++i)
        gp.internString_internal(g_WDAYS[i]);
}


const uint32_t DateLookup::getWdayID(const StoredUniverse& uni, const char* weekday_name) const{
    return globPools.internalString_getId(g_WDAYS[lookupWeekday(uni,weekday_name)]);
}

const uint32_t DateLookup::getWdayID(const StoredUniverse& uni, const uint8_t weekday_num) const{
    if (weekday_num > 7) 
        return globPools.internalString_getId(g_WDAYS[0]);
    else
        return globPools.internalString_getId(g_WDAYS[weekday_num]);
}


const uint32_t DateLookup::getMonthID(const StoredUniverse& uni, const uint8_t month_num) const{
    if (month_num > 12) 
        return globPools.internalString_getId(g_MONTHS[0]);
    else
        return globPools.internalString_getId(g_MONTHS[month_num]);
}

const uint32_t DateLookup::getMonthID(const StoredUniverse& uni, const char* month_name) const{
    return globPools.internalString_getId(g_MONTHS[lookupMonth(uni,month_name)]);
}

const uint8_t DateLookup::resolveMonthID(const StoredUniverse& uni,const uint32_t mid) const {
    return lookupMonth(uni,globPools.internalString_resolve(mid));
}

const uint8_t DateLookup::resolveWdayID(const StoredUniverse& uni,const uint32_t wid) const {
    return lookupMonth(uni, globPools.internalString_resolve(wid));
}

/// case sensitive, suppose input to be lowercased
const uint8_t DateLookup::lookupMonth(const StoredUniverse& uni, const char* mname) const {
    if (mname == 0) return 0;
    switch(Lang::getLang(uni,mname, strlen(mname))){
        case (LANG_ENGLISH):
            return en::lookupMonth(mname);break;
        case(LANG_RUSSIAN):
            return ru::lookupMonth(mname);break;
        default: return 0;
    }
}

const uint8_t DateLookup::lookupMonth(const StoredUniverse& uni,const BarzerLiteral& ltrl) const {
    const char* month = ( ltrl.isInternalString() ? globPools.internalString_resolve(ltrl.getId()) :
        globPools.string_resolve(ltrl.getId()) );
	return (month == 0) ? 0 : lookupMonth(uni,month);

}
const uint8_t DateLookup::lookupWeekday(const StoredUniverse& uni, const char* wdname) const {
    if (wdname == 0) return 0;
    switch(Lang::getLang(uni,wdname, strlen(wdname))){
        case (LANG_ENGLISH):
            return en::lookupWeekday(wdname);break;
        case(LANG_RUSSIAN):
            return ru::lookupWeekday(wdname);break;
        default: return 0;
    }
}

const uint8_t DateLookup::lookupWeekday(const StoredUniverse& uni,const BarzerLiteral& ltrl) const {
    const char* wday = ( ltrl.isInternalString() ? globPools.internalString_resolve(ltrl.getId()) :
        globPools.string_resolve(ltrl.getId()) );
	return ( wday ?  lookupWeekday(uni,wday):0 );
}

} // end of barzer namespace 
