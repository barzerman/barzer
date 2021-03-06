
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#pragma once
#include <cstdlib>
#include <iostream>

#include <ay/ay_headers.h>
#include <ay/ay_logger.h>

#include <barzer_barz.h>
#include <barzer_parse.h>
#include <barzer_server_request.h>

namespace barzer {


struct RequestVariableMap {
    typedef std::map< std::string, BarzelBeadAtomic_var > Map;
    Map d_map;
    
    const Map& getAllVars() const { return d_map; }
    bool getValue( BarzelBeadAtomic_var& v, const char* n ) const 
    {
        auto i = d_map.find(n);
        if( i != d_map.end() ) 
            return( v = i->second, true );
        else 
            return false;
    }

    bool hasVar( const char* n ) const 
        { return( d_map.find(n) != d_map.end() ); }
    const BarzelBeadAtomic_var*  getValue( const char* n ) const 
    {
        auto i = d_map.find(n);
        if( i != d_map.end() )
            return &(i->second);
        else
            return 0;
    }

    
    void setValue( const char* n, const BarzelBeadAtomic_var& v ) 
        { d_map[ n ] = v; }
    bool unset( const char* n ) 
        { return(d_map.erase(n) !=0); }
    
    void clear() { d_map.clear(); }
    
    std::ostream& print( std::ostream& ) const;
};


/// manifest info for a given request
struct RequestEnvironment {
    BarzerDateTime     d_now;
    RequestVariableMap d_reqVar;

	/// user information
	uint32_t    userId;

	/// request buffer 
	const char* buf;
	size_t      len;

	std::ostream& outStream;

    const RequestVariableMap::Map& getAllVars() const { return d_reqVar.getAllVars(); }

    const RequestVariableMap& getReqVar() const { return d_reqVar; }
          RequestVariableMap& getReqVar() { return d_reqVar; }

    RequestVariableMap*       getReqVarPtr() { return &d_reqVar; }
    const RequestVariableMap*       getReqVarPtr() const { return &d_reqVar; }

    template <typename T>
    const T* getVarVal( const char* n ) const 
    {
        if( auto var= d_reqVar.getValue(n) ) 
            return boost::get<T>(var);
        else 
            return 0;
    }

	RequestEnvironment( std::ostream& os ): 
		userId(0),
		buf(0),
		len(0),
		outStream(os)
	{}
	RequestEnvironment( std::ostream& os, const char* b, size_t l ): 
		userId(0),
		buf(b),
		len(l),
		outStream(os)
	{}
    
    void clear() 
    {
        userId=0;
        buf=0;
        len=0;
        d_reqVar.clear();
    }
    
    const BarzerDateTime* getNowPtr() const { return &d_now; }
    // returns non 0 in case of error
    /// format must be YYYY-MM-DD HH:MM:SS 
    int setNow( const std::string& );
};

int run_server(GlobalPools&, uint16_t);

namespace request {

/// standard barze request 
int barze( const GlobalPools&, RequestEnvironment& reqEnv );

/// for function proc_YYY the message format is 
/// !!YYY: <..whatever the xml or other stuff might be..>
/// emits patterns after application of control structures ANY/TAIL etc. 
int proc_EMIT( RequestEnvironment&, const GlobalPools&, const char* buf );

enum {
    ROUTE_ERROR_OK=0,
    ROUTE_ERROR_UNKNOWN_COMMAND, // 1 
    ROUTE_ERROR_EXECUTION_FAILED // 2
};
/// ends up routing between other functions
int route( GlobalPools& gpools, char* buf, const size_t len, std::ostream& os );


} // request namespace ends

} // namespace barzer
