/*
 * barzer_settings.cpp
 *
 *  Created on: May 9, 2011
 *      Author: polter
 */

#include <ay/ay_logger.h>
#include <barzer_settings.h>
#include <barzer_universe.h>
#include <barzer_basic_types.h>

#include <boost/foreach.hpp>
#include <boost/filesystem.hpp>
#include <time.h>
#include <cstdlib>

using boost::property_tree::ptree;
namespace fs = boost::filesystem;

static ptree& empty_ptree() {
	static ptree pt;
	return pt;
}

namespace barzer {

User::Spell::Spell(User *u, const char *md, const char *a)
	: user(u), hunspell(&(u->getUniverse().getHunspell())), maindict(md), affx(a) {}

void User::Spell::addExtra(const char *path)
{
	vec.push_back(Rec(EXTRA, path));
	hunspell->addWordsFromTextFile(path);
} //*/
void User::Spell::addDict(const char *path)
{
	vec.push_back(Rec(DICT, path));
	hunspell->addDictionary(path);
} //*/


User::User(Id i, BarzerSettings &s)
	: id(i), settings(s), universe(s.getGlobalPools().produceUniverse(i))
	{}

User::Spell& User::createSpell(const char *md, const char *affx)
{
	spell = Spell(this, md, affx);
	return spell.get();
}

void User::addTrie(const char *cl, const char *id) {
	tries.push_back(TriePath(cl, id));
	universe.getTrieCluster().appendTrie(cl, id);
}


User& BarzerSettings::createUser(User::Id id) {
	UserMap::iterator it = umap.find(id);
	if (it == umap.end()) {
		return umap.insert(UserMap::value_type(id, User(id, *this))).first->second;
	} else return it->second;

}


User*  BarzerSettings::getUser(User::Id id)
{
	UserMap::iterator it = umap.find(id);
	return (it == umap.end() ? 0 : &(it->second));
}

BarzerSettings::BarzerSettings(GlobalPools &gp)
	: gpools(gp), reader(gp)
{ init(); }


void BarzerSettings::addRulefile(const char *fn, const char *tclass = "", const char *tid = "")
{
	rules.push_back(Rulefile(fn, tclass, tid));
	reader.setTrie(tclass, tid);
	size_t num = reader.loadFromFile(fn, BELReader::INPUT_FMT_XML);
	std::cout << num << " statements loaded from `" << fn << "'\n";
}


void BarzerSettings::addEntityFile(const char *fname)
{
	entityFiles.push_back(fname);
	gpools.getDtaIdx().loadEntities_XML(fname);
}

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
	//BELReader r(&(u->getBarzelTrie()), *u);
	//BELReader r(gpools);
	BOOST_FOREACH(const ptree::value_type &v, rules) {
		if (v.first == "file") {
			const ptree &file = v.second;
			const char *fname =  file.data().c_str();
			try {
				const ptree &attrs = file.get_child("<xmlattr>");
				const std::string &cl = attrs.get<std::string>("class"),
			                  	  &id = attrs.get<std::string>("id");
				reader.setTrie(cl, id);
				std::cout << "loading `" << fname << "' into trie ("
						  << cl << "." << id << ")\n";
			} catch (boost::property_tree::ptree_bad_path&) {
				reader.setTrie("", "");
				std::cout << "loading `" << fname << "' into default trie\n";
			}

			int num = reader.loadFromFile(fname, BELReader::INPUT_FMT_XML);
			std::cout << num << " statements loaded from `" << fname << "'\n";
		} else {
			AYLOG(ERROR) << "Unknown tag /rules/" << v.first;
		}
	}
}


void BarzerSettings::load() {
	//AYLOG(DEBUG) << "BarzerSettings::load()";
	load(DEFAULT_CONFIG_FILE);
}


void BarzerSettings::loadParseSettings() {
	const ptree &rules = pt.get_child("config.parse", empty_ptree());
	//if( !rules.empty() ) { dont need this as empty ptree means 0 foreach iterations
	StoredUniverse *u = getCurrentUniverse();
	BOOST_FOREACH(const ptree::value_type &v, rules) {
		const std::string& tagName = v.first;
		const std::string& text =  v.second.data();
		if( tagName == "stem" ) {
			if( text != "NO" ) {
				u->getGlobalPools().parseSettings().set_stemByDefault();
				std::cerr << ">>>>> STEMMING MODE\n";
			} else
				std::cerr << ">>>>> no stemming\n";
		}

	}
	//}
}
void BarzerSettings::loadEntities() {
	using boost::property_tree::ptree;
	//DtaIndex &dix = gpools.getDtaIndex();
	const ptree &ents = pt.get_child("config.entities", empty_ptree());
	BOOST_FOREACH(const ptree::value_type &v, ents) {
		//dix.loadEntities_XML(v.second.data().c_str());
		addEntityFile(v.second.data().c_str());
	}
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
	fs::path oldPath = fs::current_path();

	try {
		read_xml(fname, pt);

		fs::path oldPath = fs::current_path();
		const char *dataPath = std::getenv("BARZER_HOME");
		if (dataPath) {
			fs::current_path(dataPath);
		}


		loadParseSettings();
		loadEntities();
		loadRules();
		loadUsers();

		fs::current_path(oldPath);
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




