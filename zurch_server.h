#pragma once 
#include <zurch_docidx.h>

/// zurch (docidx) service interface 
namespace zurch {

class DocIdxSearchResponse {
protected:
public:
    const DocIndexAndLoader& d_ixl;
    const barzer::Barz&      d_barz;
    DocIdxSearchResponse( const DocIndexAndLoader& ixl, const barzer::Barz& barz ) : d_ixl(ixl), d_barz(barz) {}
};
class DocIdxSearchResponseXML : public DocIdxSearchResponse {
public:
    DocIdxSearchResponseXML( const DocIndexAndLoader& ixl, const barzer::Barz& barz ) : DocIdxSearchResponse(ixl,barz){}
    std::ostream& print( std::ostream& os, const DocFeatureIndex::DocWithScoreVec_t& ) const ;
};

class DocIdxSearchResponseJSON : public DocIdxSearchResponse {
public:
    DocIdxSearchResponseJSON( const DocIndexAndLoader& ixl, const barzer::Barz& barz ) : DocIdxSearchResponse(ixl,barz){}
    std::ostream& print( std::ostream& os, const DocFeatureIndex::DocWithScoreVec_t& ) const ;
   
};
} // namespace zurch
