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

// these names are for making your eyes bleed

typedef ay::UniqueCharPool::StrId BELStoredFunId;

typedef boost::function<bool(const BELFunctionStorage*,
		                    BarzelEvalResult&,
		                    const BarzelEvalResultVec&)> BELStoredFunction;
typedef std::pair<BELStoredFunId,BELStoredFunction> BELStoredFunRec;
typedef boost::unordered_map<BELStoredFunId,BELStoredFunction> BELStoredFunMap;

// Stores map ay::UniqueCharPool::StrId -> BELFunctionStorage::function
// the function names are using format "stfun_*function name*"
// ADDFN(fname) macro is used add new functions in the constructor
// so we never have to debug typing mistakes

class BELFunctionStorage {
	StoredUniverse &universe;
	BELStoredFunMap funmap;
public:

	BELFunctionStorage(StoredUniverse &u);

	void addFun(const char *fname, BELStoredFunction);
	void addFun(BELStoredFunId, BELStoredFunction);

	bool call(const char *fname, BarzelEvalResult&, const BarzelEvalResultVec&) const;
	bool call(const BELStoredFunId, BarzelEvalResult& ,const BarzelEvalResultVec&) const;


	// stored functions part
#define DEFN(n) bool stfun_##n(BarzelEvalResult&, const BarzelEvalResultVec&) const
	// makers
	DEFN(mkDate);

	// arith
	DEFN(opPlus);
	DEFN(opMinus);
	DEFN(opMult);
	DEFN(opDiv);
	/*
	bool stfun_opPlus(BarzelEvalResult&, BarzelEvalResultVec&);
	bool stfun_opMinus(BarzelEvalResult&, BarzelEvalResultVec&);
	bool stfun_opMult(BarzelEvalResult&, BarzelEvalResultVec&);
	bool stfun_opDiv(BarzelEvalResult&, BarzelEvalResultVec&);
	 */

	// logic
	DEFN(opAnd);
	DEFN(opOr);
	DEFN(opXor);
	DEFN(opMore);
	DEFN(opLess);
	DEFN(opEq);

	/*
	bool stfun_opAnd(BarzelEvalResult&, BarzelEvalResultVec&);
	bool stfun_opOr(BarzelEvalResult&, BarzelEvalResultVec&);
	bool stfun_opXor(BarzelEvalResult&, BarzelEvalResultVec&);
	bool stfun_opMore(BarzelEvalResult&, BarzelEvalResultVec&);
	bool stfun_opLess(BarzelEvalResult&, BarzelEvalResultVec&);
	bool stfun_opEq(BarzelEvalResult&, BarzelEvalResultVec&);
	*/
#undef DEFN

};


}


#endif /* BARZER_EL_FUNCTION_H_ */
