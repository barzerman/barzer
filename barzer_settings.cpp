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
#include <barzer_bzspell.h>

using boost::property_tree::ptree;
namespace fs = boost::filesystem;

static ptree& empty_ptree() {
	static ptree pt;
	return pt;
}

namespace barzer {

User::Spell::Spell(User *u, const char *md, const char *a)
	: user(u), hunspell(&(u->getUniverse().getHunspell())), maindict(md), affx(a)
{
	std::cout << "initializing hunspell with "
			  << a << " and " << md << "\n";
	hunspell->initHunspell(md, a);

}

void User::Spell::addExtra(const char *path)
{
	std::cout << "adding extra file " << path << "... " <<
			hunspell->addWordsFromTextFile(path) << " words added\n";
	vec.push_back(Rec(EXTRA, path));

} //*/
void User::Spell::addDict(const char *path)
{
	std::cout << "adding dictionary " << path << std::endl;
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

void User::addTrie(const TriePath &tp) {
	tries.push_back(tp);
	universe.getTrieCluster().appendTrie(tp.first, tp.second);
}


User& BarzerSettings::createUser(User::Id id) {
	UserMap::iterator it = umap.find(id);
	if (it == umap.end()) {
		std::cout << "Adding user id(" << id << ")\n";
		return umap.insert(UserMap::value_type(id, User(id, *this))).first->second;
	} else return it->second;

}


User*  BarzerSettings::getUser(User::Id id)
{
	UserMap::iterator it = umap.find(id);
	return (it == umap.end() ? 0 : &(it->second));
}

BarzerSettings::BarzerSettings(GlobalPools &gp)
	: gpools(gp), reader(gp), d_numThreads(0), d_currentUniverse(0)
{ 
	init(); 
}

void BarzerSettings::addRulefile(const Rulefile &f) {
	const std::string &tclass = f.trie.first,
					  &tid = f.trie.second;
	const char *fname = f.fname;
	reader.setCurrentUniverse( getCurrentUniverse() );
	reader.setTrie(tclass, tid);

	size_t num = reader.loadFromFile(fname, BELReader::INPUT_FMT_XML);
	std::cout << num << " statements (" << reader.getNumMacros() << " macros, " << 
	reader.getNumProcs() << " procs)" << " loaded from `" << fname << "'";
	if (!(tclass.empty() || tid.empty()))
		std::cout << " into the trie `" << tclass << "." << tid << "'";
	std::cout << "\n";
}

void BarzerSettings::addEntityFile(const char *fname)
{
	entityFiles.push_back(fname);
	gpools.getDtaIdx().loadEntities_XML(fname);
}

StoredUniverse* BarzerSettings::setCurrentUniverse(User& u ) 
{
	d_currentUniverse = gpools.getUniverse(u.getId());
	return d_currentUniverse;
}
StoredUniverse* BarzerSettings::getCurrentUniverse() 
{
	if( !d_currentUniverse )
		d_currentUniverse = gpools.getUniverse(0);
	return d_currentUniverse;
}

void BarzerSettings::init() {
	BarzerDate::initToday();

}

void BarzerSettings::loadRules(const boost::property_tree::ptree& rules) 
{
	if (rules.empty()) {
		// warning goes here
		return;
	}
	BOOST_FOREACH(const ptree::value_type &v, rules) {
		if (v.first == "file") {
			const ptree &file = v.second;
			const char *fname =  file.data().c_str();
			try {
				const ptree &attrs = file.get_child("<xmlattr>");
				const std::string &cl = attrs.get<std::string>("class"),
			                  	  &id = attrs.get<std::string>("id");
				addRulefile(Rulefile(fname, cl, id));
			} catch (boost::property_tree::ptree_bad_path&) {
				addRulefile(Rulefile(fname));
			}

		} else {
			AYLOG(ERROR) << "Unknown tag /rules/" << v.first;
		}
	}
}

void BarzerSettings::loadRules() 
{
	using boost::property_tree::ptree;

	StoredUniverse *u = getCurrentUniverse();
	if (!u) return;
	const ptree &rules = pt.get_child("config.rules", empty_ptree());
	if (rules.empty()) {
		// warning goes here
		return;
	}
	loadRules(rules);
}


void BarzerSettings::load() {
	//AYLOG(DEBUG) << "BarzerSettings::load()";
	load(DEFAULT_CONFIG_FILE);
}


void BarzerSettings::loadInstanceSettings() {
	const ptree &rules = pt.get_child("config.instance", empty_ptree());
	BOOST_FOREACH(const ptree::value_type &v, rules) {
		const std::string& tagName = v.first;
		const std::string& text =  v.second.data();
		if( tagName == "threads" )  {
			size_t nt = atoi( text.c_str() );
				
			std::cerr << "EXTRA THREADS: " << nt << " created" << std::endl;
			if( !nt )
				nt = 1;
			if( nt > MAX_REASONABLE_THREADCOUNT ) {
				AYLOG(ERROR) << "threadcoutn of " << nt << " exceeds " <<  MAX_REASONABLE_THREADCOUNT << std::endl;
			}
			setNumThreads( nt );
		}
	}
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


void BarzerSettings::loadSpell(User &u, const ptree &node)
{
	const ptree &spell = node.get_child("spell", empty_ptree());
	if (spell.empty()) {
		std::cout << "No <spell> tag\n";
		return;
	}
	try {
		// here we can add some fancier secondary spellchecker  logic
		// for now all users that are not 0 will get 0 passed in
		const GlobalPools& gp = u.getUniverse().getGlobalPools();

		const StoredUniverse* secondaryUniverse = ( u.getId() ? gp.getDefaultUniverse() : 0 );
		BZSpell* bzs = u.getUniverse().initBZSpell( secondaryUniverse );
		BOOST_FOREACH(const ptree::value_type &v, spell) {
			const std::string& tagName = v.first;
			const char* tagVal = v.second.data().c_str();
			if( tagName == "extra" ) {
				bzs->loadExtra( tagVal );
			}
		}
		if( bzs ) {
			bzs->init( secondaryUniverse );
			bzs->printStats( std::cerr );
		}
		else {
			AYLOG(ERROR) << "Null BZSpell object for user id " << u.getId() << std::endl;
		}
	} catch (boost::property_tree::ptree_bad_path &e) {
		AYLOG(ERROR) << "Can't get " << e.what();
		return;
	}
}

void BarzerSettings::loadHunspell(User &u, const ptree &node)
{
	const ptree &spell = node.get_child("spell", empty_ptree());
	if (spell.empty()) {
		std::cout << "No <spell> tag\n";
		return;
	}

	try {
		const ptree &attrs = spell.get_child("<xmlattr>");

		const std::string &affx = attrs.get<std::string>("affx"),
			              &dict = attrs.get<std::string>("maindict");

		User::Spell &s = u.createSpell(affx.c_str(), dict.c_str());

		BOOST_FOREACH(const ptree::value_type &v, spell) {
			const std::string& tagName = v.first;
			const char* tagVal = v.second.data().c_str();
			if( tagName == "dict" ) {
				s.addDict( tagVal );
			} else if( tagName == "extra" ) {
				s.addExtra( tagVal );
		   }
		}
	} catch (boost::property_tree::ptree_bad_path &e) {
		AYLOG(ERROR) << "Can't get " << e.what();
		return;
	}
}

void BarzerSettings::loadTrieset(User &u, const ptree &node) {
	BOOST_FOREACH(const ptree::value_type &v, node.get_child("trieset", empty_ptree())) {
		if (v.first == "trie") {
			const ptree &trie = v.second;
			try {
				const ptree &attrs = trie.get_child("<xmlattr>");
				const std::string &cl = attrs.get<std::string>("class"),
								  &name = attrs.get<std::string>("name");
				u.addTrie(TriePath(cl, name));
			} catch (boost::property_tree::ptree_bad_path &e) {
				AYLOG(ERROR) << "Can't get " << e.what();
			}
		}
	}
}

void BarzerSettings::loadUserRules(User& u, const ptree &node )
{
	try {
		const ptree &rules = node.get_child("rules", empty_ptree());
		setCurrentUniverse(u);
		loadRules( rules );
	} catch (...) {
		
	}
}
void BarzerSettings::loadUser(const ptree::value_type &user) 
{
	const ptree &children = user.second;
	const boost::optional<uint32_t> userId
		= children.get_optional<uint32_t>("<xmlattr>.id");

	if (!userId) {
		AYLOG(ERROR) << "No user id in tag <user>";
		return;
	}

	User &u = createUser(userId.get());

	std::cout << "Loading user id: " << userId << "\n";

	loadUserRules(u,children);
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


		loadInstanceSettings();
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




