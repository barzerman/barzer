/*
 * barzer_settings.cpp
 *
 *  Created on: May 9, 2011
 *      Author: polter
 */

#include <barzer_settings.h>
#include <barzer_universe.h>
#include <boost/foreach.hpp>
#include <ay/ay_logger.h>
#include <time.h>
#include <barzer_basic_types.h>

namespace barzer {

/*BarzerSettings::BarzerSettings(StoredUniverse &u)
	: universe(u), reader(&u.getBarzelTrie(), u) {
	init();
} //*/

BarzerSettings::BarzerSettings(GlobalPools &gp)
	: gpools(gp) {
	init();
} //*/


void BarzerSettings::init() {
	//reader.initParser(BELReader::INPUT_FMT_XML);
	BarzerDate::initToday();
}

void BarzerSettings::loadRules(const char *fname) {
#warning "need to change the reader not to require trie and universe for its input"
	StoredUniverse *u = gpools.getUniverse(0);
	BELReader r(&(u->getBarzelTrie()), *u);

	int num = r.loadFromFile(fname, BELReader::INPUT_FMT_XML);
	std::cout << num << " statements loaded from `" << fname << "'\n";
}


void BarzerSettings::load() {
	//AYLOG(DEBUG) << "BarzerSettings::load()";
	load(DEFAULT_CONFIG_FILE);
}

void BarzerSettings::load(const char *fname) {
	using boost::property_tree::ptree;
	//AYLOGDEBUG(fname);

	read_xml(fname, pt);

	BOOST_FOREACH(ptree::value_type &v, pt.get_child("config.rules")) {
		//AYLOG(DEBUG) << v.second.data().c_str();
		loadRules(v.second.data().c_str());
	}


}

const std::string BarzerSettings::get(const char *key) const {
	return pt.get<std::string>(key);
}

const std::string BarzerSettings::get(const std::string &key) const {
	return get(key.c_str());
}

}
