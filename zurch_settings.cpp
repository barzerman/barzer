#include <zurch_settings.h>

namespace zurch {


bool ZurchSettings::operator()( DocFeatureIndex& index, const boost::property_tree::ptree& pt )
{

    return true;
}
bool ZurchSettings::operator()( DocFeatureLoader& loader, const boost::property_tree::ptree& pt )
{
    return true;
}
bool ZurchSettings::operator()( DocIndexLoaderNamedDocs& loader, const boost::property_tree::ptree& pt )
{
    return true;
}
bool ZurchSettings::operator()( PhraseBreaker& phraser, const boost::property_tree::ptree& pt)
{
    return true;
}

} // namespace zurch 
