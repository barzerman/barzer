/*
 * bazer_dict.h
 *
 *  Created on: May 11, 2011
 *      Author: polter
 */

#ifndef BAZER_DICT_H_
#define BAZER_DICT_H_

#include <boost/unordered_map.hpp>

namespace barzer {

class GlobalPools;
class BarzerDict {
	typedef boost::unordered_map<std::string,uint32_t> Dict;
	typedef Dict::value_type DictRec;
	GlobalPools &globPools;
	Dict hmap;
public:
	BarzerDict(GlobalPools &g);

	void init();

	void add(const char *key, const char *value);
	//const uint32_t lookup(const char *key) const;
	const uint32_t lookup(const std::string&) const;


};

}


#endif /* BAZER_DICT_H_ */
