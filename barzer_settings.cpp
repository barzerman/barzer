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

BarzerSettings::BarzerSettings(GlobalPools &gp)
	: gpools(gp) 
{ init(); }

StoredUniverse* BarzerSettings::getCurrentUniverse() 
{
	/// this needs to be properly implemented to work for the 
	/// user specific universes 
	StoredUniverse *u = gpools.getUniverse(0);
	return u;
}

void BarzerSettings::init() {
	//reader.initParser(BELReader::INPUT_FMT_XML);
	BarzerDate::initToday();
}

void BarzerSettings::loadRules() {
	using boost::property_tree::ptree;

	StoredUniverse *u = getCurrentUniverse();
	if (!u) return;

	BOOST_FOREACH(ptree::value_type &v, pt.get_child("config.rules")) {
		//AYLOG(DEBUG) << v.second.data().c_str();
		//loadRules(v.second.data().c_str());
		BELReader r(&(u->getBarzelTrie()), *u);
		const char *fname =  v.second.data().c_str();
		int num = r.loadFromFile(fname, BELReader::INPUT_FMT_XML);

		std::cout << num << " statements loaded from `" << fname << "'\n";
	}

}


void BarzerSettings::load() {
	//AYLOG(DEBUG) << "BarzerSettings::load()";
	load(DEFAULT_CONFIG_FILE);
}


void BarzerSettings::loadEntities() {
	using boost::property_tree::ptree;
	DtaIndex &dix = gpools.getDtaIndex();
	BOOST_FOREACH(ptree::value_type &v, pt.get_child("config.entities")) {
		dix.loadEntities_XML(v.second.data().c_str());
	}
}

void BarzerSettings::loadSpell()
{
	using boost::property_tree::ptree;
	StoredUniverse* uptr = getCurrentUniverse();
	if( !uptr ) {
		std::cerr << "No UNIVERSE. PANIC!!!\n";
		return;
	}

	StoredUniverse& universe = *uptr;
	BarzerHunspell& hunspell = universe.getHunspell();

	const char* affxFile = 0;
	const char* mainDict  = 0;
	bool initDone = false;
	BOOST_FOREACH(ptree::value_type &v, pt.get_child("config.spell")) {
		const std::string& tagName = v.first;
		const char* tagVal = v.second.data().c_str();
		if( tagName == "dict" ) {
			if( !initDone )
				mainDict = tagVal;
			else {
				std::cerr << "adding dictionary " << tagVal << std::endl;
				hunspell.addDictionary( tagVal );
			}
		} else 
		if( tagName == "affx" ) {
			affxFile = tagVal;
		} else {
			if( !initDone ) {
				std::cerr << "IGNORED tag " << tagName << ": " << tagVal << std::endl;
				std::cerr << "such tags should follow <dict> and <affx> \n";
			} else {
				if( tagName == "extra" ) {
					std::cerr << "adding extra file " << tagVal << "... " <<
					hunspell.addWordsFromTextFile( tagVal ) << " words added\n";
				}
			}
		}
		if( ! initDone && mainDict && affxFile ) {
			initDone = true;
			std::cerr << "initializing hunspell with " << affxFile << " and " << mainDict << std::endl;
			hunspell.initHunspell( affxFile, mainDict );
		}
	}
}

void BarzerSettings::load(const char *fname) {
	//AYLOGDEBUG(fname);

	read_xml(fname, pt);
	loadSpell();
	loadEntities();
	loadRules();
}

const std::string BarzerSettings::get(const char *key) const {
	return pt.get<std::string>(key);
}

const std::string BarzerSettings::get(const std::string &key) const {
	return get(key.c_str());
}

}
