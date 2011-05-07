/*
 * barzer_el_function.cpp
 *
 *  Created on: Apr 27, 2011
 *      Author: polter
 */

#include <barzer_el_function.h>
#include <barzer_universe.h>
#include <barzer_basic_types.h>
#include <sstream>
#include <ay/ay_logger.h>
#include <barzer_date_util.h>

namespace barzer {

namespace {

// some utility stuff

// returns BarzerNumber ot of random barzer type. NaN if fails
struct NumberMatcher : public boost::static_visitor<BarzerNumber> {
	BarzerNumber operator()( const BarzerNumber &dt ) {	return dt; }
	/*BarzerNumber operator()( const 	BarzerLiteral &dt ) {
		dt.print(AYLOG(DEBUG) << "literal type: ");
		return BarzerNumber();
	} */
	BarzerNumber operator()( const BarzelBeadAtomic &dt )
		{ return boost::apply_visitor(*this, dt.dta); }
	template <typename T> BarzerNumber operator()( const T& )
		{
			//AYLOG(DEBUG) << "unknown type";
			return BarzerNumber();
		}  // NaN
};


static BarzerNumber getNumber(const BarzelEvalResult &result) {
	NumberMatcher m;
	return boost::apply_visitor(m, result.getBeadData());
}

// writes result number into BarzerNumber - NaN when failed
static bool setNumber(BarzerNumber &num, const BarzelEvalResult &result) {
	NumberMatcher m;
	num = boost::apply_visitor(m, result.getBeadData());
	return !num.isNan();
}

template<class T> struct BeadMatcher : public boost::static_visitor<T> {
	const T* operator()( const T &dt ) { return &dt; }
	const T* operator()( const BarzelBeadAtomic &dt )
			{ return boost::apply_visitor(*this, dt.dta); }
	template<class U> T* operator()( const U& ) { return 0; }
};

// safe version, returns 0 on fail
template<class T> const T* getBead(const BarzelEvalResult &result)
{
	//BeadMatcher<T> m;
	return boost::apply_visitor(BeadMatcher<T>(), result.getBeadData());
}

// throwing version. only call it when catching boost::bad_get
template<class T> const T& getAtomic(const BarzelEvalResult &result) {
	return boost::get<T>(
			boost::get<BarzelBeadAtomic>(result.getBeadData()).dta);
}

//template<class T> void setResult(BarzelEvalResult&, const T&);

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


// concats N strings into one using std::stringstream

struct StrConcatVisitor : public boost::static_visitor<bool> {
	std::stringstream ss;
	StoredUniverse &universe;
	StrConcatVisitor(StoredUniverse &u) : universe(u) {}

	bool operator()( const BarzerString &dt ) {
		ss << dt.getStr();
		return true;
	}
	bool operator()( const BarzerLiteral &dt ) {
		switch (dt.getType()) {
		case BarzerLiteral::T_STRING:
		case BarzerLiteral::T_COMPOUND: {
			const char *str = universe.getStringPool().resolveId(dt.getId());
			if (str) {
				ss << str;
				return true;
			} else {
				AYLOG(ERROR) << "Unknown string ID";
				return false;
			}
		}
		case BarzerLiteral::T_PUNCT:
			ss << (char)dt.getId();
			return true;
		default:
			AYLOG(ERROR) << "Wrong literal type";
			return false;
		}
	}

	bool operator()( const BarzelBeadAtomic &dt ) {
		return boost::apply_visitor(*this, dt.dta);
	}

	template <typename T> bool operator()( const T& )
	{
		return false;
	}
};


// tries to pack 2 values into 1 range
// always assumes the values are atomic and of the same type
// the only supported types at the moments are
// BarzerNumber, BarzerDate and BarzerTimeOfDay

struct RangePacker : public boost::static_visitor<bool> {
	BarzelBeadAtomic left;
	BarzerRange &range;

	RangePacker(BarzerRange &r) : range(r) {}

	bool setLeft(const BarzelBeadData &dt) {
		if (dt.which() == BarzelBeadAtomic_TYPE) {
			left = boost::get<BarzelBeadAtomic>(dt);
			return true;
		} else return false;
	}

	template<class T> bool setLeft(const T&) { return false; }

	bool operator()(const BarzerNumber &rnum) {
		if (left.isNumber()) {
			const BarzerNumber &lnum = left.getNumber();
			if (lnum.isReal()) {
				if (rnum.isReal()) {
					range.dta = BarzerRange::Real((float)lnum.getReal(), (float)rnum.getReal());
				} else {
					range.dta = BarzerRange::Real((float)lnum.getReal(), (float)rnum.getInt());
				}
			} else {
				if (rnum.isReal()) {
					range.dta = BarzerRange::Real((float)lnum.getInt(), (float)rnum.getReal());
				} else {
					range.dta = BarzerRange::Integer(lnum.getInt(), rnum.getInt());
				}
			}
			//range.print(AYLOG(DEBUG) << "done: ");
			return true;
		} else return false;
	}

	bool operator()(const BarzerDate &rdate) {
		//if (left.getType() == BarzerDate_TYPE) {
		try {
			range.dta = BarzerRange::Date(boost::get<BarzerDate>(left.dta), rdate);
			return true;
		} catch (boost::bad_get) {
			AYLOG(ERROR) << "Types don't match";
			return false;
		}
		//}
//		return false;
	}

	bool operator()(const BarzerTimeOfDay &rtod) {
		try {
			range.dta = BarzerRange::TimeOfDay(boost::get<BarzerTimeOfDay>(left.dta), rtod);
			return true;
		} catch (boost::bad_get) {
			AYLOG(ERROR) << "Types don't match";
			return false;
		}
	}

	bool operator()(const BarzelBeadAtomic &data) {
		return boost::apply_visitor(*this, data.dta);
	}

	template<class T> bool operator()(const T&) {
		AYLOG(ERROR) << "Wrong range type";
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
		ADDFN(mkRange);
		ADDFN(mkEnt);
		ADDFN(mkERC);
		// arith
		ADDFN(opPlus);
		ADDFN(opMinus);
		ADDFN(opMult);
		ADDFN(opDiv);
		//logic
		ADDFN(opAnd);
		ADDFN(opOr);
		ADDFN(opLt);
		ADDFN(opGt);
		ADDFN(opEq);
		// string
		ADDFN(strConcat);
		// lookup
		ADDFN(lookupMonth);
		ADDFN(lookupWday);

	}
	#undef ADDFN

	void addFun(const char *fname, BELStoredFunction fun) {
		const uint32_t fid = universe.getStringPool().internIt(fname);
		//AYLOG(DEBUG) << "adding function(" << fname << ":" << fid << ")";
		addFun(fid, fun);
	}

	void addFun(const uint32_t fid, BELStoredFunction fun) {
		funmap.insert(BELStoredFunRec(fid, fun));
	}


	// stored functions
	#define STFUN(n) bool stfun_##n( BarzelEvalResult &result,\
							        const BarzelEvalResultVec &rvec) const

	// makers
	STFUN(mkDate) //(d) | (d,m) | (d,m,y)
	{
		//sets everything to today
		BarzerNumber m(BarzerDate::thisMonth),
					 d(BarzerDate::thisDay),
					 y(BarzerDate::thisYear),
					 tmp;

		BarzerDate date;
		// changes fields to whatever was set
		//AYLOGDEBUG(rvec.size());
		switch (rvec.size()) {
		case 3:
			if (setNumber(y, rvec[2]));
			else {
				AYLOG(DEBUG) << "(mkDate) Wrong type in the third argument: ";
				//             << rvec[2].getBeadData().which();
			}
		case 2:
			if (setNumber(m, rvec[1]));
			else {
				AYLOG(DEBUG) << "(mkDate) Wrong type in the second argument: ";
				//             << rvec[1].getBeadData().which();
			}
		case 1:
			if (!setNumber(d, rvec[0])) {
				AYLOG(DEBUG) << "(mkDate) Wrong type in the first argument: ";
				//             << rvec[0].getBeadData().which();
			}
		case 0: break; // 0 arguments = today
		default: return false;
			// huhuh
		}
		date.setDayMonthYear(d,m,y);
		//date.print(AYLOG(DEBUG) << "date formed: ");
		setResult(result, date);
		return true;
	}

	STFUN(mkTime)
	{
		//AYLOG(DEBUG) << "mkTime called";
		BarzerNumber h(0),m(0),s(0);
		BarzerTimeOfDay time;
		//AYLOGDEBUG(rvec.size());
		switch (rvec.size()) {
		case 3: time.ss = getNumber(rvec[2]).getInt();
		case 2: time.mm = getNumber(rvec[1]).getInt();
		case 1: time.hh = getNumber(rvec[0]).getInt();
			break;
		default: return false;
		}
		//AYLOG(DEBUG) << "setting result";
		setResult(result, time);
		return true;
	}



	STFUN(mkRange)
	{
		if (rvec.size() >= 2) {
			BarzerRange br;
			RangePacker rp(br);
			if (rp.setLeft(rvec[0].getBeadData())) {
				if (boost::apply_visitor(rp, rvec[1].getBeadData())) {
					setResult(result, br);
					return true;
				}
			}
		}
		return false;
	}

	const StoredEntity* fetchEntity(const char* tokname,
									const uint16_t cl, const uint16_t scl) const
	{
		const DtaIndex &dtaIdx = universe.getDtaIdx();
		const StoredToken *tok = dtaIdx.getTokByString(tokname);
		if (!tok) {
			AYLOG(ERROR) << "Invalid token: " << tokname;
			return 0;
		}
		const StoredEntityUniqId euid(tok->tokId, cl, scl);
		return dtaIdx.getEntByEuid(euid);
	}

	STFUN(mkEnt) // makes Entity
	{
		if (rvec.size() < 2) {
			AYLOG(DEBUG) << "mkEnt: Wrong number of arguments";
			return false;
		}
		try {
			const BarzerLiteral &ltrl = getAtomic<BarzerLiteral>(rvec[0]);
			const BarzerNumber &cl = getNumber(rvec[1]);
			const BarzerNumber &scl = getNumber(rvec[2]);

			const char *tokstr = universe.getStringPool().resolveId(ltrl.getId());

			if (!tokstr) {
				AYLOG(ERROR) << "Invalid literal ID: " << ltrl.getId();
				return false;
			}

			const StoredEntity *ent = fetchEntity(tokstr, cl.getInt(), scl.getInt());
			if (!ent) {
				AYLOG(ERROR) << "No such entity: ("
							 << tokstr << ", " << cl << ", " << scl << ")";
				return false;
			}

			BarzerEntityList belst;
			belst.addEntity(*ent);
			setResult(result, belst);
			return true;

		} catch (boost::bad_get) {
			AYLOG(ERROR) << "mkEnt: Wrong argument type";
		}
		return false;
	}

	STFUN(mkERC) // makes EntityRangeCombo
	{
		if (rvec.size() < 2) {
			AYLOG(ERROR) << "Need 2 arguments";
			return false;
		}
		try {
			BarzelEntityRangeCombo erc;
			const BarzerEntityList &belst = getAtomic<BarzerEntityList>(rvec[0]);

			erc.setEntityId(belst.getList().front().entId);
			erc.range = getAtomic<BarzerRange>(rvec[1]);
			setResult(result, erc);
			return true;
		} catch (boost::bad_get) {
			AYLOG(ERROR) << "Wrong argument type";
		}
		return false;
	}


	STFUN(mkPriceRC)
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

	STFUN(opAnd) // stfunc_opAnd(&result, &rvec)
	{
//		AYLOG(DEBUG) << "opAnd called";
//		AYLOGDEBUG(rvec.size());
		if (rvec.size()) {
			BarzelEvalResultVec::const_iterator ri = rvec.begin();
			do {
				if ((ri++)->getBeadData().which() == BarzelBeadBlank_TYPE)	break;
			} while (ri != rvec.end());
			result.setBeadData((--ri)->getBeadData());
		} else {
			//AYLOG(DEBUG) << "setting blank";
			BarzelBeadBlank blank;
			result.setBeadData(blank);
		}
		return true;
	}

	STFUN(opOr)
	{
		//	AYLOG(DEBUG) << "opOr called";
		//	AYLOGDEBUG(rvec.size());
		if (rvec.size()) {
			BarzelEvalResultVec::const_iterator ri = rvec.begin();
			do {
				if ((ri++)->getBeadData().which() != BarzelBeadBlank_TYPE) break;
			} while (ri != rvec.end());
			result.setBeadData((--ri)->getBeadData());
		} else {
			//AYLOG(DEBUG) << "setting blank";
			BarzelBeadBlank blank;
			result.setBeadData(blank);
		}
		return true;
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

	// string
	STFUN(strConcat) { // strfun_strConcat(&result, &rvec)
		if (rvec.size()) {
			StrConcatVisitor scv(universe);
			for (BarzelEvalResultVec::const_iterator ri = rvec.begin();
													 ri != rvec.end(); ++ri) {
				if (!boost::apply_visitor(scv, ri->getBeadData())) return false;
			}
			BarzerString bzs;
			bzs.setStr(scv.ss.str());
			setResult(result, bzs);
			return true;
		}
		return false;
	}

	// lookup

	STFUN(lookupMonth)
	{
		if (!rvec.size()) {
			AYLOG(ERROR) << "Wrong number of agruments";
			return false;
		}
		try {
			const BarzerLiteral &bl = getAtomic<BarzerLiteral>(rvec[0]);
			const uint8_t mnum = universe.getDateLookup().lookupMonth(bl.getId());
			if (!mnum) return false;
			BarzerNumber bn(mnum);
			setResult(result, bn);
			return true;
		} catch(boost::bad_get) {
			AYLOG(ERROR) << "Wrong argument type";
		}
		return false;
	}

	STFUN(lookupWday)
	{
		//AYLOG(DEBUG) << "lookupwday called";
		if (!rvec.size()) {
			AYLOG(ERROR) << "Wrong number of agruments";
			return false;
		}
		try {
			const BarzerLiteral &bl = getAtomic<BarzerLiteral>(rvec[0]);
			uint8_t mnum = universe.getDateLookup().lookupWeekday(bl.getId());
			if (!mnum) return false;
			BarzerNumber bn(mnum);
			setResult(result, bn);
			return true;
		} catch(boost::bad_get) {
			AYLOG(ERROR) << "Wrong argument type";
		}
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




