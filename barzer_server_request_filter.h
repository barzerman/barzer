#pragma once
#include <map>
#include <string>
#include <vector>
#include <boost/variant.hpp>

namespace barzer {

class ReqFilter {
public:
    boost::variant<
        std::vector<int>,
        std::vector<double>,
        std::vector<std::string>
    > filterData;

    enum {
        VT_INT,
        VT_DOUBLE,
        VT_STRING,

        // add new types only above his line 
        VT_MAX
    };
    
    typedef enum {
        FILTER_TYPE_ONEOF,
        FILTER_TYPE_RANGE_FULL,

        FILTER_TYPE_RANGE_NOHI,
        FILTER_TYPE_RANGE_NOLO,

        /// add new types above
        FILTER_TYPE_RANGE_MAX

    } filter_type_t; 

    bool          positive; // when false this is interpreted as a negation
    filter_type_t filterType;
    
    template <typename T> std::vector<T>* get( ) { return boost::get<T>(&filterData); }
    template <typename T> const std::vector<T>* get( ) const { return boost::get<T>(&filterData); }
    
    void setFilterType( filter_type_t x ) { filterType = x; }
    bool isFilter_oneof() const { return filterType == FILTER_TYPE_ONEOF; }
    bool isFilter_range_full() const { return filterType == FILTER_TYPE_ONEOF; }

    template <typename T> std::vector<T>& set( const std::vector<T>& v ) { return (filterData = v, filterData ); }
    template <typename T> std::vector<T>& set( ) { 
        filterData = std::vector<T>();
        return boost::get<std::vector<T>>(filterData) ; 
    }

    ReqFilter() : positive(true), filterType(FILTER_TYPE_ONEOF) 
        {}
    
    void push_back( const std::string& s ) 
    {
        switch( filterData.which() ) {
        case VT_INT: boost::get<std::vector<int>>(filterData).push_back( atoi(s.c_str()) ); break;
        case VT_DOUBLE: boost::get<std::vector<double>>(filterData).push_back( atof(s.c_str()) ); break;
        case VT_STRING: boost::get<std::vector<std::string>>(filterData).push_back(s); break;
        }
    }
    bool empty() const
    {
        switch( filterData.which() ) {
        case VT_INT: return boost::get<std::vector<int>>(filterData).empty();
        case VT_DOUBLE: return boost::get<std::vector<double>>(filterData).empty();
        case VT_STRING: return boost::get<std::vector<std::string>>(filterData).empty();
        }
        return true;
    }
    // i - integer, d - double, s- string
    void setDataType( const char* buf, const char* buf_end ) 
    {
        if( buf_end<= buf ) // default
            set<int>();
        else {
            switch( *buf ) {
            case 'i': set<int>(); break;
            case 'd': set<double>(); break;
            case 's': set<std::string>(); break;
            default: set<int>(); break;
            }
        }
    }
    // r - range, o - oneof
    void setFilterType( const char* buf, const char* buf_end ) 
    {
        if( buf_end<= buf ) // default
            filterType = FILTER_TYPE_ONEOF;
        else {
            switch( *buf ) {
            case 'o': filterType=FILTER_TYPE_ONEOF;         break;
            case 'r': filterType=FILTER_TYPE_RANGE_FULL;    break;
            default: filterType=FILTER_TYPE_ONEOF;    break;
            }
        }
    }
};

struct ReqFilterCascade {
    std::map< std::string, ReqFilter > filterMap;
    ReqFilter& addFilter( const std::string& name ) 
        { return filterMap.insert( {name, ReqFilter()} ).first->second; }
    
    template <typename T>
    ReqFilter& addFilter( const std::string& name, const std::vector<T>& vec  )
    { 
        ReqFilter &i = addFilter(name);
        return( i.set( vec ), i );
    }
    ReqFilter& addFilter( const std::string& name, const ReqFilter& filter )
    {
        return filterMap.insert({name, filter}).first->second;
    }
    const ReqFilter* getFilter( const std::string& name ) const
    {
        auto i = filterMap.find( name );
        return( i != filterMap.end() ? &(i->second) : 0 );
    }
    void clear() { filterMap.clear(); }
    
    // T,F,P [,x0,x1,x2...]
    // T - type i - integer, d - double, s - string
    // F - filter type - r - range, o - one of
    // P - name of the property 
    // 
    void parse( const char* str, const char* str_end );
    bool empty() const { return filterMap.empty(); }
};

} // namespace barzer
