#include <zurch_server.h>

namespace zurch {

/// zurch (docidx) service interface 

std::ostream& DocIdxSearchResponseXML::print( std::ostream& os, const DocFeatureIndex::DocWithScoreVec_t& docVec ) const 
{
    #warning implement DocIdxSearchResponseXML::print
    return os;
}

std::ostream& DocIdxSearchResponseJSON::print( std::ostream& os, const DocFeatureIndex::DocWithScoreVec_t& docVec ) const 
{

    #warning implement DocIdxSearchResponseJSON::print
    return os;
}
} // namespace zurch
