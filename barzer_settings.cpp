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

using boost::property_tree::ptree;

static ptree& empty_ptree() {
	static ptree pt;
	return pt;
}

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
	const ptree &rules = pt.get_child("config.rules", empty_ptree());
	if (rules.empty()) {
		// warning goes here
		return;
	}
	BOOST_FOREACH(const ptree::value_type &v, rules) {
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
	const ptree &ents = pt.get_child("config.entities", empty_ptree());
	BOOST_FOREACH(const ptree::value_type &v, ents) {
		dix.loadEntities_XML(v.second.data().c_str());
	}
/*	} catch(boost::property_tree::ptree_bad_path &e) {
		std::cerr << "WARNING: entities section not found in config\n";
	} //*/
}


void BarzerSettings::loadSpell(StoredUniverse &u, const ptree &node)
{
	BarzerHunspell& hunspell = u.getHunspell();

	//const boost::optional<ptree &> spell_opt = node.get_child_optional("spell");
	const ptree &spell = node.get_child("spell", empty_ptree());
	if (spell.empty()) {
		std::cout << "No <spell> tag\n";
		return;
	}

	try {
		//const ptree &spell = node.get_child("spell");
		//const ptree &spell = spell_opt.get();
		const ptree &attrs = spell.get_child("<xmlattr>");

		const std::string &affx = attrs.get<std::string>("affx"),
			              &dict = attrs.get<std::string>("maindict");

		std::cerr << "initializing hunspell with "
				  << affx << " and " << dict << std::endl;

		hunspell.initHunspell(affx.c_str(), dict.c_str());

		BOOST_FOREACH(const ptree::value_type &v, spell) {
			const std::string& tagName = v.first;
			const char* tagVal = v.second.data().c_str();
			if( tagName == "dict" ) {
				std::cerr << "adding dictionary " << tagVal << std::endl;
				hunspell.addDictionary( tagVal );
			} else if( tagName == "extra" ) {
				std::cerr << "adding extra file " << tagVal << "... " <<
				hunspell.addWordsFromTextFile( tagVal ) << " words added\n";
		   }
		}
	} catch (boost::property_tree::ptree_bad_path &e) {
		AYLOG(ERROR) << "Can't get " << e.what();
		return;
	}
}

/*
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
	try {

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
	catch(...) {
		std::cerr << "WARNING: Spelling info not found\n" ;
	}
}
*/

void BarzerSettings::loadTrieset(StoredUniverse &u, const ptree &node) {
	BOOST_FOREACH(const ptree::value_type &v, node.get_child("trieset", empty_ptree())) {
		if (v.first == "trie") {
			const ptree &trie = v.second;
			try {
				const ptree &attrs = trie.get_child("<xmlattr>");
				const std::string &cl = attrs.get<std::string>("class"),
								  &name = attrs.get<std::string>("name");
				u.getTrieCluster().appendTrie(cl, name);
			} catch (boost::property_tree::ptree_bad_path &e) {
				AYLOG(ERROR) << "Can't get " << e.what();
			}
		}
	}
}

void BarzerSettings::loadUser(const ptree::value_type &user) {
	const ptree &children = user.second;
	const boost::optional<uint32_t> userId
		= children.get_optional<uint32_t>("<xmlattr>.id");

	if (!userId) {
		AYLOG(ERROR) << "No user id in tag <user>";
		return;
	}

	StoredUniverse &u = gpools.produceUniverse(userId.get());
	std::cout << "Loading user id: " << userId << "\n";
	loadTrieset(u, children);
	loadSpell(u, children);
}

void BarzerSettings::loadUsers() {
	BOOST_FOREACH(ptree::value_type &v,
			pt.get_child("config.users", empty_ptree())) {
		if (v.first == "user") loadUser(v);
	}
}

void BarzerSettings::load(const char *fname) {
	//AYLOGDEBUG(fname);
	std::cout << "Loading config file: " << fname << std::endl;
	try {
		read_xml(fname, pt);
		//loadSpell();
		loadEntities();
		loadRules();
		loadUsers();
	} catch (boost::property_tree::xml_parser_error &e) {
		AYLOG(ERROR) << e.what();
	}
}

const std::string BarzerSettings::get(const char *key) const {
	return pt.get<std::string>(key);
}

const std::string BarzerSettings::get(const std::string &key) const {
	return get(key.c_str());
}

}
