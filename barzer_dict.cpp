/*
 * barzer_dict.cpp
 *
 *  Created on: May 11, 2011
 *      Author: polter
 */

#include <barzer_dict.h>
#include <barzer_universe.h>
#include <ay/ay_logger.h>

namespace barzer {

BarzerDict::BarzerDict(GlobalPools &u) : globPools(u), hmap(1024) {
	//AYLOG(DEBUG) << "BarzerDict::BarzerDict";
	init();
}

void BarzerDict::init() {
	add("USD", "USD");
	add("usd", "USD");
	add("dollars", "USD");
	add("bucks", "USD");
	add("euro", "EUR");
	add("eur", "EUR");
	add("euros", "EUR");
	add("rubles", "RUB");
	add("рублей", "RUB");
	add("руб", "RUB");
	add("RUB", "RUB");
}

void BarzerDict::add(const char *key, const char *value) {
	ay::UniqueCharPool &sp = globPools.stringPool;
	uint32_t id = sp.internIt(value);
	hmap.insert(DictRec(key, id));
}

const uint32_t BarzerDict::lookup(const std::string& key) const {
	Dict::const_iterator value = hmap.find(key);
	if (value == hmap.end()) {
		return 0;
	}
	return value->second;
}

}
