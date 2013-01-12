#pragma once
#include <zurch_docidx.h>

namespace zurch {

//// reads properties for doc idx using boost property tree 
class ZurchSettings {
public:
bool operator()( DocFeatureIndex& index, const boost::property_tree::ptree& );
bool operator()( DocFeatureLoader& loader, const boost::property_tree::ptree& );
bool operator()( DocFeatureIndexFilesystem& loader, const boost::property_tree::ptree& ) ;
bool operator()( PhraseBreaker& phraser, const boost::property_tree::ptree& ) ;

};

} // namespace zurch 
