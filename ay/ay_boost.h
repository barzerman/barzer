#pragma once 

#include <boost/property_tree/ptree.hpp>
#include <boost/foreach.hpp>

namespace ay {

struct iterate_name_value_attr {
    std::string nameName, valueName;

    iterate_name_value_attr() : 
        nameName("n"),
        valueName("v")
    {}
    
    /// on a  given ptree iterates over all child tags 
    /// with the same name assuming the tag is like this:
    /// <thetag n="somename" v="somevalue"/>
    // both names and attribute values are strings
    /// invokes the callback for each attribute pair
    // if tag == 0 iterates over all child nodes 
    
    template <typename CB>
    void operator()( const CB& cb, const boost::property_tree::ptree & node, const char* tag=0 ) const 
    {
        BOOST_FOREACH(const boost::property_tree::ptree::value_type &v,node) {
            if( tag && v.first != tag )
                continue;
            if( const boost::optional<const boost::property_tree::ptree&> optAttr = v.second.get_child_optional("<xmlattr>") ) {
                if( const boost::optional<std::string> nOpt = optAttr.get().get_optional<std::string>(nameName) ) {
                    if( const boost::optional<std::string> vOpt = optAttr.get().get_optional<std::string>(valueName) ) {
                        cb( nOpt.get(), vOpt.get() );
                    }
                }
            }
        }
    }
    
    void get_pairvec( std::vector< std::pair<std::string, std::string> >& vec, const boost::property_tree::ptree & node, const char* tag=0 ) const
        { (*this)( [&]( const std::string& n, const std::string& v ) { vec.push_back( {n,v} ); }, node, tag); }
};

/// 
} // end of namespace ay
