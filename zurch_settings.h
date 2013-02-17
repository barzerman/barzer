
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#pragma once
#include <zurch_docidx.h>

namespace zurch {


class ZurchSettings {
    barzer::StoredUniverse& universe;
    std::ostream& d_errFP;
    size_t d_indexCounter; /// sequential number of the index currently being processed
public:
    ZurchSettings( barzer::StoredUniverse& u, std::ostream& errFp ) : universe(u), d_errFP(errFp), d_indexCounter(0) {}
    bool loadIndex( const boost::property_tree::ptree& );

    bool operator()( const boost::property_tree::ptree& );

    bool operator()( DocIndexLoaderNamedDocs::LoaderOptions& loader, const boost::property_tree::ptree& ) ;
    bool operator()( PhraseBreaker& phraser, const boost::property_tree::ptree& ) ;

};

} // namespace zurch 
