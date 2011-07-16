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
#include <barzer_datelib.h>
#include <barzer_el_chain.h>
#include <boost/assign.hpp>

namespace barzer {

namespace {

// some utility stuff


// extracts a string from BarzerString or BarzerLiteral
struct StringExtractor : public boost::static_visitor<const char*> {
	const StoredUniverse &globPools;
	StringExtractor(const StoredUniverse &u) : globPools(u) {}
	const char* operator()( const BarzerLiteral &dt ) const {
		return globPools.getStringPool().resolveId(dt.getId());
	}
	const char* operator()( const BarzerString &dt ) const
		{ return dt.getStr().c_str(); }
	const char* operator()( const BarzelBeadAtomic &dt ) const
		{ return boost::apply_visitor(*this, dt.dta); }
	template <typename T> const char* operator()( const T& ) const
		{ return 0;	}
};

const char* extractString(const StoredUniverse &u, const BarzelEvalResult &result)
{
	return boost::apply_visitor(StringExtractor(u), result.getBeadData());
}

// throwing version. only call it when catching boost::bad_get
template<class T> const T& getAtomic(const BarzelEvalResult &result) {
	return boost::get<T>(
			boost::get<BarzelBeadAtomic>(result.getBeadData()).getData());
}

template<class T> const T& getAtomic(const BarzelBeadData &dta) {
	return boost::get<T>(
			boost::get<BarzelBeadAtomic>(dta).getData());
}

template<class T> const BarzerNumber& getNumber(const T &v)
	{ return getAtomic<BarzerNumber>(v); }

//template<class T> void setResult(BarzelEvalResult&, const T&);



template<class T> T& setResult(BarzelEvalResult &result, const T &data) {
	result.setBeadData(BarzelBeadAtomic());
	boost::get<BarzelBeadAtomic>(result.getBeadData()).setData(data);
	BarzelBeadAtomic &a = boost::get<BarzelBeadAtomic>(result.getBeadData());
	return boost::get<T>(a.getData());
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
	GlobalPools &globPools;
	StrConcatVisitor(GlobalPools &u) : globPools(u) {}

	bool operator()( const BarzerString &dt ) {
		ss << dt.getStr();
		return true;
	}
	bool operator()( const BarzerLiteral &dt ) {
		switch (dt.getType()) {
		case BarzerLiteral::T_STRING:
		case BarzerLiteral::T_COMPOUND: {
			const char *str = globPools.stringPool.resolveId(dt.getId());
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
		case BarzerLiteral::T_BLANK:
		case BarzerLiteral::T_STOP:
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

template<class T> void mergePairs(std::pair<T,T> &left, const std::pair<T,T> &right) {
	//AYLOG(DEBUG) << left.first << " " << right.first;

	left.first = std::min(left.first, right.first);
	left.second = std::max(left.second, right.second);
}

bool mergeRanges(BarzerRange &r1, const BarzerRange &r2) {
	if (r1.getType() != r2.getType() || r1.isBlank()) return false;
	switch(r1.getType()) {
	case BarzerRange::Integer_TYPE:
		mergePairs(boost::get<BarzerRange::Integer>(r1.getData()),
				   boost::get<BarzerRange::Integer>(r2.getData()));
		break;
	case BarzerRange::Real_TYPE:
		mergePairs(boost::get<BarzerRange::Real>(r1.getData()),
				   boost::get<BarzerRange::Real>(r2.getData()));
		break;
	case BarzerRange::Date_TYPE:
	case BarzerRange::TimeOfDay_TYPE:
	default: return false;
	}
	return true;
}


uint32_t getTokId(const char* tokStr, const StoredUniverse &u)
{
	const StoredToken *tok = u.getDtaIdx().getTokByString(tokStr);

	if (!tok) {
		AYLOG(ERROR) << "Invalid token: " << tokStr;
		return 0;
	}
	return tok->getId();
}

uint32_t getTokId(const BarzerLiteral &l, const StoredUniverse &u)
{
	const DtaIndex dtaIdx = u.getDtaIdx();
	const char *tokStr = u.getStringPool().resolveId(l.getId());

	if (!tokStr) {
		AYLOG(ERROR) << "Invalid literal ID: " << l.getId();
		return 0;
	}
	return getTokId(tokStr, u);
}

}
// Stores map ay::UniqueCharPool::StrId -> BELFunctionStorage::function
// the function names are using format "stfun_*function name*"
// ADDFN(fname) macro is used add new functions in the constructor
// so we never have to debug typing mistakes


struct BELFunctionStorage_holder {
	GlobalPools &gpools;
	BELStoredFunMap funmap;

	#define ADDFN(n) addFun(#n, boost::mem_fn(&BELFunctionStorage_holder::stfun_##n))
	BELFunctionStorage_holder(GlobalPools &u) : gpools(u) {
		// makers
		ADDFN(test);

		ADDFN(mkDate);
		ADDFN(mkDateRange);
		ADDFN(mkDay);
		ADDFN(mkWday);
		ADDFN(mkTime);
		ADDFN(mkDateTime);
		ADDFN(mkRange);
		ADDFN(mkEnt);
		ADDFN(mkERC);
		ADDFN(mkErcExpr);
		ADDFN(mkFluff);
		ADDFN(mkLtrl);
		ADDFN(mkExprTag);
		ADDFN(mkExprAttrs);

		// getters
		ADDFN(getWeekday); // getWeekday(BarzerDate)
		ADDFN(getTokId); // (BarzerLiteral|BarzerEntity)
		ADDFN(getMDay);
		ADDFN(getYear);
		ADDFN(getLow); // (BarzerRange)
		ADDFN(getHigh); // (BarzerRange)
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
		ADDFN(opSelect);
		ADDFN(opDateCalc);
		// string
		ADDFN(strConcat);
		// lookup
		ADDFN(lookupMonth);
		ADDFN(lookupWday);

		// --
		ADDFN(filterEList); // (BarzerEntityList, BarzerNumber[, BarzerNumber[, BarzerNumber]])
	}
	#undef ADDFN

	void addFun(const char *fname, BELStoredFunction fun) {
		const uint32_t fid = gpools.stringPool.internIt(fname);
		//AYLOG(DEBUG) << "adding function(" << fname << ":" << fid << ")";
		addFun(fid, fun);
	}

	void addFun(const uint32_t fid, BELStoredFunction fun) {
		funmap.insert(BELStoredFunRec(fid, fun));
	}


	//enum { P_WEEK = 1, P_MONTH, P_YEAR, P_DECADE, P_CENTURY };

	// stored functions
	#define STFUN(n) bool stfun_##n( BarzelEvalResult &result,\
							        const BarzelEvalResultVec &rvec,\
							        const StoredUniverse &q_universe) const

    #define SETSIG(x) const char *sig = #x
	#define FERROR(x) AYLOG(ERROR) << sig << ": " << #x

	STFUN(test) {
		AYLOGDEBUG(rvec.size());
		//const BarzelEvalResult::BarzelBeadDataVec &v = rvec[0].getBeadDataVec();
		//result.setBeadData(rvec[0].getBeadDataVec());
		// BarzelEvalResult::BarzelBeadDataVec &resultVec = result.getBeadDataVec();

		AYLOGDEBUG(rvec[0].getBeadData().which());

		AYLOGDEBUG(result.isVec());
		return true;
	}

	// makers
	STFUN(mkDate) //(d) | (d,m) | (d,m,y)
	{
		//sets everything to today
		BarzerNumber m(BarzerDate::thisMonth),
					 d(BarzerDate::thisDay),
					 y(BarzerDate::thisYear),
					 tmp;

		BarzerDate &date = setResult(result, BarzerDate());
		// changes fields to whatever was set
		try {
            switch (rvec.size()) {
            case 3: y = getNumber(rvec[2]);
            case 2: m = getNumber(rvec[1]);
            case 1: d = getNumber(rvec[0]);
            case 0: break; // 0 arguments = today
            default:
                AYLOG(ERROR) << "mkDate(): Expected max 3 arguments, "
                             << rvec.size() << " given";
                return false;
            }
            date.setDayMonthYear(d,m,y);

            //date.print(AYLOG(DEBUG) << "date formed: ");
            //setResult(result, date);
            return true;
		} catch (boost::bad_get) {
		    AYLOG(DEBUG) << "mkDate(): Wrong argument type";
		}
		return false;
	}

	STFUN(mkDateRange)
	{
		SETSIG(mkDateRange(Date, Number, [Number, [Number]]));
		int day = 0, month = 0, year = 0;
		// Holy shit ... what is this stuf?!??!  (AY)
		try {
			switch(rvec.size()) {
			case 4: year = getAtomic<BarzerNumber>(rvec[3]).getInt();
			case 3: month = getAtomic<BarzerNumber>(rvec[2]).getInt();
			case 2: {
/*
				const BarzelBeadAtomic& atomic1 = boost::get<BarzelBeadAtomic>( rvec[0].getBeadData() );
				const BarzelBeadAtomic& atomic2 = boost::get<BarzelBeadAtomic>( rvec[1].getBeadData() );
				/// trying to see if these are 2 dates 
				if( atomic1.isDate() && atomic2.isDate() ) {
					const BarzerDate& date1 = boost::get<BarzerDate>( atomic1.getData() );
					const BarzerDate& date2 = boost::get<BarzerDate>( atomic2.getData() );
					BarzerRange range;
					range.setData(BarzerRange::Date(date1, date2));
					setResult(result, range);
					return true;
				}
*/
				day = getAtomic<BarzerNumber>(rvec[1]).getInt();
				const BarzerDate &date = getAtomic<BarzerDate>(rvec[0]);
				BarzerDate_calc c;
				c.set(date.year + year, date.month + month, date.day + day);
				BarzerRange range;
				range.setData(BarzerRange::Date(date, c.d_date));
				setResult(result, range);
				return true;
			}
			default:
				AYLOG(ERROR) << sig << ": Need 2-4 arguments";
			}
		} catch (boost::bad_get) {
			AYLOG(ERROR) << sig << ": Wrong argument type";
		}
		return false;
	}


	STFUN(mkDay) {
		SETSIG(mkDay(Number));
		if (!rvec.size()) {
			AYLOG(ERROR) << sig << ": Need an argument";
			return false;
		}
		try {
			BarzerDate_calc calc;
			calc.setToday();
			calc.dayOffset(getNumber(rvec[0]).getInt());
			setResult(result, calc.d_date);
			return true;
		} catch(boost::bad_get) {
			AYLOG(ERROR) << sig << ": Wrong argument type";
		}
		return false;
	}

	STFUN(mkWday)
	{
		if (rvec.size() < 2) {
			AYLOG(ERROR) << "mkWday(Number, Number): Need 2 arguments";
			return false;
		}
		try {
			int fut  = getAtomic<BarzerNumber>(rvec[0]).getInt();
			uint8_t wday = getAtomic<BarzerNumber>(rvec[1]).getInt();
			BarzerDate_calc calc(fut);
			calc.setToday();
			calc.setWeekday(wday);
			setResult(result, calc.d_date);
			return true;
		} catch (boost::bad_get) {
			AYLOG(ERROR) << "mkWday(Number, Number): Wrong argument type";
		}
		return false;
	}


	STFUN(mkTime)
	{
		//AYLOG(DEBUG) << "mkTime called";
		BarzerTimeOfDay &time = setResult(result, BarzerTimeOfDay());
		try {
			//BarzerNumber h(0),m(0),s(0);
			//BarzerTimeOfDay time;
			int hh(0), mm(0), ss(0);
			//AYLOGDEBUG(rvec.size());
			switch (rvec.size()) {
			case 3: ss = getNumber(rvec[2]).getInt();
			case 2: mm = getNumber(rvec[1]).getInt();
			case 1: hh = getNumber(rvec[0]).getInt();
				break;
			case 0: {
				std::time_t t = std::time(0);
				std::tm *tm = std::localtime(&t);
				hh = tm->tm_hour;
				mm = tm->tm_min;
				ss = tm->tm_sec;
			}
				break;
			default:
				AYLOG(ERROR) << "mkTime(): Expected max 3 arguments, "
				             << rvec.size() << " given";
				return false;
			}
			time.setHHMMSS( hh, mm, ss );
			return true;
		} catch (boost::bad_get) {
			AYLOG(ERROR) << "mkTime(): Wrong argument type";
		}
		return false;
	}

	// applies  BarzerDate/BarzerTimeOfDay/BarzerDateTime to BarzerDateTime
	// to construct a timestamp
	struct DateTimePacker : public boost::static_visitor<bool> {
		BarzerDateTime &dtim;
		DateTimePacker(BarzerDateTime &d) : dtim(d) {}
		bool operator()(const BarzerDate &data) {
			dtim.setDate(data);
			return true;
		}
		bool operator()(const BarzerTimeOfDay &data) {
			dtim.setTime(data);
			return true;
		}
		bool operator()(const BarzerDateTime &data) {
			if (data.hasDate()) dtim.setDate(data.getDate());
			if (data.hasTime()) dtim.setTime(data.getTime());
			return true;
		}
		bool operator()(const BarzelBeadAtomic &data) {
			//AYLOG(DEBUG) << "got atomic of type: " << data.getType();
			return boost::apply_visitor(*this, data.getData());
		}
		// not applicable
		template<class T> bool operator()(const T&)
		{
			AYLOG(ERROR) << "Wrong argument type";
			return false;
		}

	};

	STFUN(mkDateTime) {
		//AYLOGDEBUG(rvec.size());
		BarzerDateTime dtim;
		DateTimePacker v(dtim);

		for (BarzelEvalResultVec::const_iterator ri = rvec.begin();
				ri != rvec.end(); ++ri) {
			if (!boost::apply_visitor(v, ri->getBeadData())) {
				AYLOG(DEBUG) << "fail";
				return false;
			}
		}
		setResult(result, dtim);
		return true;
	}

	STFUN(mkEnityList) {
		// <- yanis goes here
		return false;
	}
	// tries to pack 2 values into 1 range
	// always assumes the values are atomic and of the same type
	// the only supported types at the moments are
	// BarzerNumber, BarzerDate and BarzerTimeOfDay

	struct RangePacker : public boost::static_visitor<bool> {
		GlobalPools &globPools;
		BarzerRange &range;

		//const BarzelBeadAtomic *left;
		uint32_t cnt;

		RangePacker(GlobalPools &u, BarzerRange &r) : globPools(u), range(r), cnt(0) {}

		bool operator()(const BarzerLiteral &ltrl) {
			//AYLOG(DEBUG) << "mkRange arg " << cnt << ":BarzerLiteral";
			ay::UniqueCharPool &spool = globPools.stringPool;
			if (ltrl.getId() == spool.internIt("DESC")) {
				range.setDesc();
				return true;
			} else if (ltrl.getId() == spool.internIt("ASC")) {
				return true;
			} else {
				AYLOG(ERROR) << "Unknown literal: `"
							 << spool.resolveId(ltrl.getId()) << "'";
				return false;
			}
		}

		template<class T> void setSecond(std::pair<T,T> &p, const T &v) {
			if (p.first < v || range.isDesc())
				p.second = v;
			else
				p.first = v;
		}

		bool operator()(const BarzerNumber &rnum) {
			//AYLOG(DEBUG) << "mkRange arg " << cnt << ":BarzerNumber";
			if (cnt) {
				if (rnum.isInt()) {
					if (range.getType() == BarzerRange::Integer_TYPE) {
						BarzerRange::Integer &ip
							= boost::get<BarzerRange::Integer>(range.getData());
						//ip.second = rnum.getInt();
						setSecond(ip, rnum.getInt());
					} else if (range.getType() == BarzerRange::Real_TYPE) {
						BarzerRange::Real &rp
							= boost::get<BarzerRange::Real>(range.getData());
						//rp.second = (float) rnum.getInt();
						setSecond(rp, (float) rnum.getInt());
					}
				} else if (rnum.isReal()) {
					if (range.getType() == BarzerRange::Real_TYPE) {
						BarzerRange::Real &rp
							= boost::get<BarzerRange::Real>(range.getData());
						//rp.second = rnum.getReal();
						setSecond(rp, (float)rnum.getReal());
					} else {
						int lf = boost::get<BarzerRange::Integer>(range.getData()).first;
						BarzerRange::Real newRange((float) lf, (float) lf);
						setSecond(newRange, (float)rnum.getReal());
						range.setData(newRange);
						//range.setData(BarzerRange::Real((float) lf, rnum.getReal()));
					}
				} else {
					return false;
				}
			} else {
				if (rnum.isReal()) {
					float f = rnum.getReal();
					range.setData(BarzerRange::Real(f, f));
				} else {
					int i = rnum.getInt();
					range.setData(BarzerRange::Integer(i, i));
				}
			}
			return true;
		}


		bool operator()(const BarzerDate &rdate) {
			//AYLOG(DEBUG) << "mkRange arg " << cnt << ":BarzerDate";
			if (cnt) {
				try {
					BarzerRange::Date &dp
						= boost::get<BarzerRange::Date>(range.getData());
					setSecond(dp, rdate);
				} catch (boost::bad_get) {
					AYLOG(ERROR) << "Types don't match: " << range.getData().which();
					return false;
				}
			} else {
				range.setData(BarzerRange::Date(rdate, rdate));
			}
			return true;
		}

		bool operator()(const BarzerDateTime &dt) {
			if (cnt) {
				try {
					BarzerRange::DateTime &r
						= boost::get<BarzerRange::DateTime>(range.getData());
					setSecond(r, dt);
				} catch (boost::bad_get) {
					AYLOG(ERROR) << "Types don't match";
					return false;
				}
			} else {
				range.setData(BarzerRange::DateTime(dt, dt));
			}
			return true;
		}

		bool operator()(const BarzerTimeOfDay &rtod) {
			//AYLOG(DEBUG) << "mkRange arg " << cnt << ":BarzerTimeOfDay";
			if (cnt) {
				try {
					BarzerRange::TimeOfDay &todp
						= boost::get<BarzerRange::TimeOfDay>(range.getData());
					todp.second = rtod;
				} catch (boost::bad_get) {
					AYLOG(ERROR) << "Types don't match";
					return false;
				}
			} else {
				range.setData(BarzerRange::TimeOfDay(rtod, rtod));
			}
			return true;
		}

		bool operator()(const BarzerRange &lr) {
		    if (cnt) {
		        mergeRanges(range, lr);
		    } else {
		        range = lr;
		    }
		    return true;
		}

		bool operator()(const BarzelBeadAtomic &data) {
			//AYLOG(DEBUG) << "mkRange arg " << cnt;
			if(boost::apply_visitor(*this, data.getData())) {
				if (data.getType() != BarzerLiteral_TYPE) ++cnt;
				return true;
			}
			return false;
		}

		template<class T> bool operator()(const T&) {
			AYLOG(ERROR) << "Wrong range type";
			return false;
		}

	};

	STFUN(mkRange)
	{
		BarzerRange br;
		RangePacker rp(gpools, br);

		for (BarzelEvalResultVec::const_iterator ri = rvec.begin();
								ri != rvec.end(); ++ri)
			if (!boost::apply_visitor(rp, ri->getBeadData())) return false;

		setResult(result, br);
		return true;
	}



	const StoredEntity* fetchEntity(uint32_t tokId,
									const uint16_t cl, const uint16_t scl) const
	{
		const StoredEntityUniqId euid(tokId, cl, scl);
		return gpools.dtaIdx.getEntByEuid(euid);
	}


	struct EntityPacker : public boost::static_visitor<bool> {
		//const char *tokStr;
		uint32_t cnt, tokId;
		uint16_t cl, scl;
		//const BELFunctionStorage_holder &holder;
		const StoredUniverse &universe;

		//EntityPacker(const BELFunctionStorage_holder &h)
		EntityPacker(const StoredUniverse &u)
			: cnt(0), tokId(INVALID_STORED_ID), cl(0), scl(0), universe(u) {}

		bool operator()(const BarzerLiteral &ltrl) {
			return (tokId = getTokId(ltrl, universe));
		}
		bool operator()(const BarzerNumber &rnum) {
			uint16_t ui = (uint16_t) rnum.getInt();
			if(cnt++) {
				//AYLOG(DEBUG) << "setting subclass: " << ui;
				scl = ui;
			} else {
				//AYLOG(DEBUG) << "setting class: " << ui;
				cl = ui;
			}
			return true;
		}
		bool operator()(const BarzelBeadAtomic &data) {
			if(boost::apply_visitor(*this, data.dta))
				return true;
			return false;
		}
		template<class T> bool operator()(const T&) {
			AYLOG(ERROR) << "Wrong range type";
			return false;
		}
	};

	STFUN(mkEnt) // makes Entity
	{

		//EntityPacker ep(*this);
		EntityPacker ep(q_universe);

		for (BarzelEvalResultVec::const_iterator ri = rvec.begin();
								ri != rvec.end(); ++ri)
			if (!boost::apply_visitor(ep, ri->getBeadData())) return false;

		/*AYLOG(DEBUG) << "making entity (" << ep.tokId << ","
										  << ep.cl << ","
										  << ep.scl << ")"; //*/
		const StoredEntityUniqId euid(ep.tokId, ep.cl, ep.scl);
		setResult(result, euid);
		return true;
	}

	// tries to construct an EntityRangeCombo from a sequence of BeadData
	// first BarzerEntity counts as entity the rest - as units
	// can also merge with another ERC
	struct ERCPacker : public boost::static_visitor<bool> {
		uint8_t num;
		BarzerEntityRangeCombo &erc;
		ERCPacker(BarzerEntityRangeCombo &e) : num(0), erc(e) {}

		bool operator()(const BarzerEntityList &el) {
			return (*this)(el.getList().front());
		}
		bool operator()(const BarzerEntity &euid) {
			if (num) {
				erc.setUnitEntity(euid);
			} else {
				erc.setEntity(euid);
			}
			++num;
			return true;
		}
		// merges N ERC together. Tries to select the best entity/unit
		bool operator()(const BarzerEntityRangeCombo &other) {
			if (num) {
				const BarzerEntity &e = other.getEntity(),
								   &u = other.getUnitEntity();
				if ((!erc.getEntity().isValid()) && e.isValid())
					erc.setEntity(e);
				if ((!erc.getUnitEntity().isValid()) && u.isValid())
					erc.setUnitEntity(u);

				BarzerRange &l = erc.getRange();
				if (l.isBlank()) {
					//AYLOG(DEBUG) << "blank";
					erc.setRange(other.getRange());
					return true;
				} else return mergeRanges(l, other.getRange());
			} else {
				erc.setRange(other.getRange());
				erc.setEntity(other.getEntity());
				erc.setUnitEntity(other.getUnitEntity());
			}
			++num;
			return true;
		}

		bool operator()(const BarzerRange &el)
			{ erc.setRange(el); return true;	}
		bool operator()(const BarzelBeadAtomic &data) {
			return boost::apply_visitor(*this, data.getData());
		}
		// not applicable
		template<class T> bool operator()(const T& v) {
			AYLOG(ERROR) << "ERCPacker: Wrong argument type";
			return false;
		}

	};

	STFUN(mkERC) // makes EntityRangeCombo
	{
		//AYLOGDEBUG(rvec.size());
		//AYLOG(DEBUG) << rvec[0].getBeadData().which();
		BarzerEntityRangeCombo erc;
		ERCPacker v(erc);
		for (BarzelEvalResultVec::const_iterator ri = rvec.begin();
						ri != rvec.end(); ++ri)
			if (!boost::apply_visitor(v, ri->getBeadData())) {
					AYLOG(DEBUG) << "fail. type: " << ri->getBeadData().which();
						return false;
			}

		setResult(result, erc);
		return true;
	}

	struct ErcExprPacker : public boost::static_visitor<bool> {
		BarzerERCExpr expr;
		GlobalPools &gpools;
		ErcExprPacker(GlobalPools &gp) : gpools(gp) {}

		bool operator()(const BarzerLiteral &ltrl) {
			ay::UniqueCharPool &sp = gpools.stringPool;
			if (ltrl.getId() == sp.internIt("and")) {
				//AYLOG(DEBUG) << "AND";
				expr.setType(BarzerERCExpr::T_LOGIC_AND);
			} else if (ltrl.getId() == sp.internIt("or")) {
				//AYLOG(DEBUG) << "OR";
				expr.setType(BarzerERCExpr::T_LOGIC_OR);
			} else {
				AYLOG(ERROR) << "mkErcExpr(Literal,ERC,ERC ...):"
							 << "Unknown literal: " << sp.resolveId(ltrl.getId());
				return false;
			}
			return true;
		}
		bool operator()(const BarzerEntityRangeCombo &erc) {
			expr.addToExpr(erc, expr.d_type);
			return true;
		}
		bool operator()(const BarzelBeadAtomic &data) {
			return boost::apply_visitor(*this, data.getData());
		}
		// not applicable
		template<class T> bool operator()(const T& v) {
			AYLOG(ERROR) << "mkErcExpr(Literal,ERC,ERC ...): Type mismatch";
			return false;
		}
	};

	STFUN(mkErcExpr)
	{
		if (rvec.size() < 3) {
			AYLOG(ERROR) << "mkErcExpr(Literal,ERC,ERC ...): needs at least 3 arguments";
			return false;
		}

		ErcExprPacker p(gpools);

		for(BarzelEvalResultVec::const_iterator it = rvec.begin();
											    it != rvec.end(); ++it) {
			if (!boost::apply_visitor(p, it->getBeadData())) return false;
		}
		setResult(result, p.expr);
		return true;
	}

	STFUN(mkLtrl)
	{
		if (!rvec.size()) {
			AYLOG(ERROR) << "Need an argument";
			return false;
		}

		const char* str = extractString(gpools, rvec[0]);
		if (!str) {
			AYLOG(ERROR) << "Need a string";
			return false;
		}

		uint32_t id = gpools.stringPool.internIt(str);
		BarzerLiteral lt;
		lt.setString(id);

		setResult(result, lt);
		return true; //*/
	}

	// set "stop" type on the incoming literal.
	STFUN(mkFluff)
	{
		try {
			BarzerLiteral &ltrl = setResult(result, BarzerLiteral());
			ltrl.setStop();
			for (BarzelEvalResultVec::const_iterator rvit = rvec.begin();
													 rvit != rvec.end();
													 ++rvit) {
				const BarzelBeadDataVec &vec = rvit->getBeadDataVec();
				for (BarzelBeadDataVec::const_iterator bdit = vec.begin();
													   bdit != vec.end();
													   ++bdit) {
					const BarzerLiteral &fl = getAtomic<BarzerLiteral>(*bdit);
					if (fl.isString() || fl.isCompound()) {
						ltrl.setId(fl.getId());
						return true;
					}
				}
			}
			return true;
		} catch (boost::bad_get&) {
			AYLOG(ERROR) << "mkFluff(BarzerLiteral): Wrong argument type";
			return false;
		}
	}

	struct ExprTagPacker : public boost::static_visitor<bool> {
		BarzelBeadExpression &expr;
		ExprTagPacker(BarzelBeadExpression &e) : expr(e) {}

		bool operator()(const BarzelBeadBlank&) {
			AYLOG(ERROR) << "BarzelBeadBlank encountered";
			return false;
		}

		template <class T> bool operator()(const T &data) {
			expr.addChild(data);
			return true;
		}
	};

	STFUN(mkExprTag) {
		if (rvec.size()) {
			try {
				// to avoid too much copying
				result.setBeadData(BarzelBeadExpression());
				BarzelBeadExpression &expr
					= boost::get<BarzelBeadExpression>(result.getBeadData());
				ExprTagPacker etp(expr);

				expr.setSid(getAtomic<BarzerLiteral>(rvec[0]).getId());
				for (BarzelEvalResultVec::const_iterator it = rvec.begin()+1;
														 it != rvec.end();
														 ++it) {
					if(!boost::apply_visitor(etp, it->getBeadData())) return false;
				}
				return true;
			} catch (boost::bad_get) {
				AYLOG(ERROR) << "Type mismatch";
			}
		}
		return false;
	}

	#define GETID(x) getAtomic<BarzerLiteral>(x).getId()
	STFUN(mkExprAttrs) {
		if (rvec.size()) {
			try {
				result.setBeadData(BarzelBeadExpression());
				BarzelBeadExpression &expr
					= boost::get<BarzelBeadExpression>(result.getBeadData());
				expr.setSid(BarzelBeadExpression::ATTRLIST);

				// making sure we have even sized length
				size_t len = rvec.size() & (~1);
				for (size_t i = 0; i < len; i+=2)
					expr.addAttribute( GETID(rvec[i]), GETID(rvec[i+1]) );
				return true;
			} catch (boost::bad_get) {
				AYLOG(ERROR) << "Type mismatch";
			}
		}
		return false;
	}
	#undef GETID

	// getters

	STFUN(getWeekday) {
		BarzerDate bd;
		if (rvec.size())
			bd = getAtomic<BarzerDate>(rvec[0]);
		else bd.setToday();

		BarzerNumber n(bd.getWeekday());
		setResult(result, n);
		return true;
	}

	STFUN(getMDay) {
		BarzerDate bd;
		if (rvec.size())
			bd = getAtomic<BarzerDate>(rvec[0]);
		else bd.setToday();
		setResult(result, BarzerNumber(bd.day));
		return true;
	}

	STFUN(getYear) {
		BarzerDate bd;
		if (rvec.size())
			bd = getAtomic<BarzerDate>(rvec[0]);
		else bd.setToday();
		setResult(result, BarzerNumber(bd.year));
		return true;
	}

	struct TokIdGetter : boost::static_visitor<bool> {
		const StoredUniverse &universe;
		uint32_t tokId;
		TokIdGetter(const StoredUniverse &u) : universe(u), tokId(0) {}

		bool operator()(const BarzerLiteral &dt) {
			return ((tokId = getTokId(dt, universe)));
		}

		bool operator()(const BarzelBeadAtomic &data) {
			return boost::apply_visitor(*this, data.getData());
		}
		// not applicable
		template<class T> bool operator()(const T&) {
			AYLOG(ERROR) << "getTokId(BarzerLiteral|BarzerEntity): Type mismatch";
			return false;
		}
	};

	STFUN(getTokId)
	{
		if (!rvec.size()) {
			AYLOG(ERROR) << "getTokId(BarzerLiteral|BarzerEntity) needs an argument";
			return false;
		}
		TokIdGetter tig(q_universe);
		boost::apply_visitor(tig, rvec[0].getBeadData());
		setResult(result, BarzerNumber((int)tig.tokId));
		return true;
	}

	struct RangeGetter : public boost::static_visitor<bool> {
		BarzelEvalResult &result;
		uint8_t pos;
		RangeGetter(BarzelEvalResult &r, uint8_t p = 0) : result(r), pos(p) {}


		bool set(const BarzerNone) { return false; }
		bool set(int i) {
			setResult(result, BarzerNumber(i));
			return true;
		}
		bool set(float f) {
			setResult(result, BarzerNumber(f));
			return true;
		}

		template<class T> bool set(const T &v) {
			setResult(result, v);
			return true;
		}

		template<class T> bool operator()(const std::pair<T,T> &dt)
			{ return set(pos ? dt.second : dt.first); }
		bool operator()(const BarzerRange &r)
			{ return boost::apply_visitor(*this, r.getData()); }
		bool operator()(const BarzelBeadAtomic &data)
			{ return boost::apply_visitor(*this, data.getData()); }
		// not applicable
		template<class T> bool operator()(const T&) {
			AYLOG(ERROR) << "Type mismatch";
			return false;
		}

	};

	STFUN(getLow)
	{
		SETSIG(getLow(BarzerRange));
		RangeGetter rg(result, 0);
		if (rvec.size()) {
			return boost::apply_visitor(rg, rvec[0].getBeadData());
		} else {
			FERROR(Expected at least 1 argument);
		}
		return false;
	}

	STFUN(getHigh)
	{
		SETSIG(getLow(BarzerRange));
		RangeGetter rg(result, 1);
		if (rvec.size()) {
			return boost::apply_visitor(rg, rvec[0].getBeadData());
		} else {
			FERROR(Expected at least 1 argument);
		}
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
		BarzerNumber &bn = setResult(result, BarzerNumber()); // NaN

		try {
		    if (rvec.size()) {
                ArithVisitor<minus> visitor(bn);

                BarzelEvalResultVec::const_iterator ri = rvec.begin();

                // sets bn to the value of the first element of rvec
                bn = getNumber(*ri);
                while(++ri != rvec.end())
                    if (!boost::apply_visitor(visitor, ri->getBeadData())) return false;
                return true;
		    }
		} catch (boost::bad_get) {
		    AYLOG(ERROR) << "opMinus(): Wrong arguments type";
		}
		//setResult(result, bn); // most likely NaN if something went wrong
		return false;
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
		BarzerNumber &bn = setResult(result, BarzerNumber()); // NaN
		try {
            if (rvec.size()) {
                ArithVisitor<div> visitor(bn);

                BarzelEvalResultVec::const_iterator ri = rvec.begin();

                // sets bn to the value of the first element of rvec
                bn = getNumber(*ri);
                while(++ri != rvec.end())
                    if (!boost::apply_visitor(visitor, ri->getBeadData())) return false;
                return true;
            }
		} catch (boost::bad_get) {
		    AYLOG(ERROR) << "opDiv(): Wrong arguments type";
		}
		//setResult(result, bn); // most likely NaN if something went wrong
		return false;

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

	STFUN(opSelect) {
		if (rvec.size() < 3) {
			AYLOG(ERROR) << "opSelect: need at least 3 arguments";
			return false;
		}

		if (!(rvec.size() & 1)) {
			AYLOG(ERROR) << "opSelect: need an odd amount of arguments";
		}

		const BarzelBeadData &var = rvec[0].getBeadData();

		for (BarzelEvalResultVec::const_iterator it = rvec.begin()+1;
												 it != rvec.end(); it += 2) {
			const BarzelBeadData &rec = it->getBeadData();

			if (beadsEqual(var, rec)) {
				result.setBeadData((it+1)->getBeadData());
				return true;
			} //*/
		}
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

	STFUN(opDateCalc)
	{
		const char *sig = "opDateCalc(Date ,Number[, Number[, Number]]):";
		try {

			int month = 0, year = 0, day = 0;
			switch (rvec.size()) {
			case 4: year = getAtomic<BarzerNumber>(rvec[3]).getInt();
			case 3: month = getAtomic<BarzerNumber>(rvec[2]).getInt();
			case 2: {
				day = getAtomic<BarzerNumber>(rvec[1]).getInt();
				const BarzerDate &date = getAtomic<BarzerDate>(rvec[0]);
				BarzerDate_calc c;
				c.set(date.year + year,
				      date.month + month,
				      date.day + day);
				setResult(result, c.d_date);
				return true;
			}
			default: AYLOG(ERROR) << sig << " Need 2-4 arguments";
			}
		} catch (boost::bad_get) {
			AYLOG(ERROR) << sig << " type mismatch";
		}
		return false;
	}


	// string
	STFUN(strConcat) { // strfun_strConcat(&result, &rvec)
		if (rvec.size()) {
			StrConcatVisitor scv(gpools);
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
			const uint8_t mnum = gpools.dateLookup.lookupMonth(bl.getId());
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
			uint8_t mnum = gpools.dateLookup.lookupWeekday(bl.getId());
			if (!mnum) return false;
			BarzerNumber bn(mnum);
			setResult(result, bn);
			return true;
		} catch(boost::bad_get) {
			AYLOG(ERROR) << "Wrong argument type";
		}
		return false;
	}


	typedef boost::function<bool(const BarzerEntity&, uint32_t, uint32_t, uint32_t)> ELCheckFn;

	inline static bool checkClass(const BarzerEntity& ent, uint32_t cl, uint32_t scl = 0, uint32_t id = 0)
		{ return ent.eclass.ec == (uint16_t)cl; }
	inline static bool checkSC(const BarzerEntity& ent, uint32_t cl, uint32_t scl, uint32_t id = 0)
		{ return ent.eclass.subclass == (uint16_t)scl && checkClass(ent, cl); }
	inline static bool checkId(const BarzerEntity& ent, uint32_t cl, uint32_t scl, uint32_t id)
		{ return ent.tokId == id && checkSC(ent, cl, scl); }


	STFUN(filterEList) // (BarzerEntityList, BarzerNumber[, BarzerNumber[, BarzerNumber]])
	{
		//static const char *sig =
		//	"filterEList(BarzerEntityList, BarzerNumber, [BarzerNumber, [BarzerNumber]])";
		SETSIG(filterEList(BarzerEntityList, BarzerNumber, [BarzerNumber, [BarzerNumber]]));
		BarzerEntityList outlst;
		const BarzerEntityList::EList lst = getAtomic<BarzerEntityList>(rvec[0]).getList();
		uint32_t cl = 0, scl = 0, id = 0;
		ELCheckFn pred = checkClass;
		bool setf = true;
		try {
			switch(rvec.size()) {
			case 4:
				id = getAtomic<BarzerNumber>(rvec[3]).getInt();
				pred = checkId;
				setf = false;
			case 3:
				scl = getAtomic<BarzerNumber>(rvec[2]).getInt();
				if (setf) pred = checkSC;
			case 2:
				id = getAtomic<BarzerNumber>(rvec[1]).getInt();
				for(BarzerEntityList::EList::const_iterator it = lst.begin();
														    it != lst.end();
														    ++it) {
					if (pred(*it, cl, scl, id)) outlst.addEntity(*it);
				}
				setResult(result, outlst);
				return true;
			default:
				AYLOG(ERROR) << sig << " Need at least 2 arguments";
			}
		} catch (boost::bad_get) {
			AYLOG(ERROR) << sig << ": Type mismatch";
		}
		return false;
	}
	#undef FERROR
    #undef SETSIG
	#undef STFUN
};

BELFunctionStorage::BELFunctionStorage(GlobalPools &u) : globPools(u),
		holder(new BELFunctionStorage_holder(u)) { }

BELFunctionStorage::~BELFunctionStorage()
{
	delete holder;
}



bool BELFunctionStorage::call(const char *fname, BarzelEvalResult &er,
		                                   const BarzelEvalResultVec &ervec,
		                                   const StoredUniverse &u) const
{
	//AYLOG(DEBUG) << "calling function name `" << fname << "'";
	const uint32_t fid = globPools.stringPool.getId(fname);
	if (fid == ay::UniqueCharPool::ID_NOTFOUND) {
		AYLOG(ERROR) << "No such function name: `" << fname << "'";
		return false;
	}
	return call(fid, er, ervec, u);
}

bool BELFunctionStorage::call(const uint32_t fid, BarzelEvalResult &er,
									              const BarzelEvalResultVec &ervec,
									              const StoredUniverse &u) const
{
	//AYLOG(DEBUG) << "calling function id `" << fid << "'";
	const BELStoredFunMap::const_iterator frec = holder->funmap.find(fid);
	if (frec == holder->funmap.end()) {
		AYLOG(ERROR) << "No such function id: " << fid;
		return false;
	}
	return frec->second(holder, er, ervec, u);

}

}




