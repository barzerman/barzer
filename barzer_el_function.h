/*
 * barzer_el_function.h
 *
 *  Created on: Apr 27, 2011
 *      Author: polter
 */

#ifndef BARZER_EL_FUNCTION_H_
#define BARZER_EL_FUNCTION_H_

#include <boost/unordered_map.hpp>
#include <boost/function.hpp>
#include <barzer_el_rewriter.h>
//#include <barzer_universe.h>

namespace barzer {


class StoredUniverse;
class BELFunctionStorage;
struct BELFunctionStorage_holder;

// these names are for making your eyes bleed

typedef ay::UniqueCharPool::StrId BELStoredFunId;

typedef boost::function<bool(const BELFunctionStorage_holder*,
		                    BarzelEvalResult&,
		                    const BarzelEvalResultVec&)> BELStoredFunction;
typedef std::pair<BELStoredFunId,BELStoredFunction> BELStoredFunRec;
typedef boost::unordered_map<BELStoredFunId,BELStoredFunction> BELStoredFunMap;

class BELFunctionStorage {
	StoredUniverse &universe;
	BELFunctionStorage_holder *holder;
public:

	BELFunctionStorage(StoredUniverse &u);
	~BELFunctionStorage();

	bool call(const char *fname, BarzelEvalResult&, const BarzelEvalResultVec&) const;
	bool call(const BELStoredFunId, BarzelEvalResult& ,const BarzelEvalResultVec&) const;

};


}


#endif /* BARZER_EL_FUNCTION_H_ */
