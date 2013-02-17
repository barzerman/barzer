
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#pragma once
/*
 * barzer_ru_date_util.h
 *
 *  Created on: May 3, 2011
 *      Author: polter
 */
#include <barzer_date_util.h>
#include <ay_char.h>
namespace barzer {
namespace ru {
    const uint8_t lookupMonth(const char* mname);
    const uint8_t lookupWeekday(const char* wdname);
}
} // namespace barzer
