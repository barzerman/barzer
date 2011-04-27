/*
 * barzer_el_function.cpp
 *
 *  Created on: Apr 27, 2011
 *      Author: polter
 */

#include <barzer_el_function.h>
#include <barzer_universe.h>

namespace barzer {

// Stores map ay::UniqueCharPool::StrId -> BELFunctionStorage::function
// the function names are using format "stfun_*function name*"
// ADDFN(fname) macro is used add new functions in the constructor
// so we never have to debug typing mistakes

struct BELFunctionStorage_holder {
	StoredUniverse &universe;
	BELStoredFunMap funmap;

	#define ADDFN(n) addFun(#n, boost::mem_fn(&BELFunctionStorage_holder::stfun_##n))
	BELFunctionStorage_holder(StoredUniverse &u) : universe(u) {
		// makers
		ADDFN(mkDate);
		// arith
		ADDFN(opPlus);
		ADDFN(opMinus);
		ADDFN(opMult);
		ADDFN(opDiv);
	}
	#undef ADDFN

	void addFun(const char *fname, BELStoredFunction fun) {
		const BELStoredFunId fid = universe.getStringPool().internIt(fname);
		addFun(fid, fun);
	}

	void addFun(const BELStoredFunId fid, BELStoredFunction fun) {
		funmap.insert(BELStoredFunRec(fid, fun));
	}


	// stored functions
	#define STFUN(n) bool stfun_##n(BarzelEvalResult &result,\
							        const BarzelEvalResultVec &rvec) const

	// makers
	STFUN(mkDate)
	{
		return false;
	}

	// arith

	STFUN(opPlus)
	{
		return false;
	}

	STFUN(opMinus)
	{
		return false;
	}

	STFUN(opMult)
	{
		return false;
	}

	STFUN(opDiv)
	{
		return false;
	}


	// logic

	STFUN(opAnd)
	{
		return false;
	}
	STFUN(opOr)
	{
		return false;
	}

	STFUN(opXor)
	{
		return false;
	}
	STFUN(opLess)
	{
		return false;
	}
	STFUN(opMore)
	{
		return false;
	}
	STFUN(opEq)
	{
		return false;
	}

	#undef STFUN

};

BELFunctionStorage::BELFunctionStorage(StoredUniverse &u) : universe(u),
		holder(new BELFunctionStorage_holder(u)) { }

BELFunctionStorage::~BELFunctionStorage()
{
	delete holder;
}



bool BELFunctionStorage::call(const char *fname, BarzelEvalResult &er,
		                                   const BarzelEvalResultVec &ervec) const
{
	const BELStoredFunId fid = universe.getStringPool().getId(fname);
	if (fid == ay::UniqueCharPool::ID_NOTFOUND) return false;
	return call(fid, er, ervec);
}

bool BELFunctionStorage::call(const BELStoredFunId fid, BarzelEvalResult &er,
									              const BarzelEvalResultVec &ervec) const
{
	const BELStoredFunMap::const_iterator frec = holder->funmap.find(fid);
	if (frec == holder->funmap.end()) return false;
	return frec->second(holder, er, ervec);

}

}




