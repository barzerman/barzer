/*
 * barzer_el_function.cpp
 *
 *  Created on: Apr 27, 2011
 *      Author: polter
 */

#include <barzer_el_function.h>
#include <barzer_universe.h>

namespace barzer {

#define ADDFN(n) addFun(#n, boost::mem_fn(&BELFunctionStorage::stfun_##n))
BELFunctionStorage::BELFunctionStorage(StoredUniverse &u) : universe(u) {
		//addFun("mkDate", boost::mem_fn(&BELFunctionStorage::mkDate));
		// makers
		ADDFN(mkDate);
		// arith
		ADDFN(opPlus);
		ADDFN(opMinus);
		ADDFN(opMult);
		ADDFN(opDiv);
		// logic
}
#undef ADDFN


void BELFunctionStorage::addFun(const char *fname, BELStoredFunction fun) {
	const BELStoredFunId fid = universe.getStringPool().internIt(fname);
	addFun(fid, fun);
}

void BELFunctionStorage::addFun(const BELStoredFunId fid, BELStoredFunction fun) {
	funmap.insert(BELStoredFunRec(fid, fun));
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
	const BELStoredFunMap::const_iterator frec = funmap.find(fid);
	if (frec == funmap.end()) return false;
	return frec->second(this, er, ervec);

}

// stored functions
#define STFUN(n) bool BELFunctionStorage::stfun_##n(BarzelEvalResult &result,\
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

}




