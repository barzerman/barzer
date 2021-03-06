
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
/*
 * barzer_settings.h
 *
 *  Created on: May 9, 2011
 *      Author: polter
 */

#pragma once
#include <map>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

#include <barzer_config.h>
#include <barzer_el_parser.h>
#include <barzer_server_response.h>

#include <ay/ay_logger.h>
#include <ay/ay_bitflags.h>

namespace barzer {
struct GrammarInfo;
/// parsing settings true for all users in the instance
struct ParseSettings {

	// when true everything gets stemmed both on load and on input
	// by default. in order to reduce the load time individual rulesets can
	// set presume stemmed
	bool d_stem;
	ParseSettings() :
		d_stem(true)
	{}

	bool stemByDefault() const { return d_stem; }
	void set_stemByDefault() { d_stem = true; }
};


typedef std::pair<std::string, std::string> TriePath;
class BarzerSettings;
struct User {
	typedef uint32_t Id;
	typedef std::vector<TriePath> TrieVec;
	Id id;
    std::vector< std::string > extraDictFileNameVec;
	// TrieVec tries;
	BarzerSettings &settings;
	StoredUniverse &universe;
    
    /// optional map for translating entity class/subclass
    /// notice: neither subclasses for this user's class nor 
    /// for the 0 (root) user are translated
    std::map< StoredEntityClass, StoredEntityClass > d_entTranslationMap;
    
    /// returns 0 if translation was successful 
    int addEntTranslation( const char* src, const char* dest ) ;
    void addEntTranslation( StoredEntityClass from, StoredEntityClass to ) 
        { d_entTranslationMap[ from ] = to; }
    StoredEntityClass getEntTranslation( const StoredEntityClass & from ) const
    {
        const auto x = d_entTranslationMap.find( from );
        return ( x== d_entTranslationMap.end() ? from: x->second );
    }

    uint32_t getDefaultEntityClass() const 
        { return id; }
    
	User(Id i, BarzerSettings &s);

	// TrieVec& getTries() { return tries; }
	// const TrieVec& getTries() const { return tries; }

	//void addTrie(const std::string&, const std::string&);
	void addTrie(const TriePath&, bool isTopicTrie, GrammarInfo* );

	uint32_t getId() const { return id;}

	StoredUniverse& getUniverse() { return universe; }
	const StoredUniverse& getUniverse() const { return universe; }
	StoredUniverse* getUniversePtr() { return &universe; }
	const StoredUniverse* getUniversePtr() const { return &universe; }
    int readClassNames( const boost::property_tree::ptree& node ) ;

    int loadExtraDictionary();
};

struct Rulefile {
	TriePath trie; /// pair(string,string)
    std::string extraDictionaryPath; // dict attribute in the file tag within rules

	std::string fname;
    Rulefile() {}
	Rulefile(const std::string fn, const std::string &tclass = "", const std::string &tid = "")
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
	// typedef std::vector<const char*> EntityFileVec;
    enum { 
        ENVPATH_BARZER_HOME,  // default always reads from BARZER_HOME if its set
        ENVPATH_AUTO,         // -home  when cfg file path is absolute assumes BARZER_HOME path, otherwise current dir
        ENVPATH_ENV_IGNORE     // -home curdir always assumes current dir
    };
private:
	GlobalPools &gpools;

	boost::property_tree::ptree pt;

	ParseSettings d_parseSettings;
	UserMap umap;
	Rules rules;
	// EntityFileVec entityFiles;
	// EntityFileVec dictionaryFiles;
	Logging logging;

	/// this makes settings not thread safe

	enum {  MAX_REASONABLE_THREADCOUNT = 8 };
	size_t d_numThreads;
	StoredUniverse* d_currentUniverse;

    int d_envPath; // ENVPATH_XXX (ENVPATH_BARZER_HOME default)
    std::string d_homeOverrideStr; /// when non blank thisis taken over the value of BARZER_HOME

public:
    bool   isEnvPath_BARZER_HOME() const { return (d_envPath== ENVPATH_BARZER_HOME); }
    bool   isEnvPath_AUTO() const { return (d_envPath== ENVPATH_AUTO); }
    bool   isEnvPath_ENV_IGNORE() const { return (d_envPath== ENVPATH_ENV_IGNORE); }

    void   setEnvPath_BARZER_HOME(){ d_envPath= ENVPATH_BARZER_HOME; }
    void   setEnvPath_AUTO() { d_envPath= ENVPATH_AUTO; }

    void   setEnvPath_ENV_IGNORE( const char* path ) { 
        d_envPath= ENVPATH_ENV_IGNORE; 
        d_homeOverrideStr.assign( path );
    }
    const std::string& getHomeOverridePath( ) const { return d_homeOverrideStr; }

	size_t getNumThreads() const { return d_numThreads; }
	void   setNumThreads( size_t n ) { d_numThreads = n; }

	const ParseSettings& parseSettings() const { return d_parseSettings; }
		  ParseSettings& parseSettings() 	    { return d_parseSettings; }

	const StoredUniverse* getCurrentUniverse() const
        { return d_currentUniverse; }
	StoredUniverse* getCurrentUniverse() ;
	StoredUniverse* setCurrentUniverse( User& u );

	BarzerSettings(GlobalPools&, std::ostream* os);

	void init();

    /// expects a list of newline separated config file names (# is for comment)
    /// will load the configs sequentially (passing them to BarzerSettings::load)
    int loadListOfConfigs(BELReader& reader, const char *fname) ;

	void loadInstanceSettings();
	void loadRules(BELReader&);
	void loadRules(BELReader&, const boost::property_tree::ptree&);
	void loadParseSettings();
	void loadEntities();
	/// global dictionaries from <dictionaries> tag
	void loadLangNGrams();
	void loadDictionaries();
	void loadMeanings(User&, const boost::property_tree::ptree&);
	void loadSpell(User&, const boost::property_tree::ptree&);

	void loadTrieset(BELReader&, User&, const boost::property_tree::ptree&);
    std::string getDefaultClassName() const ;
        
	void loadLocale(BELReader&, User&, const boost::property_tree::ptree&);
	void loadUserRules(BELReader& reader, User&, const boost::property_tree::ptree&);

	void loadUsers(BELReader& reader );
	int loadUser(BELReader& reader, const boost::property_tree::ptree::value_type &, const char* uname=0 );

    int loadUserConfig( BELReader&, const char* cfgFileName, const char* uname=0 );

	User& createUser(User::Id id, const char* uname =0);
	User* getUser(User::Id id);

	void addRulefile(BELReader&, const Rulefile &f);

	void addEntityFile(const char*);
	/// file to global dictionary <dictionaries> tag
	void addDictionaryFile(const char*);
	void setLogging(const Logging&);

	void load(BELReader& reader );
	void load(BELReader& reader, const char *fname);

	GlobalPools& getGlobalPools() { return gpools; }
	const GlobalPools& getGlobalPools() const { return gpools; }

	const std::string get(const char*) const;
	const std::string get(const std::string&) const;

};

} // namespace barzer
