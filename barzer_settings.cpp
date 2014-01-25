
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
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
#include <ay/ay_util_time.h>
#include <ay_ngrams.h>

#include <boost/foreach.hpp>
#include <boost/filesystem.hpp>
#include <time.h>
#include <cstdlib>
#include <barzer_bzspell.h>
#include <barzer_relbits.h>
#include <zurch_settings.h>
#include <barzer_beni.h>

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


User& BarzerSettings::createUser(User::Id id, const char* uname ) {
	UserMap::iterator it = umap.find(id);
	if (it == umap.end()) {
		std::cerr << "Adding user id(" << id << ")\n";
        if( !gpools.getUniverse(id) ) {
            std::cerr << "Creating universe for " << id << std::endl;
            gpools.produceUniverse( id ).setUserName( uname ? uname: "");
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
	const char *fname = f.fname.c_str();
	reader.setCurrentUniverse( d_currentUniverse, getUser(d_currentUniverse->getUserId()) );

    uint32_t trieClass = gpools.internString_internal(tclass.c_str()) ;
    uint32_t trieId = gpools.internString_internal(tid.c_str()) ;

	reader.setTrie(trieClass, trieId );
    if( !f.extraDictionaryPath.empty() ) {
	    BZSpell* bzs = d_currentUniverse->initBZSpell(0);
        if( bzs ) {
            bzs->loadExtra( f.extraDictionaryPath.c_str(), reader.getTriePtr() );
        } else {
            std::cerr << "INTERNAL ERROR in addRulefile\n";
        }
    }

	int num = reader.loadFromFile(fname, BELReader::INPUT_FMT_XML);
    if( num>=0 ) {
	    std::cerr << " PROCESSOR FILE ";
        if( d_currentUniverse->userName().empty() )
            std::cerr << tclass;
        else
            std::cerr << d_currentUniverse->userName();
	    std::cerr <<  "::" << tid << "\t " << fname << " " << " rules (" << num << ")" ;
        if( reader.getNumSkippedStatements() )
            std::cerr << " skipped(" << reader.getNumSkippedStatements() << ")";
        if (reader.getNumMacros()) 
            std::cerr <<" macro(" << reader.getNumMacros() << ")";
        if (reader.getNumProcs() ) 
            std::cerr << ", proc(" << reader.getNumProcs() << ")";
        std::cerr << "\n";
    }
}

void BarzerSettings::addDictionaryFile(const char *fname)
{
	std::cerr << "Reading dictionary from " << fname << " ... " << gpools.readDictionaryFile( fname ) << "read\n";

}
void BarzerSettings::addEntityFile(const char *fname)
{
	// entityFiles.push_back(fname);
	gpools.getDtaIdx().loadEntities_XML(fname,getCurrentUniverse());
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

namespace {
template<typename T>
void feedStream(T &model, std::istream& istr)
{
    int stringsCount = 0;
    while (istr.good()) {
        std::string str;
        istr >> str;
        model.addWord(str.c_str());
        ++stringsCount;
    }
    std::cerr << "Language classifier " << stringsCount << " words from " << std::endl;
}

template <typename T>
inline bool getAttrByEitherName( T& v, const ptree & attrs, const char* n1, const char* n2=0 )
{
    if( const boost::optional<std::string> x = attrs.get_optional<std::string>(n1) ) {
        return( v = x.get(), true );
    } else if( n2 ) {
        if( const boost::optional<std::string> x = attrs.get_optional<std::string>(n2)) 
            return( v= x.get(), true );
    }
    return false;
}

inline boost::optional<const ptree&> getNodeByEitherName( const ptree & node, const char* n1, const char* n2 )
{
    auto optNode = node.get_child_optional(n1);
    if( !optNode )
        optNode = node.get_child_optional(n2);
    return optNode;
}

} // anon namespace 

void BarzerSettings::loadLangNGrams()
{
	using boost::property_tree::ptree;

	ay::UTF8TopicModelMgr *utf8lm = d_currentUniverse->getGlobalPools().getUTF8LangMgr();
	ay::ASCIITopicModelMgr *asciilm = d_currentUniverse->getGlobalPools().getASCIILangMgr();

	try
	{
		const ptree& dicts = pt.get_child("config.freqDicts", empty_ptree());
		BOOST_FOREACH(const ptree::value_type& v, dicts)
		{
			const ptree& fileTag = v.second;
			const char *filePath = fileTag.data().c_str();

			const boost::optional<std::string>& str = fileTag.get_child("<xmlattr>").get_optional<std::string>("lang");
			if (!str)
			{
				std::cerr << "lang not set for a freqDicts.file tag with file path "
						<< filePath
						<< std::endl;
				continue;
			}

			const int langId = ay::StemWrapper::getLangFromString(str->c_str());
			if (langId == ay::StemWrapper::LG_INVALID)
			{
				std::cerr << "unknown language "
						<< langId
						<< std::endl;
				continue;
			}

			std::ifstream istr(filePath);
            if (!istr.is_open())
				std::cerr << "ERROR loading language classifier cannot open file:" << filePath << std::endl;
			else if (ay::StemWrapper::isUnicodeLang(langId))
				feedStream (utf8lm->getModel(langId), istr);
			else
				feedStream (asciilm->getModel(langId), istr);
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "error loading language NGrams definitions:"
				<< e.what()
				<< std::endl;
	}
	catch (...)
	{
		std::cerr << "error loading language NGrams definitions: unknown" << std::endl;
	}
}

void BarzerSettings::loadRules(BELReader& reader, const boost::property_tree::ptree& rules)
{
	if (rules.empty()) {
		// warning goes here
		return;
	}
    std::string defaultClass = getDefaultClassName();

	BOOST_FOREACH(const ptree::value_type &v, rules) {
		if (v.first == "file") {
			const ptree &file = v.second;
            Rulefile ruleFile( file.data() );
			try {
				const ptree &attrs = file.get_child("<xmlattr>");
                const boost::optional<std::string> noCanonicalNames = attrs.get_optional<std::string>("nonames");

                if( const auto p = attrs.get_optional<std::string>("fn") )
                    ruleFile.fname = p.get();    

                if( const auto p = attrs.get_optional<std::string>("dict") ) 
                    ruleFile.extraDictionaryPath = *p;

                if( auto x = attrs.get_optional<std::string>("class")) 
				    ruleFile.trie.first  = x.get();
                else {
                    ruleFile.trie.first = defaultClass;
                }

                if( !getAttrByEitherName( ruleFile.trie.second, attrs, "name", "rewriter") ) 
                    continue;

                uint32_t trieClass = gpools.internString_internal(ruleFile.trie.first.c_str()) ;
                uint32_t trieId = gpools.internString_internal(ruleFile.trie.second.c_str()) ;

                const boost::optional<std::string> tagsToFilter = attrs.get_optional<std::string>("tag");
                if( tagsToFilter ) {
                    reader.setTagFilter(tagsToFilter.get().c_str());
                } else
                    reader.clearTagFilter();

				reader.setCurTrieId( trieClass, trieId );
                if( noCanonicalNames ) {
                    reader.set_noCanonicalNames();
                } else 
                    reader.set_canonicalNames();

				addRulefile(reader, ruleFile );
			} catch (boost::property_tree::ptree_bad_path&) {
				reader.clearCurTrieId();
				addRulefile(reader,ruleFile);
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
				std::cerr << "threadcount of " << nt << " exceeds " <<  MAX_REASONABLE_THREADCOUNT << std::endl;
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
            gpools.getEntData().readFromFile( gpools, propFileName );
        } else
		    addEntityFile(v.second.data().c_str());
	}
}


void BarzerSettings::loadMeanings (User &u, const ptree& node)
{

    const boost::optional<const ptree&> optMeaningNode = node.get_child_optional("meanings");

    if( !optMeaningNode )
        return;
	const ptree meaningsNode = optMeaningNode.get();

    StoredUniverse& uni = u.getUniverse();
    MeaningsStorage& meanings = uni.meanings();

    GlobalPools& gp = uni.gp;
    // within one user tag there may be several MEANING tags with file attributes 
    // for each of these tags we read meanings from the file 
	const boost::optional<const ptree&> optAttrs = meaningsNode.get_child_optional("<xmlattr>");
	if (!optAttrs)
		return;

	const ptree& attrs = optAttrs.get();
	const boost::optional<std::string> optFname = attrs.get_optional<std::string>("file");
	if (!optFname || optFname.get().empty()) 
		return;
	const boost::optional<std::string> mode = attrs.get_optional<std::string>("automode");

    const char* modeStr = ( mode ? mode.get().c_str(): 0 );
    meanings.setAutoextModeByName( modeStr );

	const boost::optional<uint8_t> threshold = attrs.get_optional<uint8_t>("threshold");
	meanings.setAutoexpansionThreshold(threshold ? *threshold : MeaningsStorage::AUTOEXPAND_DEFAULT_THRESHOLD);

	const boost::optional<uint8_t> defPrio = attrs.get_optional<uint8_t>("defprio");
	MeaningsXMLParser p(gp, &uni, defPrio ? *defPrio : MeaningsStorage::AUTOEXPAND_DEFAULT_PRIORITY);

	std::string fullPath;
	const std::string& fname = optFname.get();
	
    std::cerr << "loading meanings from " << fname << "... ";
	p.readFromFile(fname.c_str());
    std::cerr << "done meanings:" << p.d_countMeaningsRead << ", words: " << p.d_countWordsRead << std::endl;
}

void BarzerSettings::loadSpell(User &u, const ptree &node)
{
	const ptree &spell = node.get_child("spell", empty_ptree());
	BZSpell* bzs = u.getUniverse().initBZSpell(0);

	try {
	    const boost::optional<const ptree&> optAttrs = spell.get_child_optional("<xmlattr>");
        if( optAttrs ) {
            const ptree& attrs = optAttrs.get();

            if( const auto p = attrs.get_optional<std::string>("utf8") ) 
                if( *p== "yes" ) u.getUniverse().setBit( UBIT_NOSTRIP_DIACTITICS );

            if( const auto p = attrs.get_optional<std::string>("soundslike") ) 
                u.getUniverse().setSoundsLike( *p !="no" );
            
            if( const auto p = attrs.get_optional<std::string>("dictcorr") ) 
                if( *p== "yes" ) u.getUniverse().setBit( UBIT_CORRECT_FROM_DICTIONARY );

			if( const auto p = attrs.get_optional<std::string>("stempunct") )
				if (*p == "yes") u.getUniverse().setBit(UBIT_LEX_STEMPUNCT);
				
			if (const auto p = attrs.get_optional<std::string>("featuredsc"))
				if (*p == "yes") u.getUniverse().setBit(UBIT_FEATURED_SPELLCORRECT);
				
			if (const auto p = attrs.get_optional<std::string>("fsconly"))
				if (*p == "yes") u.getUniverse().setBit(UBIT_FEATURED_SPELLCORRECT_ONLY);
			
			if (const auto p = attrs.get_optional<std::string>("stemex"))
				u.getUniverse().getBZSpell()->loadStemExceptions(*p);
        }
		BOOST_FOREACH(const ptree::value_type &v, spell) {
			const std::string& tagName = v.first;
			const char* tagVal = v.second.data().c_str();
			if( tagName == "extra" ) {
				u.extraDictFileName = tagVal;
            } else if( tagName == "lang" ) {
				int lang = ay::StemWrapper::getLangFromString( tagVal );
				if( lang != ay::StemWrapper::LG_INVALID )
				{
					std::cerr << "Adding language: " << tagVal << "(" << lang << ")" << std::endl;
					u.getUniverse().getBarzHints().addUtf8Language(lang);
					ay::StemThreadPool::inst().addLang(lang);
				}
            } else if(tagName =="tokenizer") { /// 
                const boost::optional<const ptree&> optAttrs = spell.get_child_optional("tokenizer.<xmlattr>");
                if( optAttrs ) {
                    const ptree& attrs = optAttrs.get();
                    const boost::optional<std::string> tmpOpt = attrs.get_optional<std::string>("strat");
                    if( tmpOpt  ) { 
                        if( !strcasecmp( tmpOpt.get().c_str(),"SPACE_DEFAULT" ) ) 
                            u.getUniverse().tokenizerStrategy().setType( TokenizerStrategy::STRAT_TYPE_SPACE_DEFAULT);
                        else if( !strcasecmp( tmpOpt.get().c_str(),"CASCADE" ) ) 
                            u.getUniverse().tokenizerStrategy().setType( TokenizerStrategy::STRAT_TYPE_CASCADE);
                        else
                            u.getUniverse().tokenizerStrategy().setType( TokenizerStrategy::STRAT_TYPE_DEFAULT);
                    }
                }
            }
		}
		if( bzs ) {
			bzs->printStats( std::cerr );
		} else {
			AYLOG(ERROR) << "Null BZSpell object for user id " << u.getId() << std::endl;
		}
	} catch (boost::property_tree::ptree_bad_path &e) {
		AYLOG(ERROR) << "Can't get " << e.what();
		return;
	}
}

std::string BarzerSettings::getDefaultClassName() const 
{
    std::string defaultClass;
    {
        std::stringstream sstr;
        sstr << getCurrentUniverse()->getUserId() ;
        defaultClass = sstr.str() ; 
    }
    return defaultClass;
}

void BarzerSettings::loadTrieset(BELReader& reader, User &u, const ptree &node) 
{
    auto optNode = getNodeByEitherName( node, "trieset", "rwrchain" );
    if( !optNode )
        return;

    std::string defaultClass = getDefaultClassName();
	BOOST_FOREACH(const ptree::value_type &v, optNode.get() ) {
		if (v.first == "trie" || v.first == "rewriter" || v.first=="topicdetect") {
			const ptree &trieNode = v.second;
			try {
				const ptree &attrs = trieNode.get_child("<xmlattr>");

				std::string cl;
                if( auto x = attrs.get_optional<std::string>("class") ) 
                    cl = x.get();
                else {
                    cl = defaultClass;
                }
                std::string name ;
                if( !getAttrByEitherName( name, attrs, "name", "rewriter" )  )
                    continue;

                bool isTopic = ( attrs.get_optional<std::string>("topic") || v.first=="topicdetect" );
                // const boost::optional<std::string> optTopic = attrs.get_optional<std::string>("topic");

                // noac - autocomplete doesnt apply . optional attribute. when set
                // when this attribute is set the grammar wont be used in autocomplete
                const boost::optional<std::string> optNoAutoc = attrs.get_optional<std::string>("noac");

				std::cerr << "ADDED PROCESSOR\t\"" << ( u.getUniverse().userName().empty() ? cl : u.getUniverse().userName() ) << "::" << name << "\"" << (isTopic? " TOPIC" :"") << "\n";

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
				u.addTrie(TriePath(cl, name), isTopic, gramInfo);

			} catch (boost::property_tree::ptree_bad_path &e) {
				AYLOG(ERROR) << "Can't get " << e.what();
			}
		}
	}
	const UniverseTrieCluster& tcluster = u.getUniverse().getTrieCluster();
	std::cerr << "user " << u.getUniverse().userName() << "[" << u.getId() << "] " << tcluster.getTrieList().size()	 << " processors loaded\n";
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
void load_global_zurch_model_settings( const ptree &node )
{
    zurch::ZurchModelParms::init();

    zurch::ZurchModelParms& zp = zurch::ZurchModelParms::getNonconst();
    if( boost::optional< const ptree& > optNode = node.get_child_optional("config.zurchmodel") ) {
        zurch::ZurchModelParms::getNonconst().load( optNode.get() );
        zurch::ZurchModelParms::get().print( std::cerr );
    }
}
void load_zurch(BELReader& reader, User& u, const ptree &node)
{
    zurch::ZurchSettings zs(u.universe,reader.getErrStreamRef());
    if( !zs( node ) )  {
        std::cerr << "ERROR LOADING ZURCH" << std::endl;
    } 
}
void load_user_flags(BELReader& reader, User& u, const ptree &node)
{
    try {
        const ptree &entseg = node.get_child("flags", empty_ptree());
        BOOST_FOREACH(const ptree::value_type &v, entseg ) {
            StoredEntityClass eclass;
            const boost::optional<std::string> valOpt = v.second.get_optional<std::string>("value");

			if (valOpt)
				u.universe.setUBits( valOpt->c_str() );
        } // foreach
    } catch(...) {
        std::cerr << "load_user_flags exception\n";
    }
}
void load_ent_info(BELReader& reader, User& u, const ptree &node)
{
    {  // entity subclass translation 
    /// <enttrans>
    ///   <subclass src="class[,subclass]" dest="class[,subclass]"/>
    /// </enttrans>
    const ptree &enttrans = node.get_child("enttrans", empty_ptree());
    if( enttrans.empty() )
        return;

    BOOST_FOREACH(const ptree::value_type &v, enttrans ) {
		if (v.first == "subclass") {
            StoredEntityClass fromEc, toEc;
            if( boost::optional< const ptree& > oa = v.second.get_child_optional("<xmlattr>") ) {
                if( const boost::optional<std::string> optSrc = oa.get().get_optional<std::string>("src"))  {
                    if( const boost::optional<std::string> optDest = oa.get().get_optional<std::string>("dest"))  {
                        std::string src = optSrc.get(), dest= optDest.get();
                        if( u.addEntTranslation( src.c_str(), dest.c_str() ) )
                            std::cerr << "ERROR setting entity translation subclass src=\"" << src << "\", dest=\"" << dest << "\"" << std::endl;
                    }
                }
            }
        }
    }

    }  // end of entity subclass translation
    
    {  // entity segmentation settings processing 
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
    } // end of entity segmentation setings processing 

    // synonym designation
    if( boost::optional< const ptree& > x = node.get_child_optional("synent") ) {
        StoredEntityClass ec;
        if( optAttr_assign( ec.ec,oa,"c") && optAttr_assign( ec.subclass,oa,"s") )
            u.getUniverse().addEntClassToSynonymDesignation( ec );
    }
}

} // anonymous namespace ends

int User::addEntTranslation( const char* src, const char* dest ) 
{
    StoredEntityClass srcClass, destClass;
    const char* srcComma=strchr(src, ',');
    const char* destComma = strchr(dest,',');
    if( !(src && dest ) ) 
        return -1;
    if( srcComma && destComma ) { /// src has a comma
        srcClass.setClass( atoi(src));
        srcClass.setSubclass(atoi(srcComma+1));

        destClass.setClass( atoi(dest));
        destClass.setSubclass(atoi(destComma+1));
        addEntTranslation( srcClass, destClass );

        return 0; // success
    } else
        return -1; // failure
}
int User::loadExtraDictionary()
{
    if( !extraDictFileName.empty() ) {
        BZSpell* bzs = universe.getBZSpell();
        if( bzs ) {
            std::cerr << "LOADING EXTRA DICTIONARY from " <<  extraDictFileName << std::endl;
            return bzs->loadExtra( extraDictFileName.c_str(), 0 );
        } else 
            std::cerr << "INTERNAL ERROR: loadExtraDictionary failed for " << extraDictFileName <<std::endl;
    }
    return 0;
}
int User::readClassNames( const ptree& node )
{
    int numSubclasses = 0;
    if( boost::optional< const ptree& > optNode = node.get_child_optional("entity") ) {
	    boost::optional< const ptree& > oa = optNode.get().get_child_optional("<xmlattr>");

        if( const boost::optional< uint32_t > classIdOpt = optNode.get().get_optional<uint32_t>( "<xmlattr>.class_id" ) ) 
        {
            uint32_t eclass = classIdOpt.get();
            if( const boost::optional< std::string > classNameOpt = optNode.get().get_optional<std::string>("<xmlattr>.class_name") ) 
                universe.getGlobalPools().d_entClassToNameMap[ eclass ] = classNameOpt.get();
            BOOST_FOREACH(const ptree::value_type &i, optNode.get() ) {
                if( i.first == "subclass" ) {
		            const ptree attrs = i.second.get_child("<xmlattr>");
     
                    if( const boost::optional<std::string> idOpt = attrs.get_optional<std::string>("id") ) {
                        if( const boost::optional<std::string> nameOpt = attrs.get_optional<std::string>("name") ) {
                            uint32_t subclass = atoi( idOpt.get().c_str() ) ;
                            universe.setSubclassName( StoredEntityClass(eclass,subclass),  nameOpt.get() );
                            ++numSubclasses;
                        }
                    }
                }
            }
        }

    } 
    return numSubclasses;
}

namespace {

std::string& updateTrueUserName( std::string & trueUserName, const char* uname, uint32_t userId )
{
    if( !uname || !(*uname) ) {
        std::stringstream sstr;
        sstr << "user" << userId;
        trueUserName = sstr.str();
    } else
        trueUserName.assign( uname );
    return trueUserName;
}

/// if attribute exists assigns dest, otherwise doesnt alter dest
template <typename T>
inline bool optAttr_assign( T& dest, const boost::optional<const ptree&>& oa, const char* name ) 
{ 
    if( const boost::optional<T> x = oa.get().get_optional<T>(name) ) { 
        dest = x.get(); 
        return true;
    } else 
        return false;
}

template <typename T>
inline T optAttr_getval( const boost::optional<const ptree&>& oa, const char* name, T defVal=0 ) 
    { if( const boost::optional<T> x = oa.get().get_optional<T>(name) ) { return x.get(); } else return defVal; }

inline uint32_t optAttr_intern_internal( GlobalPools& gpools, const boost::optional<const ptree&>& oa, const char* name ) 
{
    if( const boost::optional<std::string> x = oa.get().get_optional<std::string>(name) ) 
        return gpools.internString_internal(x.get().c_str());
    else
        return 0xffffffff;
}

} // end of anon namespace

int BarzerSettings::loadUser(BELReader& reader, const ptree::value_type &user, const char* uname )
{
	const ptree &children = user.second;
	const boost::optional<uint32_t> userIdOpt
		= children.get_optional<uint32_t>("<xmlattr>.id");

	if (!userIdOpt) {
		AYLOG(ERROR) << "No user id in tag <user>";
		return 0;
	}
    uint32_t userId = userIdOpt.get();
    std::string trueUserName;
    uname = updateTrueUserName( trueUserName, uname, userId ).c_str();

	User &u = createUser(userId, uname);
	std::cerr << "Loading user id: " << userId << "\n";

	u.readClassNames(children);

	loadSpell(u, children);
	loadMeanings(u, children);

	const boost::optional<std::string> respectLimitsOpt
		= children.get_optional<std::string>("<xmlattr>.limits");
    if( !respectLimitsOpt || respectLimitsOpt.get() != "no" ) 
        reader.setRespectLimits( userId );
    else
        std::cerr << "Emit counts and all other LIMITS will be ignored\n";
    reader.setCurrentUniverse( u.getUniversePtr(), &u );
    load_ent_info(reader, u, children);
    load_user_flags(reader, u, children);
	loadUserRules(reader, u, children);
	loadTrieset(reader, u, children);
	loadLocale(reader, u, children);

	StoredUniverse& uni = u.getUniverse();
	u.getUniverse().initBZSpell(0);
    u.loadExtraDictionary();
	u.getUniverse().getBarzHints().initFromUniverse(&uni);

    if( boost::optional< const ptree& > x = children.get_child_optional("beni") ) {
        u.getUniverse().setBit( UBIT_USE_BENI_VANILLA );
         
        u.getUniverse().setBit( UBIT_USE_BENI_IDS );
        if( const boost::optional<const ptree&> xAttr = x.get().get_child_optional("<xmlattr>") ) {
            if( const boost::optional<std::string> cOpt = xAttr.get().get_optional<std::string>("noids") ) {
                u.getUniverse().setBit( UBIT_USE_BENI_IDS, false );
            }
            if( const boost::optional<std::string> cOpt = xAttr.get().get_optional<std::string>("sl") ) {
                if( cOpt.get() != "no" )
                    u.getUniverse().setBit( UBIT_BENI_SOUNDSLIKE, true );
            }
            if( const boost::optional<std::string> cOpt = xAttr.get().get_optional<std::string>("noboost") ) {
                u.getUniverse().setBit( UBIT_BENI_NO_BOOST_MATCH_LEN, (cOpt.get() == "yes") );
            }
            /// beni tag zurch attribute . value is id of the zurch universe
            if( const boost::optional<uint32_t> cOpt = xAttr.get().get_optional<uint32_t>("zurch") ) {
                uint32_t zurchUserId = cOpt.get();
                StoredUniverse* zurchUniverse = gpools.getUniverse(zurchUserId);
                if( !zurchUniverse ) {
	                User &u = createUser(zurchUserId);
                    zurchUniverse = gpools.getUniverse(zurchUserId);
                }
                u.getUniverse().beni().setZurchUniverse(zurchUniverse);
            }
        } 

        u.getUniverse().setBit( UBIT_NEED_CONFIDENCE );

        BOOST_FOREACH(const ptree::value_type &i, x.get() ) {
            if( i.first == "benient" ) {
                if( const boost::optional<const ptree&> oa = i.second.get_child_optional("<xmlattr>") ) {
                    StoredEntityClass ec;
                    if( optAttr_assign( ec.ec,oa,"c") && optAttr_assign( ec.subclass,oa,"s") )
                        u.getUniverse().indexEntityNames( ec );
                }
            } else 
            if( i.first == "benifile" ) {
                if( const boost::optional<const ptree&> oa = i.second.get_child_optional("<xmlattr>") ) {
                    if( const boost::optional<std::string> fnam = oa.get().get_optional<std::string>("f") ) {
                        std::string mode;

                        optAttr_assign( mode, oa, "m" ); // default entity class

                        StoredEntityClass ec, topicEc;
                        optAttr_assign( ec.ec,       oa, "cl" ); // default entity class
                        optAttr_assign( ec.subclass, oa, "sc" ); // default entity subclass

                        topicEc.ec      = optAttr_getval<uint32_t>(oa, "tsc", ec.ec); // default topic entity class
                        topicEc.subclass= optAttr_getval<uint32_t>(oa, "tsc", ec.subclass); // default topic entity subclass

                        u.getUniverse().beni().addEntityFile( fnam.get().c_str(), mode.empty() ? 0 : mode.c_str(), ec, topicEc );
                    }
                }
            } else 
            if( i.first == "replace" ) { /// attributes f - find, r - replace 
                if( const boost::optional<const ptree&> oa = i.second.get_child_optional("<xmlattr>") ) {
                    std::string regexFind, regexReplace;
                    optAttr_assign( regexFind,oa,   "f" );
                    optAttr_assign( regexReplace,oa,"r" );
                    u.getUniverse().beni().addMandatoryRegex( regexFind, regexReplace );
                }
            } else 
            if( i.first == "implicit_topic" ) {
                if( const boost::optional<const ptree&> oa = i.second.get_child_optional("<xmlattr>") ) {
                    BarzerEntity tent( 
                        optAttr_intern_internal( gpools, oa, "i" ) ,
                        optAttr_getval<uint32_t>(oa, "c"), 
                        optAttr_getval<uint32_t>(oa, "s")
                    );

                    u.getUniverse().topicEntLinkage().addImplicitTopic( tent ) ;
                }
            }
        }
        /// to ensure beniids is processed after all entiteis have been loaded
        BOOST_FOREACH(const ptree::value_type &i, x.get() ) {
            if( i.first == "beniids" ) {
                if( const boost::optional<const ptree&> oa = i.second.get_child_optional("<xmlattr>") ) {
                    StoredEntityClass ec;
                    if( optAttr_assign( ec.ec,oa,"c") && optAttr_assign( ec.subclass,oa,"s") ) {
                        std::string repat, rerep;
                        const char* repatStr = 0, *rerepStr = 0;
                        if( boost::optional<std::string> repatOpt = oa.get().get_optional<std::string>("repat") )  { // regex search pat
                            if( boost::optional<std::string> rerepOpt = oa.get().get_optional<std::string>("rerep") ) { // regex replace expression
                                repat = repatOpt.get();
                                repatStr = repat.c_str();
                                rerep = rerepOpt.get();
                                rerepStr = rerep.c_str();
                            }
                        }
                        u.getUniverse().entLookupBENIAddSubclass( ec, repatStr, rerepStr );
                    }
                }
            }
        }
    }
    if( boost::optional< const ptree& > zurchChild = children.get_child_optional("zurch") )
        load_zurch( reader, u, zurchChild.get() );

    return 1;
}

int BarzerSettings::loadUserConfig( BELReader& reader, const char* cfgFileName, const char* uname ) {
    if( !gpools.getUniverse(0) ) {
        User &u = createUser(0,"root");
        BZSpell* bzs = u.getUniverse().initBZSpell( 0 );
        bzs->init( 0 );
    }
    boost::property_tree::ptree userPt;
    int numUsersLoaded = 0;
    try {
        read_xml(cfgFileName, userPt);
        BOOST_FOREACH(const ptree::value_type &userV, userPt.get_child("config.users")) {
            if (userV.first == "user") {
                loadUser( reader, userV, uname );
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
            const std::string &cfgFileName = v.second.get<std::string>("<xmlattr>.cfgfile", "");
            // const std::string &cfgFileName = v.second.get<std::string>("<xmlattr>.cfgfile", "");
		    const boost::optional<std::string> userNameOpt = v.second.get_optional<std::string>("<xmlattr>.username");

            int loadedUsers = 0;
            std::string uname;
            if( userNameOpt ) 
                uname = userNameOpt.get();

            if (cfgFileName.empty()) 
                loadedUsers = loadUser(reader,v, uname.c_str());
            else
                loadedUsers = loadUserConfig( reader, cfgFileName.c_str(), uname.c_str() );

            if( loadedUsers && userNameOpt ) {
                StoredUniverse* uniPtr = reader.getCurrentUniverse();

                if( uniPtr )  {
                    const std::string uname = userNameOpt.get();
                    uniPtr->setUserName( uname.c_str() );
                    gpools.setUserNameForId( uname.c_str(), uniPtr->getUserId() );    
                }
            } else 
                reader.setCurrentUniverse(0, getUser(0) );
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
    ay::stopwatch totalTimer;

	std::cerr << "Loading config file: " << fname << std::endl;
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
        load_global_zurch_model_settings(pt);

		loadParseSettings();
		loadEntities();
		loadDictionaries();
		loadLangNGrams();
		loadRules(reader);
		loadUsers(reader);

		fs::current_path(oldPath);
	} catch (boost::property_tree::xml_parser_error &e) {
		AYLOG(ERROR) << e.what();
	} catch(...) {
        AYLOG(ERROR) << "Fatal error reading file" << std::endl;
    }
    reader.getErrStreamRef() << "Done in " << totalTimer.calcTime()  << " seconds" << std::endl;
}

const std::string BarzerSettings::get(const char *key) const {
	return pt.get<std::string>(key);
}

const std::string BarzerSettings::get(const std::string &key) const {
	return get(key.c_str());
}
}
