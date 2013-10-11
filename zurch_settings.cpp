
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#include <zurch_settings.h>
#include <boost/foreach.hpp>
#include <barzer_universe.h>
#include <zurch/zurch_loader_longxml.h>
#include <ay_util_time.h>
#include <ay_boost.h>



using boost::property_tree::ptree;

namespace zurch {

bool ZurchSettings::loadIndex( const boost::property_tree::ptree& pt )
{
    ++d_indexCounter;
    const boost::optional<const ptree&> attr = pt.get_child_optional("<xmlattr>");
    if( !attr ) {
        d_errFP << "<error>ZURCH index " << d_indexCounter << " in user " << universe.getUserId() << " has no attributes</error>" << std::endl;
        return false;
    }
    uint32_t id = 0;

    if( const boost::optional< std::string > x = attr.get().get_optional<std::string>("id") ) 
        id = atoi( x.get().c_str() );

    zurch::DocIndexAndLoader* dixl = universe.initZurchIndex( id );
    if( !dixl )  /// this should never happen
        return false;
    DocIndexLoaderNamedDocs* loader = dixl->getLoader(); 
	
	dixl->getIndex()->setInternStems();

    if( const boost::optional< const::ptree &> x = pt.get_child_optional("phraser") ) 
        (*this)( loader->phraser(), x.get() );

    if( const boost::optional< const::ptree &> x = pt.get_child_optional("loader") ) 
        (*this)( loader->loaderOpt(), x.get() );
	
	if (const auto x = pt.get_child_optional("synonyms"))
		dixl->getIndex()->loadSynonyms(x->data(), universe);

	if (const boost::optional<std::string> x = attr.get().get_optional<std::string>("flags")) {
        for( auto c = x.get().begin(), c_end = x.get().end(); c != c_end; ++c ) {
            switch( *c ) {
	        case 'c': loader->setNoChunks( true ); break;
	        case 'd': loader->setNoContent( true ); break;
	        case 'C': loader->setNoChunks( false ); break;
	        case 'D': loader->setNoContent( false ); break;
	        case 'S': dixl->getIndex()->d_bitflags.set( DocFeatureIndex::ZBIT_SMART_SCORING ); break;
	        case 's': dixl->getIndex()->d_bitflags.set( DocFeatureIndex::ZBIT_SMART_SCORING, false ); break;
			case 'P': dixl->getIndex()->setKeepDetailedPositions(true); break;
			case 'p': dixl->getIndex()->setKeepDetailedPositions(false); break;
            }
        }
	}
	
    if( const boost::optional< std::string > x = attr.get().get_optional<std::string>("dir") )   {
        loader->d_loadMode = DocFeatureLoader::LOAD_MODE_TEXT;
        dixl->addAllFilesAtPath(x.get().c_str());
    }

    /// content in phrase format 
    /// C_BEGIN|MODULE.ID
    ///   ... lines of content ...
    /// C_END|MODULE.ID
    /// TITLE|MODULE.ID|Text of the title 
    /// this format is generated by the barzer_shell "zextract" command 
    if( const boost::optional< std::string > x = attr.get().get_optional<std::string>("content") ) {
        DocFeatureLoader::DocStats stats;
        ZurchPhrase_DocLoader parser( stats, *loader );
        std::string fname = x.get();
        ay::stopwatch localTimer;
        std::cerr << "Loading zurch doc contents from" << fname << std::endl;
        parser.readDocContentFromFile( fname.c_str()  );
        parser.d_loader.computeTitleStats();
        std::cerr <<     " done in " << localTimer.calcTime() << " seconds\n";
    }
    //bool isPhraseMode = false;
    // MODULE.ID|tX|N|phrase
    // format is generated by the barzer shell "phrase" command
    if( const boost::optional< std::string > x = attr.get().get_optional<std::string>("phr") ) {
        //isPhraseMode = true;
        std::string fname = x.get();
        DocFeatureLoader::DocStats stats;
        loader->d_loadMode = DocFeatureLoader::LOAD_MODE_PHRASE;
        ZurchPhrase_DocLoader parser( stats, *loader );
        if( !fname.empty() ) 
            parser.readPhrasesFromFile( fname.c_str(), true );

        /// index tag may have child phr tags. child phr tags will be iterated . <phr file="another_phrase_file.txt"/>
        /// and all files contained will be read 
        BOOST_FOREACH(const ptree::value_type &v, pt) {
            if( v.first != "phr" ) 
                continue;
            if( const boost::optional<const ptree&> phrAttr = v.second.get_child_optional("<xmlattr>") ) {
                if( const boost::optional<std::string> optFile = phrAttr.get().get_optional<std::string>("file") ) {
                    fname = optFile.get();
                    if( !fname.empty() ) 
                        parser.readPhrasesFromFile( fname.c_str(), true ); // reads without sorting
                }
            }
        }
        /// all phrase files have been read. now we must sort
        loader->index().d_docDataIdx.simpleIdx().sort();

        stats.print( std::cerr << "\n" ) << std::endl;
    }

    if( const boost::optional< std::string > x = attr.get().get_optional<std::string>("xml") ) {
        DocFeatureLoader::DocStats stats;
        loader->d_loadMode = DocFeatureLoader::LOAD_MODE_XHTML;
        ZurchLongXMLParser_DocLoader parser( stats, *loader );
        ay::stopwatch localTimer;
        std::string fname = x.get();
        // std::cerr << "Loading doc contents from" << fname << std::endl;
        parser.readFromFile( fname.c_str() );
        std::cerr <<     " done in " << localTimer.calcTime() << " seconds\n";
        stats.print( std::cerr << "\n" ) << std::endl;
    }
    if( const boost::optional< std::string > x = attr.get().get_optional<std::string>("ent") ) {
        std::string fname = x.get();
        loader->loadEntLinks( fname.c_str() );
    }
    if( const boost::optional< std::string > x = attr.get().get_optional<std::string>("entignore") ) {
        std::string fname = x.get();
        loader->loadEntIgnoreLinks( fname.c_str() );
    }
    
    return true;
}

bool ZurchSettings::operator()( const boost::property_tree::ptree& pt )
{
    d_indexCounter= 0;
    if( boost::optional< const ptree& > zurchChild = pt.get_child_optional("model") ) {
    }
    BOOST_FOREACH(const ptree::value_type &v, pt) {
        if( v.first == "index" )  {
            loadIndex(v.second );
        }
    }
    if( !d_indexCounter ) {
        d_errFP << "<error>ZURCH in user " << universe.getUserId() << " has no index tags</error>" << std::endl;
    }
    return true;
}
bool ZurchSettings::operator()( PhraseBreaker& phraser, const boost::property_tree::ptree& pt )
{
    return true;
}
bool ZurchSettings::operator()( DocIndexLoaderNamedDocs::LoaderOptions& loaderOpt, const boost::property_tree::ptree& pt )
{
    return true;
}

ZurchModelParms* ZurchModelParms::d_modelParms = 0;

namespace {

int load_model_parms_feature_weight( const boost::property_tree::ptree& node )
{
    BOOST_FOREACH(const ptree::value_type &v,node) {
        
    }
    return 0;
}
int load_model_parms_class_weight( const boost::property_tree::ptree& node )
{
    return 0;
}

} // end of anon namespace 

int ZurchModelParms::setFeatureBoost( const std::string& n, const std::string& v )
{
    double x = atof( v.c_str() );
    if( n == "stem" ) 
        return( d_classBoosts[DocFeature::CLASS_STEM] = x, 0 );
    else if( n == "syngroup" ) 
        return( d_classBoosts[DocFeature::CLASS_SYNGROUP] = x, 0 );
    else if( n == "number" )
        return (d_classBoosts[DocFeature::CLASS_NUMBER] = x, 0 );
    else if( n == "token" )
        return (d_classBoosts[DocFeature::CLASS_TOKEN] = x, 0 );
    else if( n == "datetime" ) 
        return( d_classBoosts[DocFeature::CLASS_DATETIME] = x, 0 );
    else if( n == "entity" )
        return (d_classBoosts[DocFeature::CLASS_ENTITY] = x, 0 );

    return 1;
}

std::ostream& ZurchModelParms::print( std::ostream& fp ) const
{
    fp << "Zurch Model: { " << 
    "\n\t" << "Sections: { " << 
        "\t\t" << "NONE=" << d_section.d_WEIGHT_BOOST_NONE << "," <<  std::endl << 
        "\t\t" << "NAME=" << d_section.d_WEIGHT_BOOST_NAME << "," <<  std::endl << 
        "\t\t" << "KEYWORD=" << d_section.d_WEIGHT_BOOST_KEYWORD << "," <<  std::endl << 
        "\t\t" << "RUBRIC=" << d_section.d_WEIGHT_BOOST_RUBRIC << "," <<  std::endl << 
        "\t\t" << "PHRASE=" << d_section.d_WEIGHT_BOOST_FIRST_PHRASE << "," <<  std::endl << 
    "\t" << "}" << std::endl;

    fp << "\n\t" << "Feature: { " << std::endl <<
        "\t\t" << "STEM=" << d_classBoosts[DocFeature::CLASS_STEM] << "," << std::endl << 
		"\t\t" << "SYNGROUP=" << d_classBoosts[DocFeature::CLASS_SYNGROUP] << "," << std::endl << 
		"\t\t" << "NUMBER=" << d_classBoosts[DocFeature::CLASS_NUMBER] << "," << std::endl << 
        "\t\t" << "TOKEN=" << d_classBoosts[DocFeature::CLASS_TOKEN] << "," << std::endl << 
		"\t\t" << "DATETIME=" << d_classBoosts[DocFeature::CLASS_DATETIME] << "," << std::endl << 
        "\t\t" << "ENTITY=" << d_classBoosts[DocFeature::CLASS_ENTITY] <<  std::endl << 
    "\t" << "}" << std::endl;
    return fp;
}

int ZurchModelParms::setSectionBoost( const std::string& n, const std::string& v )
{
    int x = atoi( v.c_str() );
    if( n == "none" ) 
        return (d_section.d_WEIGHT_BOOST_NONE = x, 0 );
    else if( n == "name" ) 
        return (d_section.d_WEIGHT_BOOST_NAME = x, 0 );
    else if( n == "keyword" ) 
        return (d_section.d_WEIGHT_BOOST_KEYWORD=x, 0 );
    else if( n == "rubric" ) 
        return (d_section.d_WEIGHT_BOOST_RUBRIC=x, 0 );
    else if ( n == "first_phrase" ) 
        return (d_section.d_WEIGHT_BOOST_FIRST_PHRASE=x, 0 );

    return 1;
}

int ZurchModelParms::load( const boost::property_tree::ptree& node)
{
    ay::iterate_name_value_attr attrIter;
    BOOST_FOREACH(const ptree::value_type &v,node) {
        if( v.first == "feature_weight" ) {
            attrIter( 
                [&]( const std::string& n, const std::string& v ) {
                   setFeatureBoost( n, v );
                },
                v.second,
                "pair"
            );
        } else 
        if( v.first == "section_weight" ) 
        {
            attrIter( 
                [&]( const std::string& n, const std::string& v ) {
                   setSectionBoost( n, v );
                },
                v.second,
                "pair"
            );
        }
    }
    return false;
}
void ZurchModelParms::init( bool reinit  )
{
    if( reinit ) {
        if( d_modelParms ) {
            delete d_modelParms;
            d_modelParms=0;
        }
    }
        
    if( !d_modelParms )
        d_modelParms = new ZurchModelParms();
}

} // namespace zurch 
