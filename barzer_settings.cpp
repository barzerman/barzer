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

namespace barzer {

BarzerSettings::BarzerSettings(StoredUniverse &u)
	: universe(u), reader(&u.getBarzelTrie(), u) {
	init();
}

BarzerSettings::BarzerSettings(StoredUniverse &u, const char *fname)
	: universe(u), reader(&u.getBarzelTrie(), u) {
	init();
	load(fname);
}

void BarzerSettings::init() {
	reader.initParser(BELReader::INPUT_FMT_XML);
}

void BarzerSettings::loadRules(const char *fname) {
	int num = reader.loadFromFile(fname);
	if (num)
		std::cout << num << " statements loaded from `" << fname << "'\n";
}

void BarzerSettings::load(const char *fname = DEFAULT_CONFIG_FILE) {
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
