
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
/*
 * barzer_date_util.h
 *
 *  Created on: May 2, 2011
 *      Author: polter
 */

#pragma once

#include <boost/unordered_map.hpp>
#include <vector>

namespace barzer {

class GlobalPools;
class StoredUniverse;
class BarzerLiteral;;
class DateLookup {
private:
	const GlobalPools &globPools;
public:
	DateLookup(const GlobalPools &gp) : globPools(gp){}

	void init(GlobalPools &gp);

    const uint32_t getMonthID(const StoredUniverse&, const char* month_name) const;
    const uint32_t getMonthID(const StoredUniverse&, const uint8_t month_num) const;
    
    const uint32_t getWdayID(const StoredUniverse&, const char* month_name) const;
    const uint32_t getWdayID(const StoredUniverse&, const uint8_t month_num) const;
    
    const uint8_t resolveMonthID(const StoredUniverse&, const uint32_t mid) const;
    const uint8_t resolveWdayID(const StoredUniverse&, const uint32_t mid) const;
    
	const uint8_t lookupMonth(const StoredUniverse&, const char*) const;
	const uint8_t lookupMonth(const StoredUniverse&, const BarzerLiteral& ltrl) const;
	const uint8_t lookupWeekday(const StoredUniverse&, const char*) const;
	// const uint8_t lookupWeekday(const uint32_t) const;
	const uint8_t lookupWeekday(const StoredUniverse&, const BarzerLiteral& ltrl ) const;
};

} // namespace barzer

