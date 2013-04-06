
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
        DocFeatureLink::Weight_t d_WEIGHT_BOOST_NONE,
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

    int load( const boost::property_tree::ptree& );

    static ZurchModelParms* d_modelParms;

    static const ZurchModelParms& get() { return *d_modelParms; }
    /// this should NEVER be needed outside of the settings file
    /// be very careful 
    static ZurchModelParms& getNonconst() { return *d_modelParms; }
public:
    double getClassBoost( uint8_t maxClass ) const
        { return (maxClass< DocFeature::CLASS_MAX ? d_classBoosts[maxClass] : d_classBoosts[DocFeature::CLASS_STEM] ); }
    static void init( bool reinit = false);

    int setFeatureBoost( const std::string& n, const std::string& v );
    int setSectionBoost( const std::string& n, const std::string& v );
    std::ostream& print( std::ostream& ) const;
private: 
    ZurchModelParms() : 
        d_classBoosts { 0.5, 0.5, 0.5, 1, 1.2, 2 }
    {}
};

class ZurchSettings {
    barzer::StoredUniverse& universe;
    std::ostream& d_errFP;
    size_t d_indexCounter; /// sequential number of the index currently being processed
    
public:

    ZurchSettings( barzer::StoredUniverse& u, std::ostream& errFp ) : 
        universe(u), d_errFP(errFp), d_indexCounter(0) 
            { }
    bool loadIndex( const boost::property_tree::ptree& );

    bool operator()( const boost::property_tree::ptree& );

    bool operator()( DocIndexLoaderNamedDocs::LoaderOptions& loader, const boost::property_tree::ptree& ) ;
    bool operator()( PhraseBreaker& phraser, const boost::property_tree::ptree& ) ;

};

} // namespace zurch 
