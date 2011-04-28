/*
 * barzer_el_function.cpp
 *
 *  Created on: Apr 27, 2011
 *      Author: polter
 */

#include <barzer_el_function.h>
#include <barzer_universe.h>
#include <barzer_basic_types.h>
#include <ay/ay_logger.h>


namespace barzer {

namespace {

// some utility stuff

// returns BarzerNumber ot of random barzer type. NaN if fails
struct NumberMatcher : public boost::static_visitor<BarzerNumber> {
	BarzerNumber operator()( const BarzerNumber &dt ) {	return dt; }
	BarzerNumber operator()( const BarzelBeadAtomic &dt )
		{ return boost::apply_visitor(*this, dt.dta); }
	template <typename T> BarzerNumber operator()( const T& )
		{ return BarzerNumber(); }  // NaN
};

// writes result number into BarzerNumber - NaN when failed
static bool setNumber(BarzerNumber &num, const BarzelEvalResult &result) {
	NumberMatcher m;
	num = boost::apply_visitor(m, result.getBeadData());
	return !num.isNan();
}

template<class T> void setResult(BarzelEvalResult&, const T&);

template <>
void setResult<BarzerNumber>(BarzelEvalResult &result,
									const BarzerNumber &num)
{
	BarzelBeadAtomic atm;
	atm.dta = num;
	result.setBeadData(atm);
}

template<class T> void setResult(BarzelEvalResult &result, const T &data) {
	BarzelBeadAtomic atm;
	atm.dta = data;
	result.setBeadData(atm);
}

// probably can replace it by something from stl
struct plus {
	template <class T> T operator()(T v1, T v2) { return v1 + v2; }
};
struct minus {
	template <class T> T operator()(T v1, T v2) { return v1 - v2; }
};
struct mult {
	template <class T> T operator()(T v1, T v2) { return v1 * v2; }
};
struct div {
	template <class T> T operator()(T v1, T v2) { return v1 / v2; }
};


template<class U> struct ArithVisitor : public boost::static_visitor<bool> {
	BarzerNumber &val;
	U op;

	ArithVisitor(BarzerNumber &res)  : val(res) { }

	bool operator()( const BarzelBeadAtomic &dt ) {
		return boost::apply_visitor(*this, dt.dta);
	}

	// this is really ugly
	bool operator()( const BarzerNumber &dt ) {
		if (val.isNan() || dt.isNan()) {
			val.clear();
			return false;
		}
		if (val.isReal()) {
			if (dt.isReal()) {
				val.set( op(val.getReal(), dt.getReal()) );
			} else {
				val.set( op(val.getReal(), (double) dt.getInt()) );
			}
		} else {
			if (dt.isReal()) {
				val.set( op((double)val.getInt(), dt.getReal()) );
			} else {
				val.set( op(val.getInt(), dt.getInt()) );
			}
		}
		return true;
	}

	template <typename T> bool operator()( const T& )
	{
		val.clear();
		return false;
	}
};
}
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
		ADDFN(mkTime);
		// arith
		ADDFN(opPlus);
		ADDFN(opMinus);
		ADDFN(opMult);
		ADDFN(opDiv);
		//logic
		ADDFN(opAnd);
		ADDFN(opOr);
		ADDFN(opXor);
		ADDFN(opLt);
		ADDFN(opGt);
		ADDFN(opEq);

	}
	#undef ADDFN

	void addFun(const char *fname, BELStoredFunction fun) {
		const uint32_t fid = universe.getStringPool().internIt(fname);
		addFun(fid, fun);
	}

	void addFun(const uint32_t fid, BELStoredFunction fun) {
		funmap.insert(BELStoredFunRec(fid, fun));
	}


	// stored functions
	#define STFUN(n) bool stfun_##n( BarzelEvalResult &result,\
							        const BarzelEvalResultVec &rvec) const

	// makers
	STFUN(mkDate) // american format
	{
		//sets everything to today
		BarzerNumber m(BarzerDate::thisMonth),
					 d(BarzerDate::thisDay),
					 y(BarzerDate::thisYear);
		BarzerDate date;
		// changes fields to whatever was set
		switch (rvec.size()) {
		case 3:
			setNumber(y, rvec[2]);
		case 2:
			setNumber(d, rvec[1]);
		case 1:
			setNumber(m, rvec[0]);
			break;
		default: break;
			// huhuh
		}
		date.setDayMonthYear(d,m,y);
		setResult(result, date);
		return true;
	}

	STFUN(mkTime)
	{
		return false;
	}


	// arith

	STFUN(opPlus)
	{
		//AYLOG(DEBUG) << "opPlus called";
		BarzerNumber bn(0);
		ArithVisitor<plus> visitor(bn);

		for (BarzelEvalResultVec::const_iterator ri = rvec.begin(); ri != rvec.end(); ++ri) {
			if (!boost::apply_visitor(visitor, ri->getBeadData())) break;
		}

		setResult(result, bn);
		return true;


	}

	STFUN(opMinus)
	{
		BarzerNumber bn; // NaN

		if (rvec.size()) {
			ArithVisitor<minus> visitor(bn);

			BarzelEvalResultVec::const_iterator ri = rvec.begin();

			// sets bn to the value of the first element of rvec
			if (setNumber(bn, *ri))
				while(++ri != rvec.end())
					if (!boost::apply_visitor(visitor, ri->getBeadData())) break;
		}

		setResult(result, bn); // most likely NaN if something went wrong
		return true;
	}

	STFUN(opMult)
	{
		BarzerNumber bn(1);
		ArithVisitor<mult> visitor(bn);

		for (BarzelEvalResultVec::const_iterator ri = rvec.begin(); ri != rvec.end(); ++ri) {
			if (!boost::apply_visitor(visitor, ri->getBeadData())) break;
		}

		setResult(result, bn);
		return true;
	}

	STFUN(opDiv) // no checks for division by zero yet
	{
		BarzerNumber bn; // NaN

		if (rvec.size()) {
			ArithVisitor<div> visitor(bn);

			BarzelEvalResultVec::const_iterator ri = rvec.begin();

			// sets bn to the value of the first element of rvec
			if (setNumber(bn, *ri))
				while(++ri != rvec.end())
					if (!boost::apply_visitor(visitor, ri->getBeadData())) break;
		}

		setResult(result, bn); // most likely NaN if something went wrong
		return true;
	}


	// logic
	// no implementation yet for i'm not sure yet what to put into the result
	// meaning there's no boolean type in barzer (yet?)

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
	STFUN(opLt)
	{
		return false;
	}
	STFUN(opGt)
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
	//AYLOG(DEBUG) << "calling function name `" << fname << "'";
	const uint32_t fid = universe.getStringPool().getId(fname);
	if (fid == ay::UniqueCharPool::ID_NOTFOUND) return false;
	return call(fid, er, ervec);
}

bool BELFunctionStorage::call(const uint32_t fid, BarzelEvalResult &er,
									              const BarzelEvalResultVec &ervec) const
{
	//AYLOG(DEBUG) << "calling function id `" << fid << "'";
	const BELStoredFunMap::const_iterator frec = holder->funmap.find(fid);
	if (frec == holder->funmap.end()) return false;
	return frec->second(holder, er, ervec);

}

}




