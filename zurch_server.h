
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#pragma once 
#include <zurch_docidx.h>

/// zurch (docidx) service interface 
namespace zurch {

class DocIdxSearchResponse {
protected:
public:
    const barzer::QuestionParm&     d_qparm;
    const DocIndexAndLoader& d_ixl;
    const barzer::Barz&      d_barz;
    DocIdxSearchResponse( const barzer::QuestionParm& qparm, const DocIndexAndLoader& ixl, const barzer::Barz& barz ) : 
        d_qparm(qparm), d_ixl(ixl), d_barz(barz) 
    {}
};
class DocIdxSearchResponseXML : public DocIdxSearchResponse {
public:
    DocIdxSearchResponseXML( const barzer::QuestionParm& qparm, const DocIndexAndLoader& ixl, const barzer::Barz& barz ) : 
        DocIdxSearchResponse(qparm,ixl,barz){}
    std::ostream& print( std::ostream& os, const DocFeatureIndex::DocWithScoreVec_t&, const std::map<uint32_t, DocFeatureIndex::PosInfos_t>& ) const ;
};

class DocIdxSearchResponseJSON : public DocIdxSearchResponse {
public:
    DocIdxSearchResponseJSON( const barzer::QuestionParm& qparm, const DocIndexAndLoader& ixl, const barzer::Barz& barz ) : 
        DocIdxSearchResponse(qparm,ixl,barz){}
    std::ostream& print( std::ostream& os, const DocFeatureIndex::DocWithScoreVec_t&, const std::map<uint32_t, DocFeatureIndex::PosInfos_t>& ) const ;
   
};
} // namespace zurch
