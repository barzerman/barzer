
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#pragma once 
#include <zurch_docidx.h>
#include <ay/ay_tagindex.h>

/// zurch (docidx) service interface 
namespace zurch {

class DocIdxSearchResponse {
protected:
public:
    const barzer::QuestionParm&     d_qparm;
    const DocIndexAndLoader& d_ixl;
    const barzer::Barz&      d_barz;
    typedef ay::tagindex_checker<uint32_t> TagIndexChecker_t;

    const TagIndexChecker_t * d_tagIdxChecker;

    void setTagIndexChecker( const TagIndexChecker_t * checker ) { d_tagIdxChecker= checker; }
    DocIdxSearchResponse( const barzer::QuestionParm& qparm, const DocIndexAndLoader& ixl, const barzer::Barz& barz ) : 
        d_qparm(qparm), d_ixl(ixl), d_barz(barz), d_tagIdxChecker(0)
    {}
};
class DocIdxSearchResponseXML : public DocIdxSearchResponse {
public:
    DocIdxSearchResponseXML( const barzer::QuestionParm& qparm, const DocIndexAndLoader& ixl, const barzer::Barz& barz ) : 
        DocIdxSearchResponse(qparm,ixl,barz){}
    std::ostream& print( std::ostream& os, const DocWithScoreVec_t&, const std::map<uint32_t, DocFeatureIndex::PosInfos_t>& ) const;
    std::ostream& printHTML( std::ostream& os, const DocWithScoreVec_t&, const std::map<uint32_t, DocFeatureIndex::PosInfos_t>& ) const ;
};

class DocIdxSearchResponseJSON : public DocIdxSearchResponse {
public:
    DocIdxSearchResponseJSON( const barzer::QuestionParm& qparm, const DocIndexAndLoader& ixl, const barzer::Barz& barz ) : 
        DocIdxSearchResponse(qparm,ixl,barz){}
    std::ostream& print( std::ostream& os, const DocWithScoreVec_t&, const std::map<uint32_t, DocFeatureIndex::PosInfos_t>&, const DocFeatureIndex::TraceInfoMap_t& ) const;
};

} // namespace zurch
