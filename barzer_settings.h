/*
 * barzer_settings.h
 *
 *  Created on: May 9, 2011
 *      Author: polter
 */

#ifndef BARZER_SETTINGS_H_
#define BARZER_SETTINGS_H_


#include <map>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

#include <barzer_config.h>
#include <barzer_el_parser.h>
#include <barzer_spell.h>

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


typedef std::pair<const char*, const char*> TriePath;
class BarzerSettings;
struct User {
	typedef uint32_t Id;
	struct Spell {
		enum {DICT, EXTRA};
		typedef std::pair<uint8_t,const char*> Rec;
		typedef std::vector<Rec> Vec;

		User *user;
		BarzerHunspell *hunspell;
		const char *maindict;
		const char *affx;
		Vec vec;

		Spell(User *u, const char *md, const char *a);
		void addExtra(const char *path);
		void addDict(const char *path);

	};
	typedef std::vector<TriePath> TrieVec;
	Id id;
	boost::optional<Spell> spell;
	TrieVec tries;
	BarzerSettings &settings;
	StoredUniverse &universe;

	User(Id i, BarzerSettings &s);

	TrieVec& getTries() { return tries; }
	const TrieVec& getTries() const { return tries; }

	void addTrie(const char *cl, const char *id);

	Spell* getSpell() { return spell.get_ptr(); }
	Spell& createSpell(const char *md, const char *affx);


	StoredUniverse& getUniverse() { return universe; }
	const StoredUniverse& getUniverse() const { return universe; }

};

struct Rulefile {
	TriePath trie;
	const char *fname;
	Rulefile(const char *fn, const char *tclass = 0, const char *tid = 0)
		: trie(TriePath(tclass, tid)), fname(fn) {}
};

struct Logging {
	uint8_t level;
	const char *logfile;
	Logging (uint8_t l = 0, const char *f = 0) : level(l), logfile(f) {}
};

class GlobalPools;
class BarzerSettings {
public:
	typedef std::map<User::Id, User> UserMap;
	typedef std::vector<Rulefile> Rules;
	typedef std::vector<const char*> EntityFileVec;
private:
	GlobalPools &gpools;
	//StoredUniverse &universe;
	BELReader reader;

	boost::property_tree::ptree pt;

	ParseSettings d_parseSettings;
	UserMap umap;
	Rules rules;
	EntityFileVec entityFiles;
	Logging logging;
public:
	const ParseSettings& parseSettings() const { return d_parseSettings; } 
		  ParseSettings& parseSettings() 	    { return d_parseSettings; } 

	StoredUniverse* getCurrentUniverse() ;

	//BarzerSettings(StoredUniverse&, const char*);
	//BarzerSettings(StoredUniverse&);
	BarzerSettings(GlobalPools&);


	void init();

	void loadRules();
	void loadParseSettings();
	void loadEntities();
	///loads spellchecker related stuff (hunspell dictionaries, extra word lists and such)
	void loadSpell(StoredUniverse&, const boost::property_tree::ptree&);

	void loadTrieset(StoredUniverse&, const boost::property_tree::ptree&);

	void loadUsers();
	void loadUser(const boost::property_tree::ptree::value_type &);

	User& createUser(User::Id id);
	User* getUser(User::Id id);

	void addRulefile(const char *fn);
	void addRulefile(const char *fn, const char *tclass, const char *tid);

	void addEntityFile(const char*);
	void setLogging(const Logging&);

	void load();
	void load(const char *fname);

	GlobalPools& getGlobalPools() { return gpools; }
	const GlobalPools& getGlobalPools() const { return gpools; }

	const std::string get(const char*) const;
	const std::string get(const std::string&) const;

};

}
#endif /* BARZER_SETTINGS_H_ */
