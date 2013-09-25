
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
/*
 * barzer_el_function.h
 *
 *  Created on: Apr 27, 2011
 *      Author: polter
 */

#pragma once

#include <boost/unordered_map.hpp>
#include <boost/function.hpp>
#include <barzer_el_rewriter.h>
#include <ay/ay_vector.h>

namespace barzer {


class GlobalPools;
class BELFunctionStorage;
struct BELFunctionStorage_holder;
class BarzelEvalContext;

// these names are for making your eyes bleed



struct FuncInfo {
    std::string name; // function name
    std::string descr; // function description
    
    FuncInfo( const char* n, const char*d ) :
        name(n), descr(d? d: "") {}
};

/// built in stored function 
typedef bool (*Func)(
    const BELFunctionStorage_holder*,
    BarzelEvalResult&,
    const ay::skippedvector<BarzelEvalResult>&,
    const StoredUniverse&, 
    BarzelEvalContext&, 
    const BTND_Rewrite_Function&);

typedef std::pair< Func,  FuncInfo > FuncData;
typedef std::map<uint32_t,FuncData> FuncMap;
typedef FuncMap::value_type FuncMapRec;

class BELFunctionStorage {
	GlobalPools &globPools;
	BELFunctionStorage_holder *holder;
public:
	BELFunctionStorage(GlobalPools &u, bool initFunctions );
	~BELFunctionStorage();
    const FuncMap& getFuncMap() const;

	bool call(
              BarzelEvalContext& ctxt,
              const BTND_Rewrite_Function&, BarzelEvalResult&,
			  const ay::skippedvector<BarzelEvalResult>&,
			  const StoredUniverse &u ) const;

    
    static void help_list_funcs_json( std::ostream&, const GlobalPools& gp );
    void loadAllFunctions( );
};


} // namespace barzer
