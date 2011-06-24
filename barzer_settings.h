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
#include <ay/ay_bitflags.h>



namespace barzer {

/// parsing settings true for all users in the instance 
struct ParseSettings {

	// when true everything gets stemmed both on load and on input 
	// by default. in order to reduce the load time individual rulesets can 
	// set presume stemmed 
	bool d_stem; 
	ParseSettings() : 
		d_stem(false) 
	{}
	
	bool stemByDefault() const { return d_stem; }
	void set_stemByDefault() { d_stem = true; }
};

class GlobalPools;
class BarzerSettings {
	GlobalPools &gpools;
	//StoredUniverse &universe;
	//BELReader reader;

	boost::property_tree::ptree pt;

	ParseSettings d_parserSettings;
public:
	const ParseSettings& parserSettings() const { return d_parserSettings; } 
		  ParseSettings& parserSettings() 	    { return d_parserSettings; } 

	StoredUniverse* getCurrentUniverse() ;

	//BarzerSettings(StoredUniverse&, const char*);
	//BarzerSettings(StoredUniverse&);
	BarzerSettings(GlobalPools&);


	void init();

	void loadRules();
	void loadEntities();
	///loads spellchecker related stuff (hunspell dictionaries, extra word lists and such)
	void loadSpell(StoredUniverse&, const boost::property_tree::ptree&);

	void loadTrieset(StoredUniverse&, const boost::property_tree::ptree&);
	void loadUser(const boost::property_tree::ptree::value_type &);
	void loadUsers();



	void load();
	void load(const char *fname);

	const std::string get(const char*) const;
	const std::string get(const std::string&) const;

};

}
#endif /* BARZER_SETTINGS_H_ */
