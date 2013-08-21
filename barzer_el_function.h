
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


typedef boost::function<bool(
                            const BELFunctionStorage_holder*,
		                    BarzelEvalResult&,
		                    const ay::skippedvector<BarzelEvalResult>&,
		                    const StoredUniverse&, BarzelEvalContext&, 
                            const BTND_Rewrite_Function&)> BELStoredFunction;

typedef boost::unordered_map<uint32_t,BELStoredFunction> BELStoredFunMap;
//typedef std::pair<uint32_t,BELStoredFunction> BELStoredFunRec;
typedef BELStoredFunMap::value_type BELStoredFunRec;

struct BELFuncInfo
{
	uint32_t nameId;
	uint32_t descId;
};

class BELFunctionStorage {
	GlobalPools &globPools;
	BELFunctionStorage_holder *holder;
public:

	BELFunctionStorage(GlobalPools &u);
	~BELFunctionStorage();

	std::vector<BELFuncInfo> getFuncInfos() const;

    /*
	bool call(
              BarzelEvalContext& ctxt,
              const char *fname, 
              BarzelEvalResult&,
			  const BarzelEvalResultVec&,
			  const StoredUniverse& ) const;
    */
	bool call(
              BarzelEvalContext& ctxt,
              const BTND_Rewrite_Function&, BarzelEvalResult&,
			  const ay::skippedvector<BarzelEvalResult>&,
			  const StoredUniverse &u ) const;

};
} // namespace barzer
