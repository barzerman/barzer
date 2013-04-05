
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#pragma once
#include <zurch_docidx.h>

namespace zurch {


struct ZurchModelParms {
    /// section title, content etc
    struct Section {
        // default weights
        enum {
            WEIGHT_BOOST_NONE=0, 
            WEIGHT_BOOST_NAME=1000,
            WEIGHT_BOOST_KEYWORD=10,
            WEIGHT_BOOST_RUBRIC=10,
    
            WEIGHT_BOOST_FIRST_PHRASE=20
        };
        int d_WEIGHT_BOOST_NONE,
        d_WEIGHT_BOOST_NAME,
        d_WEIGHT_BOOST_KEYWORD,
        d_WEIGHT_BOOST_RUBRIC,
        d_WEIGHT_BOOST_FIRST_PHRASE;
        
        Section(): 
            d_WEIGHT_BOOST_NONE(WEIGHT_BOOST_NONE),
            d_WEIGHT_BOOST_NAME(WEIGHT_BOOST_NAME),
            d_WEIGHT_BOOST_KEYWORD(WEIGHT_BOOST_KEYWORD),
            d_WEIGHT_BOOST_RUBRIC(WEIGHT_BOOST_RUBRIC),
            d_WEIGHT_BOOST_FIRST_PHRASE(WEIGHT_BOOST_FIRST_PHRASE)
        {}
    };
    Section d_section;
    double d_classBoosts[DocFeature::CLASS_MAX];

    bool loadPropertyTree( const boost::property_tree::ptree& );

    ZurchModelParms() : 
        d_classBoosts { 0.5, 0.5, 0.5, 1, 1.5, 2 }
    {}
};

class ZurchSettings {
    barzer::StoredUniverse& universe;
    std::ostream& d_errFP;
    size_t d_indexCounter; /// sequential number of the index currently being processed
    
    static ZurchModelParms* d_modelParms;
    void initModelParms();
public:
    const ZurchModelParms& modelParms() { return *d_modelParms; }

    ZurchSettings( barzer::StoredUniverse& u, std::ostream& errFp ) : 
        universe(u), d_errFP(errFp), d_indexCounter(0) 
            { initModelParms(); }
    bool loadIndex( const boost::property_tree::ptree& );

    bool operator()( const boost::property_tree::ptree& );

    bool operator()( DocIndexLoaderNamedDocs::LoaderOptions& loader, const boost::property_tree::ptree& ) ;
    bool operator()( PhraseBreaker& phraser, const boost::property_tree::ptree& ) ;

};

} // namespace zurch 
