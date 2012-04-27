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

User::User(Id i, BarzerSettings &s)
	: id(i), settings(s), universe(s.getGlobalPools().produceUniverse(i))
	{}

void User::addTrie(const TriePath &tp, bool isTopicTrie, GrammarInfo* gramInfo ) {
    if( isTopicTrie )
	    universe.appendTopicTrie(tp.first.c_str(), tp.second.c_str(), gramInfo );
    else
	    universe.appendTrie(tp.first.c_str(), tp.second.c_str(), gramInfo );
}


User& BarzerSettings::createUser(User::Id id) {
	UserMap::iterator it = umap.find(id);
	if (it == umap.end()) {
		std::cout << "Adding user id(" << id << ")\n";
        if( !gpools.getUniverse(id) ) {
            std::cerr << "Creating universe for " << id << std::endl;
            gpools.produceUniverse( id );
        } else
            std::cerr << "Pre-existing universe for " << id << std::endl;
		return umap.insert(UserMap::value_type(id, User(id, *this))).first->second;
	} else return it->second;

}


User*  BarzerSettings::getUser(User::Id id)
{
	UserMap::iterator it = umap.find(id);
	return (it == umap.end() ? 0 : &(it->second));
}

BarzerSettings::BarzerSettings(GlobalPools &gp, std::ostream* os ) : 
    gpools(gp), d_numThreads(0), d_currentUniverse(0),d_envPath(ENVPATH_BARZER_HOME)
{
	init();
}

void BarzerSettings::addRulefile(BELReader& reader, const Rulefile &f)
{
	const std::string &tclass = f.trie.first,
					  &tid = f.trie.second;
	const char *fname = f.fname;
	reader.setCurrentUniverse( getCurrentUniverse() );

    uint32_t trieClass = gpools.internString_internal(tclass.c_str()) ;
    uint32_t trieId = gpools.internString_internal(tid.c_str()) ;

	reader.setTrie(trieClass, trieId );

	size_t num = reader.loadFromFile(fname, BELReader::INPUT_FMT_XML);
	std::cout << num << " statements";
        if (num) std::cout <<"(" << reader.getNumMacros() << " macros, " <<
                reader.getNumProcs() << " procs)";
        std::cout << " loaded from `" << fname ;
	if (!(tclass.empty() || tid.empty()))
		std::cout << " into the trie `" << tclass << "." << tid << "'";
	std::cout << "\n";
}

void BarzerSettings::addDictionaryFile(const char *fname)
{
	std::cerr << "Reading dictionary from " << fname << " ... " << gpools.readDictionaryFile( fname ) << "read\n";

}
void BarzerSettings::addEntityFile(const char *fname)
{
	// entityFiles.push_back(fname);
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

void BarzerSettings::loadRules(BELReader& reader, const boost::property_tree::ptree& rules)
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
			                  	  &id = attrs.get<std::string>("name");
                uint32_t trieClass = gpools.internString_internal(cl.c_str()) ;
                uint32_t trieId = gpools.internString_internal(id.c_str()) ;

				reader.setCurTrieId( trieClass, trieId );
				addRulefile(reader, Rulefile(fname, cl, id));
			} catch (boost::property_tree::ptree_bad_path&) {
				reader.clearCurTrieId();
				addRulefile(reader,Rulefile(fname));
			}

		} else {
			// AYLOG(ERROR) << "Unknown tag /rules/" << v.first;
		}
	}
}

void BarzerSettings::loadRules(BELReader& reader)
{
	using boost::property_tree::ptree;

	StoredUniverse *u = getCurrentUniverse();
	if (!u) {
        std::cerr << "Current universe is blank\n";
        return;
    }
	const ptree &rules = pt.get_child("config.rules", empty_ptree());
	if (rules.empty()) {
		// warning goes here
		return;
	}
	loadRules(reader,rules);
}


void BarzerSettings::load(BELReader& reader ) {
	//AYLOG(DEBUG) << "BarzerSettings::load()";
	load(reader,DEFAULT_CONFIG_FILE);
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
void BarzerSettings::loadDictionaries() {

	try {
	using boost::property_tree::ptree;
	const ptree &ents = pt.get_child("config.dictionaries", empty_ptree());
	BOOST_FOREACH(const ptree::value_type &v, ents) {
		addDictionaryFile(v.second.data().c_str());

	}
	} catch(...) {
	}
}

void BarzerSettings::loadEntities() {
	using boost::property_tree::ptree;
	const ptree &ents = pt.get_child("config.entities", empty_ptree());
	BOOST_FOREACH(const ptree::value_type &v, ents) {
        if( v.first == "prop" ) {
            const char* propFileName = v.second.data().c_str() ;
            gpools.entData.readFromFile( gpools, propFileName );
        } else
		    addEntityFile(v.second.data().c_str());
	}
}


void BarzerSettings::loadSpell(User &u, const ptree &node)
{
	const ptree &spell = node.get_child("spell", empty_ptree());
    /*
	if (spell.empty()) {
		std::cout << "No <spell> tag\n";
		return;
	}
    */
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

void BarzerSettings::loadTrieset(BELReader& reader, User &u, const ptree &node) {
	BOOST_FOREACH(const ptree::value_type &v, node.get_child("trieset", empty_ptree())) {
		if (v.first == "trie") {
			const ptree &trieNode = v.second;
			try {
				const ptree &attrs = trieNode.get_child("<xmlattr>");
				const std::string &cl = attrs.get<std::string>("class"),
								  &name = attrs.get<std::string>("name");
                const boost::optional<std::string> optTopic = attrs.get_optional<std::string>("topic");

                // noac - autocomplete doesnt apply . optional attribute. when set
                // when this attribute is set the grammar wont be used in autocomplete
                const boost::optional<std::string> optNoAutoc = attrs.get_optional<std::string>("noac");

				std::cerr << "user " << u.getId() << " added trie (" << cl << ":" << name << ") " << (optTopic? "TOPIC" :"") << "\n";

                GrammarInfo* gramInfo = 0;
                /// extracting topic info
                BOOST_FOREACH(const ptree::value_type &uv, trieNode.get_child("topic", empty_ptree())) {
                    const ptree &topicAttr = uv.second;

                    boost::optional<uint32_t> classOpt = topicAttr.get_optional<uint32_t>("c");
                    boost::optional<uint32_t> subclassOpt = topicAttr.get_optional<uint32_t>("s");
                    boost::optional<std::string> idOpt = topicAttr.get_optional<std::string>("i");
                    boost::optional<int> weightOpt = topicAttr.get_optional<int>("w");

                    if( classOpt ) {
                        if( !gramInfo ) {
                            gramInfo = new GrammarInfo();

                        }
                        const char* idStr = (idOpt? (*idOpt).c_str():0);
                        uint32_t idStrId = ( idStr ? gpools.internString_internal(idStr): 0xffffffff );
                        const StoredEntity& ent = gpools.getDtaIdx().addGenericEntity( idStrId, *classOpt, ( subclassOpt? *subclassOpt: 0) );
                        gramInfo->trieTopics.mustHave( ent.getEuid(), (weightOpt? *weightOpt: BarzTopics::DEFAULT_TOPIC_WEIGHT) );
                    }
                }
                if( gramInfo && optNoAutoc) {
                    gramInfo->set_autocDontApply();
                }
				u.addTrie(TriePath(cl, name), optTopic, gramInfo);

			} catch (boost::property_tree::ptree_bad_path &e) {
				AYLOG(ERROR) << "Can't get " << e.what();
			}
		}
	}
	const UniverseTrieCluster& tcluster = u.getUniverse().getTrieCluster();
	std::cerr << "user " << u.getId() << ":" << tcluster.getTrieList().size()	 << " tries loaded\n";
}

void BarzerSettings::loadLocale(BELReader& reader, User& u, const ptree& node)
{
	bool metDefault = false;
	BOOST_FOREACH(const ptree::value_type &v, node.get_child("locales", empty_ptree()))
	{
		if (v.first != "locale")
			continue;

		try
		{
			const ptree& trieNode = v.second;
			const ptree& attrs = trieNode.get_child("<xmlattr>");
			const std::string& lang = attrs.get<std::string>("lang");

			const bool def = attrs.get<bool>("default", false);
			if (def)
				metDefault = true;

			AYLOG(DEBUG) << "detected locale" << lang << " " << def << std::endl;

			u.getUniverse().addLocale(BarzerLocale::makeLocale(lang), def);
		}
		catch (const boost::property_tree::ptree_bad_path &e)
		{
			AYLOG(ERROR) << "can't get " << e.what();
		}
	}

	if (!metDefault)
	{
		std::cerr << "*** Using English locale" << std::endl;
		u.getUniverse().addLocale(BarzerLocale::makeLocale("en"), true);
	}
}

void BarzerSettings::loadUserRules(BELReader& reader, User& u, const ptree &node )
{
	try {
		const ptree &rules = node.get_child("rules", empty_ptree());
		setCurrentUniverse(u);
	    if (rules.empty()) {
            std::cerr << "rules empty!!!!\n";
        } else
		    loadRules( reader, rules );
	} catch (...) {
		std::cerr << "user rules exception\n";
	}
}
namespace {
void load_ent_segregate_info(BELReader& reader, User& u, const ptree &node)
{
    try {
        const ptree &entseg = node.get_child("entseg", empty_ptree());
        if( entseg.empty() )
            return;
        BOOST_FOREACH(const ptree::value_type &v, entseg ) {
            StoredEntityClass eclass;
            StoredUniverse& uni = u.universe;
            const boost::optional<uint32_t> ec = v.second.get_optional<uint32_t>("c"),
                                sc = v.second.get_optional<uint32_t>("sc");
            if( ec ) {
                eclass.setClass(*ec);
                if( sc )
                    eclass.setSubclass(*sc);

                uni.addEntClassToSegregate( eclass );
            }
        } // foreach
    } catch(...) {
        std::cerr << "load_ent_segregate_info exception\n";
    }
}

} // anonymous namespace ends

void BarzerSettings::loadUser(BELReader& reader, const ptree::value_type &user)
{
	const ptree &children = user.second;
	const boost::optional<uint32_t> userIdOpt
		= children.get_optional<uint32_t>("<xmlattr>.id");

	if (!userIdOpt) {
		AYLOG(ERROR) << "No user id in tag <user>";
		return;
	}
    uint32_t userId = userIdOpt.get();

	User &u = createUser(userId);

	std::cout << "Loading user id: " << userId << "\n";

    reader.setRespectLimits( userId );
    reader.setCurrentUniverse( u.getUniversePtr() );
    load_ent_segregate_info(reader, u, children);
	loadUserRules(reader, u, children);
	loadTrieset(reader, u, children);
	loadLocale(reader, u, children);
	loadSpell(u, children);

	StoredUniverse& uni = u.getUniverse();
	uni.getBarzHints().initFromUniverse(&uni);
}

int BarzerSettings::loadUserConfig( BELReader& reader, const char* cfgFileName ) {
    if( !gpools.getUniverse(0) ) {
        User &u = createUser(0);
        BZSpell* bzs = u.getUniverse().initBZSpell( 0 );
        bzs->init( 0 );
    }
    boost::property_tree::ptree userPt;
    int numUsersLoaded = 0;
    try {
        read_xml(cfgFileName, userPt);
        BOOST_FOREACH(const ptree::value_type &userV, userPt.get_child("config.users")) {
            if (userV.first == "user") {
                loadUser( reader, userV );
                ++numUsersLoaded;
            }
        }
	} catch (boost::property_tree::xml_parser_error &e) {
		reader.getErrStreamRef() << "<error>" << e.what() << "</error>" << std::endl;
    }
    return numUsersLoaded;
}

void BarzerSettings::loadUsers(BELReader& reader ) {
	if( !gpools.getUniverse(0)) { // hack user 0 must be initialized
	    User &u = createUser(0);
	    BZSpell* bzs = u.getUniverse().initBZSpell( 0 );
	    bzs->init( 0 );
	}

	BOOST_FOREACH(ptree::value_type &v, pt.get_child("config.users", empty_ptree())) {
        /// checking whether v has cfgfile attribute
        if( v.first == "user" ) {
            const std::string &cfgFileName
                = v.second.get<std::string>("<xmlattr>.cfgfile", "");
            if (cfgFileName.empty()) {
                loadUser(reader,v);
            } else {
                loadUserConfig( reader, cfgFileName.c_str() );
            }
        }
	}
}


int BarzerSettings::loadListOfConfigs(BELReader& reader, const char *fname) {
    FILE* fp = fopen( fname, "r" );
    if( !fp ) {
		std::cerr << "failed to open config list file " << fname << std::endl;
        return -1;
    }
    char cfgFileName[ 512 ];
    int cfgCount = 0;
    while( fgets(cfgFileName, sizeof(cfgFileName)-1, fp ) ) {
        cfgFileName[ sizeof(cfgFileName)-1 ] = 0;
        size_t buf_len = strlen(cfgFileName);
        if( buf_len ) cfgFileName[ buf_len-1 ] = 0;
        if( isspace(*cfgFileName) || *cfgFileName=='#') {
            continue;
        }

        char* comment = strchr( cfgFileName, '#' );
        if( comment )
            *comment= 0;
        std::cerr << "loading config from: " << cfgFileName << " ...\n";
        load( reader, cfgFileName );
        ++cfgCount;
    }
    fclose( fp );
    return cfgCount;
}

void BarzerSettings::load(BELReader& reader, const char *fname) {
	//AYLOGDEBUG(fname);
	std::cout << "Loading config file: " << fname << std::endl;
	fs::path oldPath = fs::current_path();

	try {
		read_xml(fname, pt);

		fs::path oldPath = fs::current_path();
		const char *barzerHome = std::getenv("BARZER_HOME");
		const char *dataPath = ( getHomeOverridePath().length() ? getHomeOverridePath().c_str() : barzerHome );
		if (dataPath) {
            if(isEnvPath_AUTO() && *fname == '/') {
                reader.getErrStreamRef() << "AUTOMODE Honoring BARZER_HOME " << dataPath << std::endl;
			    fs::current_path(dataPath);
            } else if( isEnvPath_BARZER_HOME() || isEnvPath_ENV_IGNORE() ) {
			    fs::current_path(dataPath);
            }
		} else {
            if( isEnvPath_BARZER_HOME() ) {
                reader.getErrStreamRef() << "WARNING! BARZER_HOME not set assuming current directory " <<
                fs::current_path() << std::endl; 
            } else if( isEnvPath_AUTO() ) {
                reader.getErrStreamRef() << "Home assumed in current directory " <<
                fs::current_path() << std::endl; 
            }
        }


		loadInstanceSettings();
		loadParseSettings();
		loadEntities();
		loadDictionaries();
		loadRules(reader);
		loadUsers(reader);

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




