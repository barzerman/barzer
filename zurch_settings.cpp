
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#include <zurch_settings.h>
#include <boost/foreach.hpp>
#include <barzer_universe.h>
#include <zurch/zurch_loader_longxml.h>


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
            }
        }
	}
	
    if( const boost::optional< std::string > x = attr.get().get_optional<std::string>("dir") )   {
        loader->d_loadMode = DocFeatureLoader::LOAD_MODE_TEXT;
        dixl->addAllFilesAtPath(x.get().c_str());
    }

    //bool isPhraseMode = false;
    if( const boost::optional< std::string > x = attr.get().get_optional<std::string>("phr") ) {
        //isPhraseMode = true;
        DocFeatureLoader::DocStats stats;
        loader->d_loadMode = DocFeatureLoader::LOAD_MODE_PHRASE;
        ZurchPhrase_DocLoader parser( stats, *loader );
        parser.readFromFile( x.get().c_str() );
        stats.print( std::cerr << "\n" ) << std::endl;
    }
    if( const boost::optional< std::string > x = attr.get().get_optional<std::string>("content") ) {
        DocFeatureLoader::DocStats stats;
        ZurchPhrase_DocLoader parser( stats, *loader );
        std::string fname = x.get();
        parser.readDocContentFromFile( fname.c_str() );
    }
    if( const boost::optional< std::string > x = attr.get().get_optional<std::string>("xml") ) {
        DocFeatureLoader::DocStats stats;
        loader->d_loadMode = DocFeatureLoader::LOAD_MODE_XHTML;
        ZurchLongXMLParser_DocLoader parser( stats, *loader );
        parser.readFromFile( x.get().c_str() );
        stats.print( std::cerr << "\n" ) << std::endl;
    }
    if( const boost::optional< std::string > x = attr.get().get_optional<std::string>("ent") ) {
        std::string fname = x.get();
        loader->loadEntLinks( fname.c_str() );
    }
    
    return true;
}

bool ZurchSettings::operator()( const boost::property_tree::ptree& pt )
{
    d_indexCounter= 0;
    BOOST_FOREACH(const ptree::value_type &v, pt) {
        if( v.first == "index" )  
            loadIndex(v.second );
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

} // namespace zurch 
