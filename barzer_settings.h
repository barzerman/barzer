/*
 * barzer_settings.h
 *
 *  Created on: May 9, 2011
 *      Author: polter
 */

#ifndef BARZER_SETTINGS_H_
#define BARZER_SETTINGS_H_


#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <barzer_config.h>
#include <barzer_el_parser.h>
#include <ay/ay_logger.h>

namespace barzer {
class GlobalPools;
class BarzerSettings {
	GlobalPools &gpools;
	//StoredUniverse &universe;
	//BELReader reader;

	boost::property_tree::ptree pt;
public:
	StoredUniverse* getCurrentUniverse() ;

	//BarzerSettings(StoredUniverse&, const char*);
	//BarzerSettings(StoredUniverse&);
	BarzerSettings(GlobalPools&);


	void init();

	void loadRules();
	void loadEntities();
	///loads spellchecker related stuff (hunspell dictionaries, extra word lists and such)
	void loadSpell(StoredUniverse&, boost::property_tree::ptree&);

	void loadTrieset(StoredUniverse&, boost::property_tree::ptree&);
	void loadUser(boost::property_tree::ptree::value_type &);
	void loadUsers();



	void load();
	void load(const char *fname);

	const std::string get(const char*) const;
	const std::string get(const std::string&) const;

};

}
#endif /* BARZER_SETTINGS_H_ */
