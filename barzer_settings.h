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

class BarzerSettings {
	boost::property_tree::ptree pt;
public:
	BarzerSettings(const char *fname = "config.xml") {
		load(fname);
	}

	void load(const char *fname);

	const std::string get(const char*) const;
	const std::string get(const std::string&) const;

};


#endif /* BARZER_SETTINGS_H_ */
