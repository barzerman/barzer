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
#include <barzer_barz.h>
#include <barzer_el_function_util.h>


namespace barzer {

namespace {

    // sig is the function's signature
    void pushFuncError( BarzelEvalContext& ctxt, const char* funcName, const char* error, const char* sig=0 )
    {
        std::stringstream ss;
        ss << "<funcerr func=\"" << ( funcName ? funcName: "" ) << "\">" << ( error ? error: "" ) ;
        if( sig ) {
            ss << "<sig>" << sig << "</sig>";
        }
        ss << "</funcerr>";

        ctxt.getBarz().barzelTrace.pushError( ss.str().c_str() );
    }
// some utility stuff


// extracts a string from BarzerString or BarzerLiteral
struct StringExtractor : public boost::static_visitor<const char*> {
	const GlobalPools &globPools;
	StringExtractor(const GlobalPools &u) : globPools(u) {}
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

const char* extractString(const GlobalPools &u, const BarzelEvalResult &result)
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

template<class T> const T* getAtomicPtr(const BarzelBeadData &dta)
{
    if( is_BarzelBeadAtomic(dta) ) {
        const BarzelBeadAtomic& atomic = boost::get<BarzelBeadAtomic>(dta);
        const BarzelBeadAtomic_var& atDta = atomic.getData();

        return boost::get<T>( &atDta );
    } else
        return 0;
}
template<class T> const T* getAtomicPtr(const BarzelEvalResult &res)
{
    return getAtomicPtr<T>( res.getBeadData() );
}

template<class T> const BarzerNumber& getNumber(const T &v)
{
    return getAtomic<BarzerNumber>(v);
}

//template<class T> void setResult(BarzelEvalResult&, const T&);



template<class T> T& setResult(BarzelEvalResult &result, const T &data) {
	result.setBeadData(BarzelBeadAtomic());
	boost::get<BarzelBeadAtomic>(result.getBeadData()).setData(data);
	BarzelBeadAtomic &a = boost::get<BarzelBeadAtomic>(result.getBeadData());
	return boost::get<T>(a.getData());
}

bool setResult(BarzelEvalResult &result, bool data) {
    if (data) {
        setResult(result, BarzerNumber(1));
    } else {
        result.setBeadData(BarzelBeadBlank());
    }
    return data;
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
struct lt {
    template <class T> bool operator()(T v1, T v2) { return v1 < v2; }
};
struct gt {
    template <class T> bool operator()(T v1, T v2) { return v1 > v2; }
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
    BarzelEvalContext& d_ctxt;
    const char* d_funcName;

	StrConcatVisitor(GlobalPools &u, BarzelEvalContext& ctxt, const char* funcName ) :
        globPools(u),
        d_ctxt(ctxt),
        d_funcName(funcName)
    {}

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
                pushFuncError(d_ctxt, d_funcName, "Unknown string ID" );
				return false;
			}
		}
		case BarzerLiteral::T_PUNCT:
			ss << (char)dt.getId();
			return true;
		case BarzerLiteral::T_BLANK:
            ss << " ";
            return true;
		case BarzerLiteral::T_STOP:
			return true;
		default:
            pushFuncError(d_ctxt, d_funcName, "Wrong literal type" );
			return true;
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

	left.first = std::min(left.first, right.first);
	left.second = std::max(left.second, right.second);
}

bool mergeRanges(BarzerRange &r1, const BarzerRange &r2) {
    if( r1.isBlank() ) r1 = r2;

	if (r1.getType() != r2.getType() ) {
        if( (r1.isInteger() ||r1.isReal()) && (r2.isInteger() ||r2.isReal()) ) {
            BarzerRange::Real realPair;
            const BarzerRange::Real *otherRealPair = 0;

            const BarzerRange::Integer* r1Int = r1.getInteger();
            if( r1Int ) {
                otherRealPair = r2.getReal();
                realPair.first = (double)( r1Int->first );
                realPair.second = (double)( r1Int->second );

            } else {
                otherRealPair = r1.getReal();
                const BarzerRange::Integer* r2Int = r2.getInteger();
                if( r2Int ) {
                    realPair.first = (double)( r2Int->first );
                    realPair.second = (double)( r2Int->second );
                } else { // this is impossible
		            AYLOG(ERROR) << "Internal inconsistency";
                    return false;
                }
            }
            if( !otherRealPair ) {  // this is impossible
                AYLOG(ERROR) << "Internal inconsistency";
                return false;
            }
            mergePairs( realPair, *otherRealPair );

            BarzerRange::Data dta = realPair;
            r1.setData( dta );
            return true;
        }
        // if types dont match and cant be reconciled
        return false;
    }
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
		return 0xffffffff;
	}
	return tok->getId();
}

uint32_t getTokId(const BarzerLiteral &l, const StoredUniverse &u)
{
	const char *tokStr = u.getStringPool().resolveId(l.getId());

	if (!tokStr) {
		AYLOG(ERROR) << "Invalid literal ID: " << l.getId();
		return 0xffffffff;
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
		ADDFN(mkMonth);
		ADDFN(mkTime);
		ADDFN(mkDateTime);
		ADDFN(mkRange);
		ADDFN(mkEnt);
		ADDFN(mkERC);
		ADDFN(mkErcExpr);
		ADDFN(mkFluff);
		// ADDFN(mkLtrl);
		ADDFN(mkExprTag);
		ADDFN(mkExprAttrs);

		// getters
		ADDFN(getWeekday); // getWeekday(BarzerDate)
		ADDFN(getTokId); // (BarzerLiteral|BarzerEntity)
		ADDFN(getMDay);
		ADDFN(setMDay);
		ADDFN(getYear);
		ADDFN(getLow); // (BarzerRange)
		ADDFN(getHigh); // (BarzerRange)
		ADDFN(isRangeEmpty); // (BarzerRange or ERC) - returns true if range.lo == range.hi
		// arith
		ADDFN(textToNum);
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

        /// array
		ADDFN(arrSz);
		ADDFN(arrIndex);
		ADDFN(typeFilter);
		ADDFN(setUnmatch);

		// --
		ADDFN(filterEList); // filters entity list by class/subclass (BarzerEntityList, BarzerNumber[, BarzerNumber[, BarzerNumber]])
        ADDFN(topicFilterEList);
		ADDFN(mkEntList); // (BarzerEntity, ..., BarzerEntity) will also accept BarzerEntityList

        // ent properties
		ADDFN(entClass); // (Entity)
		ADDFN(entSubclass); // (Entity)
		ADDFN(entId); // (Entity)
		ADDFN(entSetSubclass); // (Entity,new subclass)

        // erc properties
		ADDFN(getEnt); // ((ERC|Entity)[,entity]) -- when second parm passed replaces entity with it
		ADDFN(getRange); // ((ERC|Entity)[,range]) -- when second parm passed replaces range with it

	}
	#undef ADDFN

	void addFun(const char *fname, BELStoredFunction fun) {
		const uint32_t fid = gpools.internString_internal(fname);
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
							        const StoredUniverse &q_universe, BarzelEvalContext& ctxt ) const

    #define SETSIG(x) const char *sig = #x
    #define SETFUNCNAME(x) const char *func_name = #x

	#define FERROR(x) pushFuncError( ctxt, func_name, x )

	STFUN(test) {
		AYLOGDEBUG(rvec.size());
		AYLOGDEBUG(rvec[0].getBeadData().which());

		AYLOGDEBUG(result.isVec());
		return true;
	}
    STFUN(getEnt) {
        SETFUNCNAME(getEnt);
        if(rvec.size() )  {
            const BarzerEntityRangeCombo* erc  = getAtomicPtr<BarzerEntityRangeCombo>(rvec[0]);
            const BarzerEntity* ent  = ( erc ? &(erc->getEntity()) : getAtomicPtr<BarzerEntity>(rvec[0]) );
            if( ent ) {
                if( rvec.size() == 1 ) { /// this is a getter
                    setResult(result, *ent );
                    return true;
                } else if( rvec.size() == 2 ) { // this is a setter
                    const BarzerEntityRangeCombo* r_erc  = getAtomicPtr<BarzerEntityRangeCombo>(rvec[1]);
                    const BarzerEntity* r_ent  = ( r_erc ? &(r_erc->getEntity()) : getAtomicPtr<BarzerEntity>(rvec[1]) );

                    if( r_ent ) {
                        setResult(result, *r_ent );
                        return true;
                    }
                }
            }
        }

        FERROR( "expects ( (ERC|Entity) [,entity] ) to get/set entity for erc or bypass" );
        return true;
    }
    STFUN(getRange){
        SETFUNCNAME(getRange);
        if(rvec.size() )  {
            const BarzerEntityRangeCombo* erc  = getAtomicPtr<BarzerEntityRangeCombo>(rvec[0]);
            const BarzerRange* range  = ( erc ? &(erc->getRange()) : getAtomicPtr<BarzerRange>(rvec[0]) );
            if( range ) {
                if( rvec.size() == 1 ) { /// this is a getter
                    setResult(result, *range );
                    return true;
                } else if( rvec.size() == 2 ) { // this is a setter
                    const BarzerEntityRangeCombo* r_erc  = getAtomicPtr<BarzerEntityRangeCombo>(rvec[1]);
                    const BarzerRange* r_range  = ( r_erc ? &(r_erc->getRange()) : getAtomicPtr<BarzerRange>(rvec[1]) );

                    if( r_range ) {
                        setResult(result, *r_range );
                        return true;
                    }
                }
            }
        }
        FERROR("expects: (ERC|Range) [,(range|ERC)] to get/set range");
        return true;
    }
    STFUN(isRangeEmpty)
    {
        SETFUNCNAME(isRangeEmpty);
        if( rvec.size() == 1 ) {
            const BarzerRange* range  = getAtomicPtr<BarzerRange>(rvec[0]);

            if( !range ) {
                const BarzerEntityRangeCombo* erc  = getAtomicPtr<BarzerEntityRangeCombo>(rvec[0]);

                if( erc )
                    range = &(erc->getRange());
            }
            if( range ) {
                if( range->isEmpty())
                    result = rvec[0];
                else {
                    // BarzelBeadBlank blank;
                    result.setBeadData(BarzelBeadBlank());
                }
                return true;
            }
        }
        FERROR( "expects ( (ERC|Range) ) to tell whether the range is empty (lo == hi)" );

        return true;
    }
	STFUN(textToNum) {
        SETFUNCNAME(text2Num);
        int langId = 0; // language id . 0 means english
        ay::char_cp_vec tok;

        BarzerNumber bn;
		for (BarzelEvalResultVec::const_iterator ri = rvec.begin(); ri != rvec.end(); ++ri) {
            const BarzerLiteral* ltrl = getAtomicPtr<BarzerLiteral>( *ri ) ;
            if( ltrl )  {
                const char * str =  gpools.getStringPool().resolveId(ltrl->getId());
                if( str ) {
                    tok.push_back( str );
                } else {
                    std::stringstream strstr;
                    strstr << "invalid string in argument #" << (ri-rvec.begin());
                    FERROR( strstr.str().c_str() );
                }
            } else {
		        std::stringstream strstr;
                strstr << "expected a literal in argument #" << (ri-rvec.begin());
                FERROR( strstr.str().c_str() );
            }
        }
        int t2ierr = ( strutil::text_to_number( bn, tok, langId ) > strutil::T2NERR_JUNK );
        if( t2ierr ) {
            std::stringstream strstr;
            strstr << "text to number error #" << t2ierr ;
            FERROR( strstr.str().c_str() );
        }
		setResult(result, bn );
        return true;
    }
	// makers
	STFUN(mkDate) //(d) | (d,m) | (d,m,y)
	{
        SETFUNCNAME(mkDate);

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

            default: // size > 3
                AYLOG(ERROR) << "mkDate(): Expected max 3 arguments, "
                             << rvec.size() << " given";
                y = getNumber(rvec[2]);
                m = getNumber(rvec[1]);
                d = getNumber(rvec[0]);
                break;
            }
            date.setDayMonthYear(d,m,y);

            //date.print(AYLOG(DEBUG) << "date formed: ");
            //setResult(result, date);
            return true;
		} catch (boost::bad_get) {
            FERROR( "Wrong argument type"  );

            date.setDayMonthYear(d,m,y);
            return true;
		}
		return false;
	}

	STFUN(mkDateRange)
	{
		// SETSIG(mkDateRange(Date, Number, [Number, [Number]]));
        SETFUNCNAME(mkDateRange);

		int day = 0, month = 0, year = 0;
		// Holy shit ... what is this stuf?!??!  (AY)
		try {
			switch(rvec.size()) {
			case 4: year = getAtomic<BarzerNumber>(rvec[3]).getInt();
			case 3: month = getAtomic<BarzerNumber>(rvec[2]).getInt();
			case 2: {
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
                FERROR( "Need 2-4 arguments");
				// AYLOG(ERROR) << sig << ": Need 2-4 arguments";
			}
		} catch (boost::bad_get) {
            FERROR( "Wrong argument type");
			// AYLOG(ERROR) << sig << ": Wrong argument type";
		}
		return false;
	}

	STFUN(mkDay) {
		// SETSIG(mkDay(Number));
        SETFUNCNAME(mkDay);

		if (!rvec.size()) {
            FERROR("Need an argument");
			return false;
		}
        BarzerDate_calc calc;
        calc.setToday();
		try {
			calc.dayOffset(getNumber(rvec[0]).getInt());
			setResult(result, calc.d_date);
		} catch(boost::bad_get) {
            FERROR("Wrong argument type");
			setResult(result, calc.d_date);
		}
		return true;
	}

	STFUN(mkWday)
	{
        SETFUNCNAME(mkWday);

		if (rvec.size() < 2) {
            FERROR("Need 2 arguments");
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
            FERROR("Wrong arg type");
		}
		return false;
	}

    STFUN(mkMonth)
    {
        SETFUNCNAME(mkMonth);
        if (rvec.size() < 2) {
            FERROR("Wrong arg type");
            return false;
        }
        try {
            int fut  = getAtomic<BarzerNumber>(rvec[0]).getInt();
            uint8_t month = getAtomic<BarzerNumber>(rvec[1]).getInt();
            BarzerDate_calc calc(fut);
            calc.setToday();
            calc.setMonth(month);
            setResult(result, calc.d_date);
            return true;
        } catch (boost::bad_get) {
            FERROR("Wrong argument type");
        }
        return false;
    }



	STFUN(mkTime)
	{
        SETFUNCNAME(mkTime);
		BarzerTimeOfDay &time = setResult(result, BarzerTimeOfDay());
		try {
			int hh(0), mm(0), ss(0);
			switch (rvec.size()) {
			case 3: {
			    const BarzelBeadData &bd = rvec[2].getBeadData();
			    if (bd.which()) ss = getNumber(bd).getInt();
			}
			case 2: {
			    const BarzelBeadData &bd = rvec[1].getBeadData();
			    if (bd.which()) mm = getNumber(bd).getInt();
			}
			case 1: {
			    const BarzelBeadData &bd = rvec[0].getBeadData();
			    if (bd.which()) hh = getNumber(bd).getInt();
				break;
			}
			case 0: {
				std::time_t t = std::time(0);
				std::tm *tm = std::localtime(&t);
				hh = tm->tm_hour;
				mm = tm->tm_min;
				ss = tm->tm_sec;
			}
            // note no break here anymore
			default:
                if( rvec.size() > 3 ) {
                    FERROR("Expected max 3 arguments");
                }
			}
			time.setHHMMSS( hh, mm, ss );
			return true;
		} catch (boost::bad_get) {
            FERROR("Wrong argument type");
		}
		return false;
	}

	// applies  BarzerDate/BarzerTimeOfDay/BarzerDateTime to BarzerDateTime
	// to construct a timestamp
	struct DateTimePacker : public boost::static_visitor<bool> {
		BarzerDateTime &dtim;
        BarzelEvalContext& d_ctxt;
        const char* d_funcName;
		DateTimePacker(BarzerDateTime &d,BarzelEvalContext& ctxt,const char* funcName):
            dtim(d),d_ctxt(ctxt),d_funcName(funcName) {}

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
			return boost::apply_visitor(*this, data.getData());
		}
		// not applicable
		template<class T> bool operator()(const T&)
		{
            pushFuncError(d_ctxt,d_funcName, "Wrong argument type" );
			return false;
		}

	};

	STFUN(mkDateTime) {
		SETFUNCNAME(mkDateTime);
		BarzerDateTime dtim;
		DateTimePacker v(dtim,ctxt,func_name);

		for (BarzelEvalResultVec::const_iterator ri = rvec.begin();
				ri != rvec.end(); ++ri) {
			if (!boost::apply_visitor(v, ri->getBeadData())) {
				FERROR("fail");
				return false;
			}
		}
		setResult(result, dtim);
		return true;
	}

	STFUN(mkEnityList) {
		SETFUNCNAME(mkDateTime);
        FERROR("Unimplemented");
		return false;
	}
	// tries to pack 2 values into 1 range
	// always assumes the values are atomic and of the same type
	// the only supported types at the moments are
	// BarzerNumber, BarzerDate and BarzerTimeOfDay

	struct RangePacker : public boost::static_visitor<bool> {
		GlobalPools &globPools;
		BarzerRange &range;

		uint32_t cnt;

        const char* d_funcName;
        BarzelEvalContext& d_ctxt;


		RangePacker(GlobalPools &u, BarzerRange &r, BarzelEvalContext& ctxt, const char* funcName) :
            globPools(u), range(r), cnt(0) ,
            d_funcName(funcName),
            d_ctxt(ctxt)
        {}

		bool operator()(const BarzerLiteral &ltrl) {
			ay::UniqueCharPool &spool = globPools.stringPool;
			if (ltrl.getId() == spool.internIt("DESC")) {
				range.setDesc();
				return true;
			} else if (ltrl.getId() == spool.internIt("ASC")) {
				return true;
			} else {
                pushFuncError( d_ctxt, d_funcName, "Unknown literal" );
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
			if (cnt) {
				if (rnum.isInt()) {
					if (range.getType() == BarzerRange::Integer_TYPE) {
						BarzerRange::Integer &ip
							= boost::get<BarzerRange::Integer>(range.getData());
						setSecond(ip, rnum.getInt());
					} else if (range.getType() == BarzerRange::Real_TYPE) {
						BarzerRange::Real &rp
							= boost::get<BarzerRange::Real>(range.getData());
						setSecond(rp, (double) rnum.getInt());
					}
				} else if (rnum.isReal()) {
					if (range.getType() == BarzerRange::Real_TYPE) {
						BarzerRange::Real &rp
							= boost::get<BarzerRange::Real>(range.getData());
						setSecond(rp, rnum.getReal());
					} else {
						int lf = boost::get<BarzerRange::Integer>(range.getData()).first;
						BarzerRange::Real newRange((float) lf, (float) lf);
						setSecond(newRange, rnum.getReal());
						range.setData(newRange);
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
			if (cnt) {
				try {
					BarzerRange::Date &dp
						= boost::get<BarzerRange::Date>(range.getData());
					setSecond(dp, rdate);
				} catch (boost::bad_get) {
                    pushFuncError( d_ctxt, d_funcName, "Types don't match" );
					// AYLOG(ERROR) << "Types don't match: " << range.getData().which();
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
                    pushFuncError( d_ctxt, d_funcName, "Types don't match" );
					return false;
				}
			} else {
				range.setData(BarzerRange::DateTime(dt, dt));
			}
			return true;
		}

		bool operator()(const BarzerTimeOfDay &rtod) {
			if (cnt) {
				try {
					BarzerRange::TimeOfDay &todp
						= boost::get<BarzerRange::TimeOfDay>(range.getData());
					todp.second = rtod;
				} catch (boost::bad_get) {
					// AYLOG(ERROR) << "Types don't match";
                    pushFuncError( d_ctxt, d_funcName, "Types don't match" );
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

		bool operator()(const BarzerEntityRangeCombo &combo) {
			return operator()(combo.getRange());
		}

		template<class T> bool operator()(const T&) {
            pushFuncError( d_ctxt, d_funcName, "Wrong range type" );
			return false;
		}

	};

	STFUN(mkRange)
	{
        SETFUNCNAME(mkRange);
		BarzerRange br;
		RangePacker rp(gpools, br,ctxt,func_name);

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
		uint32_t cl, scl;
		//const BELFunctionStorage_holder &holder;
        const char* d_funcName;
		const StoredUniverse &universe;
        BarzelEvalContext& d_ctxt;

		//EntityPacker(const BELFunctionStorage_holder &h)
		EntityPacker(const StoredUniverse &u, BarzelEvalContext& ctxt, const char* funcName  )
			: cnt(0), tokId(INVALID_STORED_ID), cl(0), scl(0), d_funcName(funcName), universe(u), d_ctxt(ctxt)
        {}

		bool operator()(const BarzerLiteral &ltrl) {
			return (tokId = getTokId(ltrl, universe));
		}
		bool operator()(const BarzerNumber &rnum) {
			uint32_t ui = (uint32_t) rnum.getInt();
			if(cnt++) {
				scl = ui;
			} else {
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
            pushFuncError( d_ctxt, d_funcName, "Wrong range type" );
			return false;
		}
	};

	STFUN(mkEnt) // makes Entity
	{
        SETFUNCNAME(mkEnt);
		EntityPacker ep(q_universe,ctxt,func_name);

		for (BarzelEvalResultVec::const_iterator ri = rvec.begin();
								ri != rvec.end(); ++ri)
			if (!boost::apply_visitor(ep, ri->getBeadData())) return false;

		const StoredEntityUniqId euid(ep.tokId, ep.cl, ep.scl);
		setResult(result, euid);
		return true;
	}

	// tries to construct an EntityRangeCombo from a sequence of BeadData
	// first BarzerEntity counts as entity the rest - as units
	// can also merge with another ERC
	struct ERCPacker : public boost::static_visitor<bool> {
		uint8_t num;
        const char* d_funcName;
        BarzelEvalContext& d_ctxt;

		BarzerEntityRangeCombo &erc;
		ERCPacker(BarzerEntityRangeCombo &e, BarzelEvalContext& ctxt, const char* funcName ) :
            num(0), d_funcName(funcName), d_ctxt(ctxt), erc(e) {}

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
            pushFuncError(d_ctxt, d_funcName, "Wrong argument type in ERCPacker" );
			return false;
		}

	};


	STFUN(mkERC) // makes EntityRangeCombo
	{
		//AYLOGDEBUG(rvec.size());
		//AYLOG(DEBUG) << rvec[0].getBeadData().which();
        SETFUNCNAME(mkERC);

		BarzerEntityRangeCombo erc;
		ERCPacker v(erc,ctxt,func_name);
		for (BarzelEvalResultVec::const_iterator ri = rvec.begin();
						ri != rvec.end(); ++ri)
			if (!boost::apply_visitor(v, ri->getBeadData())) {
                    FERROR("ERC failed");
                    return false;
			}

		setResult(result, erc);
		return true;
	}

	struct ErcExprPacker : public boost::static_visitor<bool> {
		BarzerERCExpr expr;
		GlobalPools &gpools;
        const char* d_funcName;
        BarzelEvalContext& d_ctxt;
		ErcExprPacker(GlobalPools &gp, BarzelEvalContext& ctxt, const char* funcName ) :
            gpools(gp), d_funcName(funcName), d_ctxt(ctxt)
        {}


		bool operator()(const BarzerLiteral &ltrl) {
			ay::UniqueCharPool &sp = gpools.stringPool;
			if (ltrl.getId() == sp.internIt("and")) {
				//AYLOG(DEBUG) << "AND";
				expr.setType(BarzerERCExpr::T_LOGIC_AND);
			} else if (ltrl.getId() == sp.internIt("or")) {
				//AYLOG(DEBUG) << "OR";
				expr.setType(BarzerERCExpr::T_LOGIC_OR);
			} else {

                std::stringstream sstr;
				sstr << "mkErcExpr(Literal,ERC,ERC ...):"
							 << "Unknown literal: " << sp.resolveId(ltrl.getId());

                pushFuncError(d_ctxt, d_funcName, sstr.str().c_str() );
				// AYLOG(ERROR) << "mkErcExpr(Literal,ERC,ERC ...):"
							 // << "Unknown literal: " << sp.resolveId(ltrl.getId());
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
            pushFuncError(d_ctxt, d_funcName, "Type mismatch" );
			// AYLOG(ERROR) << "mkErcExpr(Literal,ERC,ERC ...): Type mismatch";
			return false;
		}
	};

	STFUN(mkErcExpr)
	{
        SETFUNCNAME(mkErcExpr);
		if (rvec.size() < 3) {
            FERROR("needs at least 3 arguments");
			return false;
		}

		ErcExprPacker p(gpools,ctxt,func_name);

		for(BarzelEvalResultVec::const_iterator it = rvec.begin();
											    it != rvec.end(); ++it) {
			if (!boost::apply_visitor(p, it->getBeadData())) return false;
		}
		setResult(result, p.expr);
		return true;
	}

	STFUN(mkLtrl)
	{
        SETFUNCNAME(mkErcExpr);
		if (!rvec.size()) {
            FERROR("Argument is required");
			return false;
		}

		const char* str = extractString(gpools, rvec[0]);
		if (!str) {
			// AYLOG(ERROR) << "Need a string";
            FERROR("Need a string");
			return false;
		}

		uint32_t id = gpools.stringPool.internIt(str);
		BarzerLiteral lt;
		lt.setString(id);

		setResult(result, lt);
		return true;
	}

	// set "stop" type on the incoming literal.
	STFUN(mkFluff)
	{
        SETFUNCNAME(mkFluff);
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
		} catch (boost::bad_get&) {
			// AYLOG(ERROR) << "mkFluff(BarzerLiteral): Wrong argument type";
            FERROR("Wrong argument type");
		}
	    return true;
	}

	struct ExprTagPacker : public boost::static_visitor<bool> {
        const char* d_funcName;
		BarzelBeadExpression &expr;
        BarzelEvalContext& d_ctxt;

		ExprTagPacker(BarzelBeadExpression &e,BarzelEvalContext& ctxt, const char* funcName ) : d_funcName(funcName), expr(e), d_ctxt(ctxt) {}

		bool operator()(const BarzelBeadBlank&) {
			// AYLOG(ERROR) << "BarzelBeadBlank encountered";
            pushFuncError(d_ctxt, d_funcName, "Blank encountered" );
			return false;
		}

		template <class T> bool operator()(const T &data) {
			expr.addChild(data);
			return true;
		}
	};

	STFUN(mkExprTag) {
        SETFUNCNAME(mkExprTag);
		if (rvec.size()) {
			try {
				// to avoid too much copying
				result.setBeadData(BarzelBeadExpression());
				BarzelBeadExpression &expr
					= boost::get<BarzelBeadExpression>(result.getBeadData());
				ExprTagPacker etp(expr,ctxt,func_name);

				expr.setSid(getAtomic<BarzerLiteral>(rvec[0]).getId());
				for (BarzelEvalResultVec::const_iterator it = rvec.begin()+1;
														 it != rvec.end();
														 ++it) {
					if(!boost::apply_visitor(etp, it->getBeadData())) return false;
				}
				return true;
			} catch (boost::bad_get) {
                FERROR("Type Mismatch");
			}
		}
		return false;
	}

	#define GETID(x) getAtomic<BarzerLiteral>(x).getId()
	STFUN(mkExprAttrs) {
        SETFUNCNAME(mkExprAttrs);
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
				FERROR("Type mismatch");
                return false;
			}
		}
        FERROR("Blank");
		return false;
	}
	#undef GETID

	// getters

	STFUN(getWeekday) {
        SETFUNCNAME(getWeekday);
		BarzerDate bd;
		try {
            if (rvec.size())
                bd = getAtomic<BarzerDate>(rvec[0]);
            else bd.setToday();

            BarzerNumber n(bd.getWeekday());
            setResult(result, n);
            return true;
		} catch (boost::bad_get) {
            FERROR("wrong argument type, date expected");
		}
		return false;
	}

	STFUN(getMDay) {
        SETFUNCNAME(getMDay);
		BarzerDate bd;
		try {
            if (rvec.size())
                bd = getAtomic<BarzerDate>(rvec[0]);
            else bd.setToday();
            setResult(result, BarzerNumber(bd.day));
            return true;
		} catch (boost::bad_get) {
            FERROR("wrong argument type");
        }
		return false;
	}

    STFUN(setMDay) {
        SETFUNCNAME(setMDay);
        BarzerDate bd;
        BarzerNumber n;
        try {
            switch (rvec.size()) {
            case 2: n = getAtomic<BarzerNumber>(rvec[1]);
            case 1: bd = getAtomic<BarzerDate>(rvec[0]); break;
            case 0:
                bd.setToday();
                n.set(1);
            }
            bd.setDay(n);
            setResult(result, bd);
            return true;
        } catch (boost::bad_get) {
            FERROR("wrong argument type");
            setResult(result, bd);
        }
        return true;
    }

	STFUN(getYear) {
        SETFUNCNAME(getYear);
		BarzerDate bd;
		try {
            if (rvec.size())
                bd = getAtomic<BarzerDate>(rvec[0]);
            else bd.setToday();
            setResult(result, BarzerNumber(bd.year));
            return true;
		} catch (boost::bad_get) {
            FERROR("wrong argument type");
            setResult(result,BarzerNumber(0));
            return true;
        }
	}

	struct TokIdGetter : boost::static_visitor<bool> {
		const StoredUniverse &universe;
		uint32_t tokId;
        const char* d_funcName;
        BarzelEvalContext& d_ctxt;

		TokIdGetter(const StoredUniverse &u, BarzelEvalContext& ctxt, const char* funcName) :
            universe(u), tokId(0) , d_funcName(funcName), d_ctxt(ctxt)
        {}

		bool operator()(const BarzerLiteral &dt) {
			return ((tokId = getTokId(dt, universe)));
		}

		bool operator()(const BarzelBeadAtomic &data) {
			return boost::apply_visitor(*this, data.getData());
		}
		// not applicable
		template<class T> bool operator()(const T&) {
            pushFuncError(d_ctxt, d_funcName, "Type mismatch" );

			return false;
		}
	};

	STFUN(entId)
    {
        SETFUNCNAME(entClass);

        uint32_t stringId = 0xffffffff;
        if( rvec.size() ) {
            const BarzerEntity* ent  = getAtomicPtr<BarzerEntity>(rvec[0]);
            if( !ent ) {
                FERROR("entity expected");
            } else {
	            const StoredToken *tok = q_universe.getDtaIdx().getStoredTokenPtrById(ent->tokId);
                if( tok )
                    stringId = tok->stringId;
            }
        }

        setResult(result, BarzerLiteral(stringId) );
        return true;
    }
	STFUN(entClass)
    {
        SETFUNCNAME(entClass);
        uint32_t entClass = 0;
        if( !rvec.size() ) {
            entClass = q_universe.getEntClass() ;
        } else {
            const BarzerEntity* ent  = getAtomicPtr<BarzerEntity>(rvec[0]);
            if( !ent ) {
                FERROR("entity expected");
            } else
                entClass = ent->eclass.ec;
        }

        setResult(result, BarzerNumber(entClass) );
        return true;
    }
	STFUN(entSetSubclass)
    {
        // SETFUNCNAME(entSetSubclass);
        if( rvec.size() < 2 ) {
            return true;
        }
        const BarzerEntity* ent_begin = 0, *ent_end = 0;
        const BarzerEntity* ent  = getAtomicPtr<BarzerEntity>(rvec[0]);
        size_t numEntities = 0;
        if( ent ) {
            ent_begin = ent;
            ent_end   = ent_begin + 1;
            numEntities = 1;
        } else { /// lests try for entity list
            const BarzerEntityList* entList  = getAtomicPtr<BarzerEntityList>(rvec[0]);
            ent_begin = &(entList->getList()[0]);
            numEntities = entList->getList().size();
            ent_end = ent_begin + numEntities;

        }
        if( !numEntities )
            return true;

        if( rvec.size() == 2 ) { /// ent, newSubclass
            const BarzerNumber* num = getAtomicPtr<BarzerNumber>(rvec[1]);
            uint32_t newSubclass = (num? num->getInt():0);
            if( numEntities == 1 ) {
                setResult(result,
                    BarzerEntity(
                        ent_begin->tokId,
                        ent_begin->eclass.ec,
                        newSubclass
                    )
                );
            } else {
                BarzerEntityList& newEntList = setResult(result, BarzerEntityList() );
                for( const BarzerEntity* x = ent_begin; x!= ent_end; ++x ) {
                    newEntList.addEntity( BarzerEntity( x->tokId, x->eclass.ec, newSubclass ) );
                }
            }
        } else { ///  ent , oldSubclass, newSubclass, oldSubclass, newSubclass ...
            std::map< uint32_t, uint32_t > subclassTranslateMap;
            size_t i = 1;
            size_t rvec_sz_1 = rvec.size()-1;
            for( ; i< rvec_sz_1; i+=2 ) {
                const BarzerNumber* fromNum = getAtomicPtr<BarzerNumber>(rvec[i]);
                const BarzerNumber* toNum = getAtomicPtr<BarzerNumber>(rvec[i+1]);
                uint32_t fromSubclass = ( fromNum ? fromNum->getInt() : 0 ),
                    toSubclass = ( toNum ? toNum->getInt() : 0 );
                if( fromSubclass && toSubclass )
                    subclassTranslateMap[ fromSubclass ] = toSubclass;
            }

            if( numEntities == 1 ) {
                std::map< uint32_t, uint32_t >::const_iterator tmi = subclassTranslateMap.find( ent_begin->eclass.subclass );
                uint32_t newSubclass = ( tmi == subclassTranslateMap.end() ? 0 : tmi->second );
                setResult(result,
                    BarzerEntity(
                        ent_begin->tokId,
                        ent_begin->eclass.ec,
                        newSubclass
                    )
                );
            } else {
                BarzerEntityList& newEntList = setResult(result, BarzerEntityList() );
                for( const BarzerEntity* x = ent_begin; x!= ent_end; ++x ) {
                    std::map< uint32_t, uint32_t >::const_iterator tmi = subclassTranslateMap.find( x->eclass.subclass );
                    uint32_t newSubclass = ( tmi == subclassTranslateMap.end() ? 0 : tmi->second );
                    newEntList.addEntity( BarzerEntity( x->tokId, x->eclass.ec, newSubclass ) );
                }
            }
        }
        return true;
    }
	STFUN(entSubclass)
    {
        SETFUNCNAME(entSubclass);
        if( !rvec.size() ) {
            FERROR( "entity missing" );
            return false;
        }
        const BarzerEntity* ent  = getAtomicPtr<BarzerEntity>(rvec[0]);
        if( ent ) {
            setResult(result, BarzerNumber(ent->eclass.subclass));
        } else {
            FERROR("wrong argument: entity expected");
            setResult(result, BarzerNumber() );
        }

        return true;
    }

	STFUN(getTokId)
	{
        SETFUNCNAME(getTokId);
		if (!rvec.size()) {
            FERROR("needs an argument");
			return false;
		}
		TokIdGetter tig(q_universe,ctxt,func_name);
		boost::apply_visitor(tig, rvec[0].getBeadData());
		setResult(result, BarzerNumber((int)tig.tokId));
		return true;
	}

	struct RangeGetter : public boost::static_visitor<bool> {
		BarzelEvalResult &result;
		uint8_t pos;
        const char* d_funcName;
        BarzelEvalContext& d_ctxt;

		RangeGetter(BarzelEvalResult &r, uint8_t p, BarzelEvalContext& ctxt, const char* funcName ) :
            result(r), pos(p), d_funcName(funcName),d_ctxt(ctxt)
        {}


		bool set(const BarzerNone) { return false; }
		bool set(int i) {
			setResult(result, BarzerNumber(i));
			return true;
		}
		bool set(int64_t i) {
			setResult(result, BarzerNumber(i));
			return true;
		}
		bool set(float f) {
			setResult(result, BarzerNumber(f));
			return true;
		}
		bool set(double f) {
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
		bool operator()(const BarzerEntityRangeCombo &erc)
			{ return operator()(erc.getRange()); }
		bool operator()(const BarzelBeadAtomic &data)
			{ return boost::apply_visitor(*this, data.getData()); }
		// not applicable
		template<class T> bool operator()(const T&) {
            pushFuncError(d_ctxt, d_funcName, "Type mismatch" );
			return false;
		}

	};

	STFUN(getLow)
	{
		// SETSIG(getLow(BarzerRange));
        SETFUNCNAME(getLow);

		RangeGetter rg(result, 0,ctxt,func_name);
		if (rvec.size()) {
			return boost::apply_visitor(rg, rvec[0].getBeadData());
		} else {
			FERROR("Expected at least 1 argument");
		}
		return false;
	}

	STFUN(getHigh)
	{
		// SETSIG(getLow(BarzerRange));
        SETFUNCNAME(getHigh);

		RangeGetter rg(result, 1,ctxt,func_name);
		if (rvec.size()) {
			return boost::apply_visitor(rg, rvec[0].getBeadData());
		} else {
			FERROR("Expected at least 1 argument");
		}
		return false;
	}

	// arith

	STFUN(opPlus)
	{
        // SETFUNCNAME(opPlus);
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
        SETFUNCNAME(opMinus);
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
            FERROR("Wrong argument types");
		}
		//setResult(result, bn); // most likely NaN if something went wrong
		return false;
	}

	STFUN(opMult)
	{
        // SETFUNCNAME(opMult);
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
        SETFUNCNAME(opDiv);
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
            FERROR("Wrong argument types");
		}
		//setResult(result, bn); // most likely NaN if something went wrong
		return false;

	}


	// logic
	// no implementation yet for i'm not sure yet what to put into the result
	// meaning there's no boolean type in barzer (yet?)

	STFUN(opAnd) // stfunc_opAnd(&result, &rvec)
	{
        // SETFUNCNAME(opAnd);
		if (rvec.size()) {
			BarzelEvalResultVec::const_iterator ri = rvec.begin();
			do {
				if ((ri++)->getBeadData().which() == BarzelBeadBlank_TYPE)	break;
			} while (ri != rvec.end());
			result.setBeadData((--ri)->getBeadData());
		} else {
			BarzelBeadBlank blank;
			result.setBeadData(blank);
		}
		return true;
	}

	STFUN(opOr)
	{
        // SETFUNCNAME(opOr);
		if (rvec.size()) {
			BarzelEvalResultVec::const_iterator ri = rvec.begin();
			do {
				if ((ri++)->getBeadData().which() != BarzelBeadBlank_TYPE) break;
			} while (ri != rvec.end());
			result.setBeadData((--ri)->getBeadData());
		} else {
			BarzelBeadBlank blank;
			result.setBeadData(blank);
		}
		return true;
	}

	STFUN(opSelect) {
        SETFUNCNAME(opOr);

		if (rvec.size() < 3) {
            FERROR("need at least 3 arguments");
			return false;
		}

		if (!(rvec.size() & 1)) {
			FERROR("opSelect: need an odd amount of arguments");
		}

		const BarzelBeadData &var = rvec[0].getBeadData();

		for (BarzelEvalResultVec::const_iterator it = rvec.begin()+1;
												 it != rvec.end(); it += 2) {
			const BarzelBeadData &rec = it->getBeadData();

			if (beadsEqual(var, rec)) {
				result.setBeadData((it+1)->getBeadData());
				return true;
			}
		}
		return false;
	}

	// should make this more generic
	template<class T> bool cmpr(
             BarzelEvalResult &result,
       const BarzelEvalResultVec &rvec,
       BarzelEvalContext& ctxt,
       const char* func_name ) const
	{
	    T op;
	    try {
	        if (rvec.size() >= 2) {
                BarzelEvalResultVec::const_iterator end = rvec.end(),
                                                    it = rvec.begin()+1;
                bool cont = true;
                for (; cont && it != end; ++it) {
                    const BarzerNumber &l = getNumber((it-1)->getBeadData()),
                                       &r = getNumber(it->getBeadData());

                    if (l.isNan() || r.isNan()) {
                        cont = false;
                    } else if (l.isReal()) {
                        if (r.isReal()) {
                            cont = op(l.getReal(), r.getReal());
                        } else {
                            cont = op(l.getReal(), (double) r.getInt());
                        }
                    } else {
                        if (r.isReal()) {
                            cont = op((double)l.getInt(), r.getReal());
                        } else {
                            cont = op(l.getInt(), r.getInt());
                        }
                    }
                }
                setResult(result, cont);
	        } else {
                FERROR("Need at least 2 arguments");
                setResult(result, false);
	        }
	    } catch (boost::bad_get) {
            FERROR("Trying to compare non-numerics");
            setResult(result, false);
        }
	    return true;
	}

	STFUN(opLt) { SETFUNCNAME(opLt); return cmpr<lt>(result, rvec,ctxt,func_name); }
	STFUN(opGt)	{ SETFUNCNAME(opGt); return cmpr<gt>(result, rvec,ctxt,func_name); }

	STFUN(opEq)
	{
        SETFUNCNAME(opEq);
		if (rvec.size() >= 2) {
		    BarzelEvalResultVec::const_iterator end = rvec.end(),
		                                        it = rvec.begin()+1;
		    for (; it != end; ++it) {
		        if (!beadsEqual((it-1)->getBeadData(), it->getBeadData())) {
		            setResult(result, false);
		            return true;
		        }
		    }
		    setResult(result, true);
		} else {
            FERROR("need at least 2 arguments");
		    setResult(result, true);
		}
		return true;
	}

	STFUN(opDateCalc)
	{
        SETFUNCNAME(opDateCalc);
		// const char *sig = "opDateCalc(Date ,Number[, Number[, Number]]):";
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
			default:
                FERROR("Need 2-4 arguments" );
			}
		} catch (boost::bad_get) {
            FERROR("Type mismatch");
		}
		return false;
	}

    // concatenates all parameters as one list
	STFUN(listCat) { //
        BarzelBeadDataVec& resultVec = result.getBeadDataVec();
        for( BarzelEvalResultVec::const_iterator i = rvec.begin(); i!= rvec.end(); ++i ) {
            const BarzelBeadDataVec& v = i->getBeadDataVec();
            for( BarzelBeadDataVec::const_iterator vi = v.begin(); vi !=v.end(); ++vi )
                resultVec.push_back( *vi );
        }
        return true;
    }
	STFUN(setUnmatch) { //
        BarzelBeadDataVec& resultVec = result.getBeadDataVec();
        for( BarzelEvalResultVec::const_iterator i = rvec.begin(); i!= rvec.end(); ++i ) {
            const BarzelBeadDataVec& v = i->getBeadDataVec();
            for( BarzelBeadDataVec::const_iterator vi = v.begin(); vi !=v.end(); ++vi )
                resultVec.push_back( *vi );
        }
        result.setUnmatchability(1);
        return true;
    }
    // typeFilter( sample, arg1, arg2, ..., argN )
    // filters out everything from arg1-N of the same type as the sample
	STFUN(typeFilter) { //
        SETFUNCNAME(typeFilter);
        if( rvec.size() <2 ) {
            FERROR("need at least 2 arguments. Usage (sample, a1,...,aN)");
            return false;
        }
        const BarzelBeadData& sample = rvec[0].getBeadData();
        BarzelBeadDataVec& resultVec = result.getBeadDataVec();

        BarzelEvalResultVec::const_iterator i = rvec.begin();
        for( ++i ; i!= rvec.end(); ++i ) {
            const BarzelBeadDataVec& v = i->getBeadDataVec();
            for( BarzelBeadDataVec::const_iterator vi = v.begin(); vi !=v.end(); ++vi )
                if( vi->which() == sample.which() )
                    resultVec.push_back( *vi );
        }
        return true;
    }
    /// arrIndex( variable, Index )
	STFUN(arrIndex) { //
        SETFUNCNAME(arrIndex);
        if( rvec.size() < 2 ) {
            FERROR( "must have at least 2 arguments: variable, index" );
            return false;
        }
        const BarzelBeadDataVec& arrVec = rvec[0].getBeadDataVec();

        int idx = 0;
        const BarzerNumber* num  = getAtomicPtr<BarzerNumber>(rvec[1]);
        if( !num ) {
            FERROR( "second argument should be a positive number" );
        } else
            idx = num->getInt();

        if( idx < 0 )
            FERROR("index must be positive" );
        else if( (size_t)idx > arrVec.size() )
            FERROR("index out of range" );

        result.getBeadDataVec().push_back( arrVec[idx] );
        return true;
    }
    // array size
    STFUN(arrSz) {
        BarzerNumber bn;
        setResult(result, BarzerNumber(
            (int)( rvec.size() ? rvec[0].getBeadDataVec().size() : 0 )
        ));
        return true;
    }
	// string
	STFUN(strConcat) { // strfun_strConcat(&result, &rvec)
        SETFUNCNAME(strConcat);
		if (rvec.size()) {
			StrConcatVisitor scv(gpools,ctxt,func_name);
			for (BarzelEvalResultVec::const_iterator ri = rvec.begin();
													 ri != rvec.end(); ++ri) {

                const BarzelBeadDataVec& v = ri->getBeadDataVec();
                for( BarzelBeadDataVec::const_iterator vi = v.begin(); vi !=v.end(); ++vi ) {
				    if (!boost::apply_visitor(scv, (*vi))) {
                        // ignoring errors, continuing concatenation
                        // we can theoretically print error messages
                    }
                }
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
		{ return ent.eclass.ec == (uint32_t)cl; }
	inline static bool checkSC(const BarzerEntity& ent, uint32_t cl, uint32_t scl, uint32_t id = 0)
		{ return ent.eclass.subclass == (uint32_t)scl && checkClass(ent, cl); }
	inline static bool checkId(const BarzerEntity& ent, uint32_t cl, uint32_t scl, uint32_t id)
		{ return ent.tokId == id && checkSC(ent, cl, scl); }

    /// mkEntList( entities and entity lists in any order )
	STFUN(mkEntList) // (BarzerEntityList, EntityList, BarzerNumber[, BarzerNumber[, BarzerNumber]])
    {
        SETFUNCNAME(mkEntList);
        BarzerEntityList outlst;

        bool outlistClassSet = false;
        for( BarzelEvalResultVec::const_iterator i = rvec.begin(); i != rvec.end(); ++i ) {
            const BarzerEntity* ent  = getAtomicPtr<BarzerEntity>(*i);
            if( !ent ) { // there's a non-entity in our list
                const BarzerEntityList* entLst = getAtomicPtr<BarzerEntityList>(*i);
                if( entLst ) {
                    const BarzerEntityList::EList& theEList = entLst->getList();
                    for( BarzerEntityList::EList::const_iterator ei = theEList.begin(); ei != theEList.end(); ++ei ) {
                        outlst.addEntity(*ei);
                        if( !outlistClassSet ) {
                            outlistClassSet = true;
                            outlst.setClass( ei->getClass() );
                        }
                    }
                } else {
                    std::stringstream sstr;
                    sstr << "Parameter #" << ( i - rvec.begin() ) << " ignored - it's not an entity\n";
                    FERROR( sstr.str().c_str() );
                }
            } else {
                outlst.addEntity(*ent);
                if( !outlistClassSet ) {
                    outlistClassSet = true;
                    outlst.setClass( ent->getClass() );
                }
            }
        }
        setResult(result, outlst);
        return true;
    }
    /// list, {class,subclass, filterClass, filterSubclass}[N]
	STFUN(topicFilterEList) //
    {

        /// first parm is the list we're filtering
        /// followed by list of pairs of class/subclass of topics we want to filter on
        /// ACHTUNG: extract the list of filter topics
        /// filter on them
        /// if this list is empty NEVER filter on own class/subclass
        ///
        BarzerEntityList outlst;
        const BarzerEntityList* entLst =0;
        BarzerEntityList tmpList;
        if( rvec.size() )  {

            entLst = getAtomicPtr<BarzerEntityList>(rvec[0]);
            if( !entLst ) {
                const BarzerEntity* ent = getAtomicPtr<BarzerEntity>(rvec[0]);
                if( ent ) {
                    tmpList.addEntity( *ent );
                    entLst = &tmpList;
                }
            }
        }

        if( !entLst ) {
            setResult(result, outlst);
            return true;
        }
        // at this point entLst points to the list of all entities that may belong in the outlst
        // parsing filtering topic {class,subclass, filterClass,filterSubclass} pairs
        typedef std::vector< StoredEntityClass > StoredEntityClassVec;
        typedef std::map< StoredEntityClass, StoredEntityClassVec > SECFilterMap;
        SECFilterMap fltrMap;

        if( rvec.size() > 4 )  {
            size_t rvec_size_1 = rvec.size()-1;
            for( size_t i = 1; i< rvec_size_1; i+= 4 ) {
                StoredEntityClass sec;

                {
                const BarzerNumber* cn  = getAtomicPtr<BarzerNumber>(rvec[i]);
                if( cn )
                    sec.setClass( cn->getInt() );
                const BarzerNumber* scn  = getAtomicPtr<BarzerNumber>(rvec[i+1]);
                if( scn )
                    sec.setSubclass( scn->getInt() );
                }
                StoredEntityClass filterSec;
                {
                const BarzerNumber* cn  = getAtomicPtr<BarzerNumber>(rvec[i+2]);
                if( cn )
                    filterSec.setClass( cn->getInt() );
                const BarzerNumber* scn  = getAtomicPtr<BarzerNumber>(rvec[i+3]);
                if( scn )
                    filterSec.setSubclass( scn->getInt() );
                }
                StoredEntityClassVec& filterClassVec = fltrMap[ sec ];

                if( std::find( filterClassVec.begin(), filterClassVec.end(), filterSec ) == filterClassVec.end() )
                    fltrMap[ sec ].push_back( filterSec );
            }
        }
        /// here ecVec has all entity {c,sc} we're filtering on
        const BarzTopics::TopicMap& topicMap = ctxt.getBarz().topicInfo.getTopicMap();

        // computing the list of topics, currently in Barz, to filter on
        std::set< BarzerEntity > filterTopicSet;

        for( BarzTopics::TopicMap::const_iterator topI = topicMap.begin(); topI != topicMap.end(); ++topI ) {
            const BarzerEntity& topicEnt = topI->first;
            if( !fltrMap.size() )
                filterTopicSet.insert( topicEnt );
            else {
                for( SECFilterMap::const_iterator fi = fltrMap.begin(); fi != fltrMap.end(); ++fi ) {
                    if( std::find( fi->second.begin(), fi->second.end(), topicEnt.eclass ) !=
                        fi->second.end())
                    {
                        filterTopicSet.insert( topicEnt );
                    }
                }
            }
        }

        // here filterTopicSet contains every topic we need to filter on
        if( !filterTopicSet.size() ) { /// if nothing to filter on we're short circuitin
            setResult(result, *entLst );
            return true;
        }

        // pointers to all eligible entities will be in eligibleEntVec
        std::vector< const BarzerEntity* > eligibleEntVec;

        const BarzerEntityList::EList& theEList = entLst->getList();
        eligibleEntVec.reserve( theEList.size() );
        for( BarzerEntityList::EList::const_iterator ei = theEList.begin(); ei != theEList.end(); ++ei ) {
            eligibleEntVec.push_back( &(*ei) );
        }
        // pointers to all eligible entities ARE in eligibleEntVec and all topics to filter on are in filterTopicSet

        // loopin over all entities
        typedef std::pair< BarzerEntity, StoredEntityClass > EntFilteredByPair;
        typedef std::vector< EntFilteredByPair > FilteredEntityVec;
        FilteredEntityVec fltrEntVec;

        typedef std::pair< StoredEntityClass, StoredEntityClass > EntClassPair;
        std::vector< EntClassPair > matchesToRemove;  // very small vector - things to clear (en class, filtered by class) pairs

        for( std::vector< const BarzerEntity* >::iterator eei = eligibleEntVec.begin(); eei != eligibleEntVec.end(); ++eei ) {
            const BarzerEntity* eptr = *eei;
            if( !eptr )
                continue;
            /// looping over all topics
            bool entFilterApplies = false;
            bool entPassedFilter = false;

            StoredEntityClass  filterPassedOnTopic;
            SECFilterMap::iterator entFAi;

            for( std::set< BarzerEntity >::const_iterator fi = filterTopicSet.begin(); fi != filterTopicSet.end(); ++ fi ) {
                const BarzerEntity& topicEnt = *fi;
                const StoredEntityClass& topicEntClass = topicEnt.getClass();
                // trying to filter all still eligible entities eei - eligible entity iterator
                if( eptr->eclass != topicEntClass )  {  // topic applicable for filtering if its in a different class
                    // continuing to check filter applicability
                    entFAi=  fltrMap.find(eptr->eclass);
                    if( entFAi== fltrMap.end() )
                        continue;

                    StoredEntityClassVec::iterator truncIter = std::find( entFAi->second.begin(), entFAi->second.end(), topicEntClass );
                    if( truncIter  != entFAi->second.end() ) {
                        if( !entFilterApplies )
                            entFilterApplies = true;
                        const std::set< BarzerEntity >* topEntSet= q_universe.getTopicEntities( topicEnt );
                        if( topEntSet && topEntSet->find( *(eptr) ) != topEntSet->end() ) {
                            entPassedFilter = true;
                            filterPassedOnTopic  = topicEntClass;

                            if( entFAi->second.size() > 1 ) {
                                StoredEntityClassVec::iterator xx = truncIter;
                                ++xx;
                                if( xx != entFAi->second.end() ) {
                                    for( StoredEntityClassVec::const_iterator x = xx; x != entFAi->second.end(); ++x ) {
                                        EntClassPair m2remove( (*eei)->eclass, *x );
                                        if( std::find( matchesToRemove.begin(), matchesToRemove.end(), m2remove ) == matchesToRemove.end() )
                                            matchesToRemove.push_back( m2remove );
                                    }
                                    entFAi->second.erase( xx, entFAi->second.end() );
                                }
                            }
                        }
                    }
                }
            } // end of topic loop

            if( entFilterApplies ) {
                if( !entPassedFilter )
                    *eei= 0;
                else { // something was eligible for filtering and passed filtering
                    // we will see if there are any topics more junior than filterPassedOnTopic
                    fltrEntVec.push_back( EntFilteredByPair(*(*eei), filterPassedOnTopic) );
                }
            } else { // didnt have to filter
                fltrEntVec.push_back( EntFilteredByPair(*(*eei), StoredEntityClass()) );
            }
        } // end of entity loop

        /// here all non 0 pointers in eligibleEntVec can be copied to the outresult
        /// filled the vector with all eligible

        Barz& barz = ctxt.getBarz();
        bool strictMode = barz.topicInfo.isTopicFilterMode_strict();
        for(FilteredEntityVec::const_iterator i = fltrEntVec.begin(); i!= fltrEntVec.end(); ++i ) {
            if( strictMode && !i->second.isValid() )
                continue;
            bool shouldKeep = true;
            for( std::vector< EntClassPair >::const_iterator x = matchesToRemove.begin(); x!= matchesToRemove.end(); ++x ) {
                if( x->first == i->first.eclass && x->second == i->second ) {
                    /// this should be cleaned out
                    shouldKeep = false;
                    break;
                }
            }
            if( shouldKeep )
                outlst.addEntity( i->first );
        }
        if( !outlst.getList().size()  ) {
            for( BarzerEntityList::EList::const_iterator ei = theEList.begin(); ei != theEList.end(); ++ei ) {
                outlst.addEntity( *ei );
            }
        }

        setResult(result, outlst );
        return true;
    }

	STFUN(filterEList) // (BarzerEntityList, BarzerNumber[, BarzerNumber[, BarzerNumber]])
	{
        SETFUNCNAME(filterEList);
		//static const char *sig =
		//	"filterEList(BarzerEntityList, BarzerNumber, [BarzerNumber, [BarzerNumber]])";
		// SETSIG(filterEList(BarzerEntityList, BarzerNumber, [BarzerNumber, [BarzerNumber]]));

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
				FERROR( "Need at least 2 arguments" );
			}
		} catch (boost::bad_get) {
			FERROR( "Type mismatch" );
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



bool BELFunctionStorage::call(BarzelEvalContext& ctxt, const char *fname, BarzelEvalResult &er,
		                                   const BarzelEvalResultVec &ervec,
		                                   const StoredUniverse &u) const
{
	const uint32_t fid = globPools.stringPool.getId(fname);
	if (fid == ay::UniqueCharPool::ID_NOTFOUND) {
		std::stringstream strstr;
		strstr << "No such function name: `" << fname << "'";
        pushFuncError(ctxt, "", strstr.str().c_str() );
		return false;
	}
	return call(ctxt,fid, er, ervec, u);
}

bool BELFunctionStorage::call(BarzelEvalContext& ctxt, const uint32_t fid, BarzelEvalResult &er,
									              const BarzelEvalResultVec &ervec,
									              const StoredUniverse &u) const
{
	const BELStoredFunMap::const_iterator frec = holder->funmap.find(fid);
	if (frec == holder->funmap.end()) {
		std::stringstream strstr;
		const char *str = u.getGlobalPools().internalString_resolve(fid);
        strstr << "No such function: " << (str ? str : "<unknown>") << " (id: " << fid << ")";
        pushFuncError(ctxt, "", strstr.str().c_str() );
		return false;
	}
	return frec->second(holder, er, ervec, u, ctxt);

}

}




