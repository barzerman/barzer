#pragma once 

#include <boost/property_tree/ptree.hpp>
#include <boost/foreach.hpp>
#include <ay_util_char.h>
#include <iostream>

namespace ay {

struct ptree_opt {
    const boost::property_tree::ptree& pt;
    bool operator()(bool& v, const char* name ) const
    {
        if(auto x = pt.get_optional<std::string>(name)) {
            std::string val = x.get();
            if((val.length() == 1 && tolower(val[0]) =='y') || ay_strcasecmp(val.c_str(), "true") || ay_strcasecmp(val.c_str(), "yes"))
                return (v=true, true);
            else
            if((val.length() == 1 && tolower(val[0]) =='n') || ay_strcasecmp(val.c_str(), "false") || ay_strcasecmp(val.c_str(), "no"))
                return (v=false, true);
            else {
                std::cerr << "ptree_opt: assuming \"" << val << "\" means false\n";
                return (v=false, true);
            }
        } else
            return false;
    }

    bool get(const char* name, const bool dfv = false ) const 
    {
        bool v = false;
        return( (*this)(v,name)? v: dfv);
    }

    template <typename T>
    bool operator()(T& v, const char* name) const
    {
        if(auto x = pt.get_optional<T>(name))
            return (v = x.get(), true);
        else
            return false;
    }
    template <typename T>
    T get(const char* name, const T dfv) const
    {
        if(auto x = pt.get_optional<T>(name))
            return x.get();
        else
            return dfv;
    }
    ptree_opt( const boost::property_tree::ptree& p): pt(p) {}
};

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
