/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
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
#include <boost/format.hpp>
#include <barzer_el_matcher.h>

namespace barzer {

struct BTND_Rewrite_Function;
namespace {
    uint32_t   getInternalStringIdFromLiteral( const GlobalPools& g, const BarzerLiteral* ltrl ) {
        if( !ltrl ) 
            return 0xffffffff;
        const char* str = g.string_resolve(ltrl->getId());
        if( !str ) 
            return 0xffffffff;
        uint32_t internalStrId = g.internalString_getId(str);
        return internalStrId;
    }
    uint32_t   getInternalStringIdFromLiteral( const StoredUniverse& u, const BarzerLiteral* ltrl ) {
        if( !ltrl ) 
            return 0xffffffff;
        const char* str = u.getGlobalPools().string_resolve(ltrl->getId());
        if( !str ) 
            return 0xffffffff;
        uint32_t internalStrId = u.getGlobalPools().internalString_getId(str);
        return internalStrId;
    }


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
    
    void pushFuncError( BarzelEvalContext& ctxt, const char* funcName, boost::basic_format<char>& error, const char* sig=0 )
    {
      pushFuncError(ctxt, funcName, error.str().c_str(), sig);
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
	template <class T> T operator()(T v1, T v2)
	{
		if (v2)
			return v1 / v2;
		if (!v1)
			return std::numeric_limits<T>::quiet_NaN();
		return std::numeric_limits<T>::infinity();
	}
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
		val = op(val, dt);
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
	const GlobalPools &globPools;
    BarzelEvalContext& d_ctxt;
    const char* d_funcName;

	StrConcatVisitor(const GlobalPools &u, BarzelEvalContext& ctxt, const char* funcName ) :
        globPools(u),
        d_ctxt(ctxt),
        d_funcName(funcName)
    {}

	bool operator()( const BarzerNumber &dt ) {
        ss << dt;
        return true;
    }
	bool operator()( const BarzerString &dt ) {
		ss << dt.getStr();
		return true;
	}
	bool operator()( const BarzerLiteral &dt ) {
		switch (dt.getType()) {
		case BarzerLiteral::T_STRING:
		case BarzerLiteral::T_COMPOUND: {
			const char *str = 0;
            if( dt.isInternalString() ) {
                str = (globPools.internalString_resolve(dt.getId()));
            } else {
                uint32_t fid = getInternalStringIdFromLiteral( globPools, &dt );
                if( fid == 0xffffffff ) {
			        str = globPools.stringPool.resolveId(dt.getId());
                } else {
	                str = (globPools.internalString_resolve(fid));
                }
                if( !str ) 
                    str = globPools.stringPool.resolveId(dt.getId());
            }
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
struct DateGetter_vis : public boost::static_visitor<const BarzerDate*>  {
    template <typename T>
    const BarzerDate* operator()( const T& d ) const { return 0; }

    const BarzerDate* operator()( const BarzerDate& d )     const { return &d; }
    const BarzerDate* operator()( const BarzerDateTime& d ) const { return &(d.date); }
    const BarzerDate* operator()( const BarzerRange& d )    const {
        const BarzerRange::Date* p = d.getDate();
        return ( p ? &(p->first) : 0);
    }
    const BarzerDate* operator()( const BarzerEntityRangeCombo& d ) const { return (*this)( d.getRange() ); }
};
inline const BarzerDate* getBarzerDateFromAtomic( const BarzelBeadAtomic* atomic )
{
    return boost::apply_visitor(DateGetter_vis(), atomic->getData());
}

}
// Stores map ay::UniqueCharPool::StrId -> BELFunctionStorage::function
// the function names are using format "stfun_*function name*"
// ADDFN(fname) macro is used add new functions in the constructor
// so we never have to debug typing mistakes


struct BELFunctionStorage_holder {
	const GlobalPools &gpools;
	BELStoredFunMap funmap;

	#define ADDFN(n) addFun(#n, boost::mem_fn(&BELFunctionStorage_holder::stfun_##n))
	BELFunctionStorage_holder(GlobalPools &u) : gpools(u) {
		// makers
		ADDFN(test);

		ADDFN(mkDate);
		ADDFN(mkDateRange);
		ADDFN(mkDay);
		ADDFN(mkWday);
		ADDFN(mkWeekRange);
		ADDFN(mkMonth);
        ADDFN(mkWdayEnt);
        ADDFN(mkMonthEnt);
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


		// caller
		ADDFN(call); 

        //setter
        ADDFN(set);
                
		// getters
		ADDFN(getWeekday);      // getWeekday(BarzerDate)
		ADDFN(getTokId);        // (BarzerLiteral|BarzerEntity)
		ADDFN(getTime);         // getTime(DateTime)
		ADDFN(getDate);         // getDate(DateTime)
		ADDFN(getMDay);         
		ADDFN(setMDay);
		ADDFN(getMonth);         
		ADDFN(getYear);
		ADDFN(getLow); // (BarzerRange)
		ADDFN(getHigh); // (BarzerRange)
		ADDFN(isRangeEmpty); // (BarzerRange or ERC) - returns true if range.lo == range.hi

        ADDFN(getLeftBead);  // returns closest bead on the left of the type matching arg . null otherwise
        ADDFN(getBeadSrcTok);  // a string contactenated with spaces from scrtokens of all beads in substitution sequence
		// arith
		ADDFN(textToNum);
		ADDFN(opPlus);
		ADDFN(opAdd);
		ADDFN(opMinus);
		ADDFN(opSub);
		ADDFN(opMult);
		ADDFN(opDiv);
		//logic
		ADDFN(opLt);
		ADDFN(opGt);
		ADDFN(opEq);
		ADDFN(opDateCalc);
        ADDFN(opTimeCalc);
		// string
		ADDFN(strConcat);
		// lookup
		ADDFN(lookupMonth);
		ADDFN(lookupWday);

        /// array
		ADDFN(arrSz);
		ADDFN(arrIndex);
		ADDFN(typeFilter);
		ADDFN(filterRegex);
		ADDFN(setUnmatch); // bead

		// --
		ADDFN(filterEList); // filters entity list by class/subclass (BarzerEntityList, BarzerNumber[, BarzerNumber[, BarzerNumber]])
        ADDFN(hasTopics);
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
        
        /// generic getter 
        ADDFN(get);
	}
	#undef ADDFN
    
	void addFun(const char *fname, BELStoredFunction fun) {
		// const uint32_t fid = gpools.internString_internal(fname);
		const uint32_t fid = ( const_cast<GlobalPools*>( &gpools ) )->internString_internal(fname);
		//AYLOG(DEBUG) << "adding function(" << fname << ":" << fid << ")";
		addFun(fid, fun);
	}

    const BarzerLiteral* getRvecLiteral( const BarzelEvalResultVec& rvec, size_t i ) const
    { return ( i < rvec.size() ? getAtomicPtr<BarzerLiteral>( rvec[i] ) : 0 ) ; }
    
    inline const char* literalToCString( const BarzerLiteral& ltrl ) const
    {
        return(
            ltrl.isInternalString() ? 
            gpools.internalString_resolve(ltrl.getId()) :
            gpools.string_resolve(ltrl.getId())
        );
    }
    const char* getCharPtr( const BarzelEvalResultVec& rvec, size_t i ) const
    {
        if( i >= rvec.size() )
            return 0;
        if( const BarzerLiteral* x = getRvecLiteral(rvec,i) ) {
            if( x )
                return literalToCString(*x);
        } else if( const BarzerString* x = getAtomicPtr<BarzerString>( rvec[i] )  ) {
            return x->c_str();
        } 
        return 0;
    }
	void addFun(const uint32_t fid, BELStoredFunction fun) 
        { funmap.insert(BELStoredFunRec(fid, fun)); }


	//enum { P_WEEK = 1, P_MONTH, P_YEAR, P_DECADE, P_CENTURY };
	// stored functions
	#define GETARGSTR() (q_universe.getGlobalPools().internalString_resolve(fr.getArgStrId()))
	#define GETRVECSTR(i) (q_universe.getGlobalPools().string_resolve( getRvecLiteral(rvec,i) ))

	#define STFUN(n) bool stfun_##n( BarzelEvalResult &result,\
							        const ay::skippedvector<BarzelEvalResult> &rvec,\
							        const StoredUniverse &q_universe, BarzelEvalContext& ctxt, const BTND_Rewrite_Function& fr ) const

    #define SETSIG(x) const char *sig = #x
    #define SETFUNCNAME(x) const char *func_name = #x

	#define FERROR(x) pushFuncError( ctxt, func_name, x )

	STFUN(test) {
		AYLOGDEBUG(rvec.size());
		AYLOGDEBUG(rvec[0].getBeadData().which());

		AYLOGDEBUG(result.isVec());
		return true;
	}
    /// generic getter
    STFUN(get) {
        SETFUNCNAME(call);
        const char* argStr = GETARGSTR();
        if( !argStr && rvec.size()>1) {
            const BarzerLiteral* ltrl = getAtomicPtr<BarzerLiteral>( rvec[1] ) ;
            if( ltrl ) {
                argStr =  literalToCString(*ltrl) ; 
            } else {
                const BarzerString* bs = getAtomicPtr<BarzerString>( rvec[1] ) ;
                if( bs ) 
                    argStr = bs->getStr().c_str();
            }
        }
        if( argStr && rvec.size() ) { 
            std::stringstream os;
            BarzelBeadData_FieldBinder binder( rvec[0].getBeadData(), q_universe, os );

            return binder( result.getBeadData(), argStr );
        }
        FERROR( "expects arg to be set and at least one argument" );
        return false;
    }
    STFUN(set) {
        SETFUNCNAME(set);
        const char* argStr = GETARGSTR();
        if( fr.isReqVar() ){ 
            // for now only single values are supported (no tupples yet) 
            if( rvec.size() ) {/// set<x>( rvec ) - assigns variable x to rvec[0]
                const char * vname = ctxt.resolveStringInternal(fr.getVarId());

                const BarzelBeadAtomic* atomic = rvec[0].getSingleAtomic();
                if( atomic ) {
                    ctxt.getBarz().setReqVarValue( vname, atomic->getData() );
                }
                return true;
            } else
                return false;
        } else {
            if( argStr && rvec.size() == 2 ) {
                const BarzelEvalResult& arg0 = rvec[0]; // objecct
                const BarzelEvalResult& arg1 = rvec[1]; // new property value
    
    
                const BarzelBeadAtomic* atomic = arg0.getSingleAtomic();
                if( atomic ) {
                    std::stringstream os;
                    BarzelBeadData_FieldBinder binder( arg0.getBeadData(), q_universe, os );
    
                    std::vector< BarzelBeadData > vv;
                    vv.push_back( arg1.getBeadData() );
    
                    bool rc =  binder( result.getBeadData(), argStr, &vv );
                    if( !rc ) 
                        FERROR( os.str().c_str() );
                    return rc;
                }
                return true;
            } else {
                FERROR( "expects: object, new property value. Property name in arg" );
                return false;
            }
        }
    }
    /// get bead source tokens 
    STFUN(getBeadSrcTok)
    {
        SETFUNCNAME(getBeadSrcTok);
        if( ctxt.matchInfo.isSubstitutionBeadRangeEmpty() ) {
            setResult(result,BarzerString());
            return true;
        }
        std::stringstream sstr;
        const BeadRange& rng = ctxt.matchInfo.getSubstitutionBeadRange();
        for( BeadList::const_iterator i = rng.first; i!= rng.second; ++i) 
            i->streamSrcTokens(sstr);
        setResult(result,BarzerString(sstr.str())) ;
        return true;
    }
    STFUN(getLeftBead)
    {
        SETFUNCNAME(getLeftBead);
        const char* argStr = GETARGSTR();
        if( argStr ) {
            BarzelBeadAtomic_type_t t = BarzelBeadAtomic_type_MAX;
            if( !strcmp( argStr,"date" ) ) {
                t= BarzerDate_TYPE;
            } 
            bool  foundAtomic = false;
            const BarzelBeadAtomic*  atomic = ( t!= BarzelBeadAtomic_type_MAX ? ctxt.matchInfo.getClosestAtomicFromLeft(t,foundAtomic) : 0 );
            if( !atomic ) 
                return true;

            if(t== BarzerDate_TYPE){
                const BarzerDate * dataPtr = getBarzerDateFromAtomic( atomic );
                if( dataPtr ) 
                    setResult( result, *dataPtr );
                else {
                    BarzerDate_calc calc;
                    calc.setNowPtr ( ctxt.getNowPtr() ) ;
                    calc.setToday();
                    setResult(result,calc.getDate());
                }
            }
        }
        return true;
    }
    STFUN(filterRegex)
    {
        /// parms: 0 - regex
        ///        1 - whats being filtered  - either a list of entities or an id-less entity (class/subclass) 
        ///       
        SETFUNCNAME(filterRegex);
        const char* argStr = GETARGSTR();
        /// argstr - can be 0, name, id, ... 
        if( rvec.size() > 1 ) {
            const char* rex = getCharPtr(rvec.vec(),0);
            if( !rex )
                return true;
            
            int entFilterMode = StoredEntityRegexFilter::ENT_FILTER_MODE_ID;
            if( argStr ) {
                if( !strcasecmp(argStr,"name") )
                    entFilterMode = StoredEntityRegexFilter::ENT_FILTER_MODE_NAME;
                else if( !strcasecmp(argStr,"both") )
                    entFilterMode = StoredEntityRegexFilter::ENT_FILTER_MODE_BOTH;
            }
            StoredEntityRegexFilter filter(  q_universe, rex, entFilterMode );
            const BarzelEvalResult& r1=rvec[1];

            const BarzerEntity* ent = getAtomicPtr<BarzerEntity>(r1);
            if( ent ) {
                /// if id is 0xffffffff we will filter all entities of the class 
                if( ent->isTokIdValid() ) {
                    if( filter(*ent) )
                        setResult( result, *ent );
                    else
                        return true;
                } else {
                    struct SubclassCB {
                        BarzerEntityList lst;

                        bool operator()( const StoredEntityUniqId& ent ) { lst.theList().push_back( ent ); return false; }
                    } cb;

                    gpools.getDtaIdx().entPool.iterateSubclassFilter( cb, filter, ent->getClass() );
                    setResult( result, cb.lst );
                }
                return true;
            } else if(const BarzerEntityList* entList=getAtomicPtr<BarzerEntityList>(r1) ) {
                BarzerEntityList outEList;
                for( std::vector< BarzerEntity >::const_iterator i = entList->getList().begin(); i!= entList->getList().end(); ++i ) {
                    if( filter(*i) ) 
                        outEList.theList().push_back( *i );
                }
                if( outEList.theList().size() ) 
                    setResult( result, outEList );
            }
        }
        return true;
    }
    STFUN(call) {
        SETFUNCNAME(call);
        if( rvec.size() ) {
            uint32_t fid = getInternalStringIdFromLiteral( q_universe,getRvecLiteral(rvec.vec(),0) );
	        const BELStoredFunMap::const_iterator frec = funmap.find(fid);
	        if (frec == funmap.end()) {
		        std::stringstream strstr;
		        const char *str = q_universe.getGlobalPools().internalString_resolve(fid);
                strstr << "No such function: " << (str ? str : "<unknown/>") ;//<< " (id: " << fid << ")";
                // pushFuncError(ctxt, "", strstr.str().c_str() );
                FERROR( strstr.str().c_str() );
		        return true;
	        }
            // const char* argStr = GETARGSTR();
            uint32_t argStrId = getInternalStringIdFromLiteral( q_universe,getRvecLiteral(rvec.vec(),1) );

            BTND_Rewrite_Function rwrFunc;
            rwrFunc.setNameId(fid);
            if( argStrId )
                rwrFunc.setArgStrId(argStrId);

	        return frec->second(this, result, ay::skippedvector<BarzelEvalResult>(rvec.vec(),2), q_universe, ctxt, rwrFunc );
        }

        FERROR( "expects (ltrl:functionName ltrl:arg ... ). There must be arg literal (blank or non literal ok for default)" );
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
                const char * str =  literalToCString(*ltrl);
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
	STFUN(mkDate) //(d) | (d,m) | (d,m,y) where m can be both number or entity
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
            case 2: { // Do we need to check if ent is ent(1,3) or not ?
                const BarzerEntity* be = getAtomicPtr<BarzerEntity>(rvec[1]);
                m = (be? BarzerNumber(gpools.dateLookup.resolveMonthID(q_universe,be->getTokId())) :getNumber(rvec[1]));
            }
            case 1: d = getNumber(rvec[0]);
            case 0: break; // 0 arguments = today

            default: // size > 3
            FERROR(boost::format("Expected max 3 arguments,%1%  given") % rvec.size());
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
                c.setNowPtr ( ctxt.getNowPtr() ) ;
				c.set(date.year + year, date.month + month, date.day + day);
				BarzerRange range;
				range.setData(BarzerRange::Date(date, c.d_date));
				setResult(result, range);
				return true;
			}
			default:
                           FERROR( boost::format("Expected 2-4 arguments,%1%  given") % rvec.size() );
			}
		} catch (boost::bad_get) {
                     FERROR( "Wrong argument type");
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
        calc.setNowPtr ( ctxt.getNowPtr() ) ;
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
            calc.setNowPtr ( ctxt.getNowPtr() ) ;
			calc.setToday();
			calc.setWeekday(wday);
			setResult(result, calc.d_date);
			return true;
		} catch (boost::bad_get) {
            FERROR("Wrong arg type");
		}
		return false;
	}
	
	STFUN(mkWeekRange)
	{
		SETFUNCNAME(mkWeekRange);
		
		try
		{
			int offset = ( rvec.size() ? getAtomic<BarzerNumber>(rvec[0]).getInt() : 0 );
			
			BarzerDate_calc calc;
			calc.setNowPtr ( ctxt.getNowPtr() ) ;
			calc.setToday();
			
			std::pair<BarzerDate, BarzerDate> pair;
			calc.getWeek(pair, offset);
			
			BarzerRange range;
			range.dta = pair;
			
			setResult(result, range);
			
			return true;
		}
		catch (const boost::bad_get&)
		{
			FERROR("Wrong arg type");
		}
		catch (const std::exception& e)
		{
			FERROR(e.what());
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
            calc.setNowPtr ( ctxt.getNowPtr() ) ;
            calc.setToday();
            calc.setMonth(month);
            setResult(result, calc.d_date);
            return true;
        } catch (boost::bad_get) {
            FERROR("Wrong argument type");
        }
        return false;
    }

    STFUN(mkWdayEnt)
    {
        SETFUNCNAME(mkWdayEnt);
        uint wnum(BarzerDate().getWeekday()); // that can be done softer, YANIS
        try {
                if (rvec.size()) {                      
                        const BarzerLiteral* bl = getAtomicPtr<BarzerLiteral>(rvec[0]);
                        const BarzerString* bs = getAtomicPtr<BarzerString>(rvec[0]);
                        const BarzerNumber* n = getAtomicPtr<BarzerNumber>(rvec[0]);
                        if (bl) wnum = gpools.dateLookup.lookupWeekday(q_universe,*bl);
                        else if (bs) wnum = gpools.dateLookup.lookupWeekday(q_universe,bs->getStr().c_str());
                        else if (n) wnum = ((n->isInt() && n->getInt() > 0 && n->getInt() < 8 )? n->getInt(): 0 );
                        else {  
                                FERROR("Wrong argument type");
                                return false;
                        }                       
                        if (!wnum) {
                                FERROR("Unknown weekday name given");
                                return false;
                        }                      
                }    
            uint32_t mid = gpools.dateLookup.getWdayID(q_universe,wnum);
            //check if mid exists
            const StoredEntityUniqId euid( mid, 1, 4);//put constants somewhere! c1;s4 == weekdays
            setResult(result, euid);
            return true;
        } catch(boost::bad_get) {
            FERROR("Wrong argument type");
        }
        return false;
    }    
    
    STFUN(mkMonthEnt)
    {
        SETFUNCNAME(mkMonthEnt);
        uint mnum(BarzerDate::thisMonth);
        try {
                if (rvec.size()) {                      
                        const BarzerLiteral* bl = getAtomicPtr<BarzerLiteral>(rvec[0]);
                        const BarzerString* bs = getAtomicPtr<BarzerString>(rvec[0]);
                        const BarzerNumber* n = getAtomicPtr<BarzerNumber>(rvec[0]);
                        if (bl) mnum = gpools.dateLookup.lookupMonth(q_universe,*bl);
                        else if (bs) mnum = gpools.dateLookup.lookupMonth(q_universe,bs->getStr().c_str());
                        else if (n) mnum = ((n->isInt() && n->getInt() > 0 && n->getInt() < 13 )? n->getInt(): 0 );
                        else {  
                                FERROR("Wrong argument type");
                                return false;
                        }                       
                        if (!mnum) {
                                FERROR("Unknown month given");
                                return false;
                        }                      
                }    
            uint32_t mid = gpools.dateLookup.getMonthID(q_universe,mnum);
            //check if mid exists
            const StoredEntityUniqId euid( mid, 1, 3);//put constants somewhere! c1;s3 == months
            setResult(result, euid);
            return true;
        } catch(boost::bad_get) {
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
		const GlobalPools &globPools;
		BarzerRange &range;
                bool isAutoOrder;
		uint32_t cnt;

        const char* d_funcName;
        BarzelEvalContext& d_ctxt;
        bool d_isEscaped, d_attrSet;

	RangePacker(const GlobalPools &u, BarzerRange &r, BarzelEvalContext& ctxt, const char* funcName) :
            globPools(u), range(r),
            isAutoOrder(true),
            cnt(0),
            d_funcName(funcName),
            d_ctxt(ctxt),
            d_isEscaped(false),
            d_attrSet(false)
        {}

        void setArgStr( const char* s ) 
        {
            d_isEscaped = d_attrSet = false;
            if( !s ) 
                return; 
            std::vector< std::string > vc;
            ay::separated_string_to_vec parser(vc,'|');
            parser( s ); 
            for( auto i = vc.begin();i!= vc.end(); ++i ) {
			    if (*i == "DESC") 
				   range.setDesc();
			    else if (*i == "ASC") 
				   range.setAsc();
                else if (*i == "NOHI") 
                   range.setNoHI();
			    else if (*i == "NOLO") 
                   range.setNoLO();
			    else if (*i == "FULLRANGE") 
                   range.setFullRange();
                else if (*i == "AUTO") 
                   isAutoOrder = true;
                d_attrSet = true;
            }
        }

		bool operator()(const BarzerLiteral &ltrl) {
			// const ay::UniqueCharPool &spool = globPools.stringPool;
            if( !d_isEscaped && !d_attrSet ) {
                bool leave = true;
			    if (ltrl.getId() == globPools.string_getId("DESC")) {
				    range.setDesc();
			    } else if (ltrl.getId() == globPools.string_getId("ASC")) {
				    range.setAsc();
                } else if (ltrl.getId() == globPools.string_getId("NOHI")) {
                    range.setNoHI();
			    } else if (ltrl.getId() == globPools.string_getId("NOLO")) {
                    range.setNoLO();
			    } else if (ltrl.getId() == globPools.string_getId("FULLRANGE")) {
                    range.setFullRange();
                } else if (ltrl.getId() == globPools.string_getId("AUTO")) {
                    isAutoOrder = true;
                } else if (ltrl.getId() == globPools.string_getId("\\")) {
                    d_isEscaped = true;
                } else
                    leave = false;
                if( leave )
                    return true;
            } 

            if( cnt ) {
                if( BarzerRange::Literal* er = range.get<BarzerRange::Literal>() ) 
                    er->second = ltrl;
            } else 
                range.set<BarzerRange::Literal>()->first = ltrl;
            if( d_isEscaped )
                d_isEscaped= false;

            return true;
		}

		template<class T> void setSecond(std::pair<T,T> &p, const T &v) {
                        if (isAutoOrder) {
                            p.second = v;                            
                            if (p.first < v) range.setAsc();
                                else range.setDesc();
                            return; 
                        }
                        if (p.first < v) {
                                if (range.isAsc()) {p.second = v; return;}
                                else {p.first = v; return;}
                        } else {
                                if (range.isDesc()) {p.second = v;return;}
                                else { p.first = v; return;}
                        }
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
                    pushFuncError( d_ctxt, d_funcName, boost::format("Types don't match: %1%")% range.getData().which() );
					// AYLOG(ERROR) << "Types don't match: " << range.getData().which();
					return false;
				}
			} else {
				range.setData(BarzerRange::Date(rdate, rdate));
			}
			return true;
		}

		bool operator()(const BarzerEntityList &el) {
		    const BarzerEntityList::EList &elist = el.getList();
            if( !elist.size() )
                return false;

            uint32_t maxRel = 0;
            size_t  bestEntIndex = 0; /// front by default
            for( auto i = elist.begin(); i!= elist.end(); ++i ) {
                auto rel = d_ctxt.universe.getEntityRelevance(*i);
                if( rel> maxRel )
                    bestEntIndex= ( (maxRel=rel), (i-elist.begin()) );
            }
            return (*this)( elist[bestEntIndex] );
        }
		bool operator()(const BarzerEntity &e) {
            if( cnt ) {
                if( BarzerRange::Entity* er = range.get<BarzerRange::Entity>() ) 
                    er->second = e;
                else 
                    return false;
            } else 
                range.set<BarzerRange::Entity>()->first = e;
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
					setSecond(todp, rtod);
				} catch (boost::bad_get) {
					// AYLOG(ERROR) << "Types don't match";
                    pushFuncError( d_ctxt, d_funcName, boost::format("Types don't match: %1%")% range.getData().which() );
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

            
		bool operator()(const BarzelBeadBlank&) {
            if( cnt == 0 ) {
               range.setNoLO();
            } else if( cnt ==1 ) {
               range.setNoHI();
            }
            return true;
        }
		template<class T> bool operator()(const T&) {
            pushFuncError( d_ctxt, d_funcName, "Wrong range type. Number, Time, DateTime, Range or ERC expected" );
			return false;
		}

	};

	STFUN(mkRange)
	{
        SETFUNCNAME(mkRange);
        const char* argStr = GETARGSTR();

		BarzerRange br;
		RangePacker rp(gpools, br,ctxt,func_name);
        rp.setArgStr(argStr);
		for (BarzelEvalResultVec::const_iterator ri = rvec.begin(); ri != rvec.end(); ++ri)
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
        const char* func_name;
		const StoredUniverse &universe;
        BarzelEvalContext& ctxt;

		//EntityPacker(const BELFunctionStorage_holder &h)
		EntityPacker(const StoredUniverse &u, BarzelEvalContext& ctxt, const char* funcName  )
			: cnt(0), tokId(INVALID_STORED_ID), cl(0), scl(0), func_name(funcName), universe(u), ctxt(ctxt)
        {}

		bool operator()(const BarzerString &ltrl) {
            /// need to recode ltrl into an internal string id here
			const char* str = ltrl.c_str();
            uint32_t internalStrId = 0xffffffff;
            if( str ) {
                internalStrId = universe.getGlobalPools().internalString_getId(str);
                if( internalStrId == 0xffffffff ) {
                    std::stringstream sstr;
                    sstr << "Couldn't internally resolve string: " << ( str ? str : "" );
                    FERROR( sstr.str().c_str() );
                }
            }
            return( tokId = internalStrId );
        }
		bool operator()(const BarzerLiteral &ltrl) {
            /// need to recode ltrl into an internal string id here
			const char* str = universe.getGlobalPools().string_resolve(ltrl.getId());
            uint32_t internalStrId = 0xffffffff;
            if( str ) {
                internalStrId = universe.getGlobalPools().internalString_getId(str);
                if( internalStrId == 0xffffffff ) {
                    /// couldnt internally resolve 
                    FERROR("Couldn't internally resolve. Use &lt;mkent s=\"\" c=\"\" id=\"\"/&gt; instead");
                }
            }
            return( tokId = internalStrId );
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
                    FERROR("Wrong arg type" );
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
            if( !el.getList().size() )
                return false;

            uint32_t maxRel = 0;
            size_t   bestEntIndex = 0; /// front by default
            for( auto i = el.getList().begin(); i!= el.getList().end(); ++i ) {
                auto rel = d_ctxt.universe.getEntityRelevance(*i);
                if( rel> maxRel ) 
                    bestEntIndex= ( (maxRel=rel), (i-el.getList().begin()) );
            }
			return (*this)( el.getList()[bestEntIndex] ); 
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
		const GlobalPools &gpools;
        const char* d_funcName;
        BarzelEvalContext& d_ctxt;
		ErcExprPacker(const GlobalPools &gp, BarzelEvalContext& ctxt, const char* funcName ) :
            gpools(gp), d_funcName(funcName), d_ctxt(ctxt)
        {}


		bool operator()(const BarzerLiteral &ltrl) {
			const ay::UniqueCharPool &sp = gpools.stringPool;
			if (ltrl.getId() == sp.getId("and")) {
				//AYLOG(DEBUG) << "AND";
				expr.setType(BarzerERCExpr::T_LOGIC_AND);
			} else if (ltrl.getId() == sp.getId("or")) {
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
        SETFUNCNAME(mkLtrl);
        FERROR("mkLtrl deprecated and disabled");
        /*
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
        */
		return false;
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

	STFUN(getMonth) {
        SETFUNCNAME(getMonth);
		BarzerDate bd;
		try {
            if (rvec.size())
                bd = getAtomic<BarzerDate>(rvec[0]);
            else bd.setToday();
            setResult(result, BarzerNumber(bd.month));
            return true;
		} catch (boost::bad_get) {
            FERROR("wrong argument type");
            setResult(result,BarzerNumber(0));
            return true;
        }
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
        SETFUNCNAME(entId);

        if( rvec.size() ) {
            const BarzerEntity* ent  = getAtomicPtr<BarzerEntity>(rvec[0]);
            if( !ent ) {
                FERROR("Entity expected.");
            } else {
                const char* entid = q_universe.gp.internalString_resolve(ent->getTokId());
                if (entid){
                        setResult(result, BarzerString(entid) );
                        return true;
                } else {
                        AYLOG(ERROR) << "Internal error";
                        setResult(result, BarzerString() );
                        return true;
                }
            }
        }
        return false;
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

	STFUN(getTime)
        {
            SETFUNCNAME(getTime);
            if (rvec.size()) {
                const BarzerDateTime* dt = getAtomicPtr<BarzerDateTime>(rvec[0]);
                if (dt) {
                    BarzerTimeOfDay t(dt->getTime());
                    setResult(result, t);
                    return true;
                } else {
                    FERROR("Wrong argument type. DateTime expected");
                    return false;
                }
            } else { FERROR("At least one argument is needed"); }
            return true;
        }
   
        STFUN(getDate)
        {
            SETFUNCNAME(getDate);
            if (rvec.size()) {
                const BarzerDateTime* dt = getAtomicPtr<BarzerDateTime>(rvec[0]);
                if (dt) {
                    BarzerDate d(dt->getDate());
                    setResult(result, d);
                    return true;
                } else {
                    FERROR("Wrong argument type. DateTime expected");
                    return false;
                }
            } else { 
                BarzerDate_calc calc;
                calc.setNowPtr ( ctxt.getNowPtr() ) ;
                calc.setToday();
                setResult(result,calc.getDate());
            }
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

///        mode =  BarzerRange::RANGE_MOD_SCALE_(MULT|DIV|PLUS|MINUS)  
    bool tryScaleRange( BarzelEvalResult &result, const ay::skippedvector<BarzelEvalResult> &rvec,
                        const StoredUniverse &q_universe, BarzelEvalContext& ctxt, const BTND_Rewrite_Function& fr, int mode /*RANGE_MOD_XXX*/ ) const
    {
        const char* func_name = (mode == BarzerRange::RANGE_MOD_SCALE_MULT ? "opMult" :
                            mode == BarzerRange::RANGE_MOD_SCALE_DIV ? "opDiv" :
                            mode == BarzerRange::RANGE_MOD_SCALE_PLUS ? "opPlus":
                            mode == BarzerRange::RANGE_MOD_SCALE_MINUS ?"opMinus" : "<Unknown>");
        if( rvec.size() == 2 ) { // trying to scale range 
            const BarzelEvalResult& arg0 = rvec[0];
            const BarzelEvalResult& arg1 = rvec[1];
            const BarzerNumber* n = getAtomicPtr<BarzerNumber>(arg1);
            if( n ) {
                const BarzerRange* r = getAtomicPtr<BarzerRange>(arg0);
                if( r ) {
                    BarzerRange newR(*r);                    
                    if( !newR.scale(*n, mode) ) 
                        FERROR("failed to scale the range");
                     else {
		        setResult(result, newR);
                        return true;
                    }
                } else {
                    const BarzerEntityRangeCombo* erc = getAtomicPtr<BarzerEntityRangeCombo>(arg0);
                    if( erc ) {
                        BarzerRange newR(erc->getRange());
                        if( !newR.scale(*n,mode) ) 
                            FERROR("failed to scale the range");
                         else {
                            BarzerEntityRangeCombo berc;
                            berc.setRange(newR);  
                            berc.setEntity(erc->getEntity());                             
		            setResult(result, berc);
                            return true;
                        }
                    } 
                } 
            } 
        } else  if( rvec.size() == 3 ) { // trying to scale range 
            const BarzerNumber* n1 = getAtomicPtr<BarzerNumber>(rvec[1]);
            const BarzerNumber* n2 = getAtomicPtr<BarzerNumber>(rvec[2]);            
            if( n1 && n2 ) {
                const BarzerRange* r = getAtomicPtr<BarzerRange>(rvec[0]);
                if( r ) {
                    BarzerRange newR(*r);
                    if( !newR.scale(*n1, *n2, mode) ) 
                        FERROR("failed to scale the range");
                    else {
                        setResult(result, newR);
                        return true;
                    }
                } else {
                    const BarzerEntityRangeCombo* erc = getAtomicPtr<BarzerEntityRangeCombo>(rvec[0]);
                    if( erc ) {
                        BarzerRange newR(erc->getRange());
                        if( !newR.scale(*n1, *n2,mode) ) 
                            FERROR("failed to scale the range");
                        else {
                            BarzerEntityRangeCombo berc;
                            berc.setRange(newR);  
                            berc.setEntity(erc->getEntity());                             
                            setResult(result, berc);
                            return true;
                        }
                    } 
                }
            } 
        }      
        return false;
    }
    STFUN(opPlus)
    {
        if ( tryScaleRange( result, rvec, q_universe, ctxt, fr, BarzerRange::RANGE_MOD_SCALE_PLUS ) )
            return true;
        SETFUNCNAME(opPlus);
        BarzerNumber bn(0);
        ArithVisitor<plus> visitor(bn);
        try {
            for (BarzelEvalResultVec::const_iterator ri = rvec.begin(); ri != rvec.end(); ++ri) {
                if (!boost::apply_visitor(visitor, ri->getBeadData())) {FERROR("Wrong argument types"); break;}
            }
            setResult(result, bn);
            return true;
        } catch (boost::bad_get) {
            FERROR("Wrong argument types");
        }
        return false;
    }
    STFUN(opAdd) { return stfun_opPlus(result,rvec,q_universe,ctxt,fr ); }

    STFUN(opMinus)
    {
        if ( tryScaleRange( result, rvec, q_universe, ctxt, fr, BarzerRange::RANGE_MOD_SCALE_MINUS ) )
            return true;
        SETFUNCNAME(opMinus);
        BarzerNumber &bn = setResult(result, BarzerNumber()); // NaN

        try {
            if (rvec.size()) {
                ArithVisitor<minus> visitor(bn);

                BarzelEvalResultVec::const_iterator ri = rvec.begin();

                // sets bn to the value of the first element of rvec
                bn = getNumber(*ri);
                while (++ri != rvec.end())
                    if (!boost::apply_visitor(visitor, ri->getBeadData())) {FERROR("Wrong argument types"); return false;}
                return true;
            }
        } catch (boost::bad_get) {
            FERROR("Wrong argument types");
        }
        //setResult(result, bn); // most likely NaN if something went wrong
        return false;
    }
    STFUN(opSub) { return stfun_opMinus(result,rvec,q_universe,ctxt,fr ); }
    
    STFUN(opMult)
    {
        if ( tryScaleRange( result, rvec, q_universe, ctxt, fr, BarzerRange::RANGE_MOD_SCALE_MULT ) )
            return true;
     SETFUNCNAME(opMult);
        BarzerNumber bn(1);
        ArithVisitor<mult> visitor(bn);
        try {
            for (BarzelEvalResultVec::const_iterator ri = rvec.begin(); ri != rvec.end(); ++ri) {
                if (!boost::apply_visitor(visitor, ri->getBeadData())) {FERROR("Wrong argument types"); break;}
            }
                setResult(result, bn);
                return true;
        } catch (boost::bad_get) {
            FERROR("Wrong argument types");
        }
        return false;

    }

    STFUN(opDiv) // no checks for division by zero yet
    {
        if ( tryScaleRange( result, rvec, q_universe, ctxt, fr, BarzerRange::RANGE_MOD_SCALE_DIV ) )
            return true;
        SETFUNCNAME(opDiv);
        BarzerNumber &bn = setResult(result, BarzerNumber()); // NaN
        try {
            if (rvec.size()) {
                ArithVisitor<div> visitor(bn);

                BarzelEvalResultVec::const_iterator ri = rvec.begin();

                // sets bn to the value of the first element of rvec
                bn = getNumber(*ri);
                while (++ri != rvec.end())
                    if (!boost::apply_visitor(visitor, ri->getBeadData())) {FERROR("Wrong argument types");return false;}
                return true;
            }
        } catch (boost::bad_get) {
            FERROR("Wrong argument types");
        }
        //setResult(result, bn); // most likely NaN if something went wrong
        return false;

    }
    
    struct lt {
        template <class T> bool operator()(T v1, T v2) { return v1 < v2; }
    };
    struct gt {
        template <class T> bool operator()(T v1, T v2) { return !(v1 < v2); }
    };
    struct eq {
        template <class T> bool operator()(T v1, T v2) { return v1 == v2; }
    };
    
    template <typename Op>    
    bool cmpr(BarzelEvalResult &result,
                const ay::skippedvector<BarzelEvalResult> &rvec,
                BarzelEvalContext& ctxt,
                const char* func_name) const
    {
        Op op;
        if (rvec.size() < 2) return (FERROR("At least two arguments are needed"), false);
        BarzelEvalResultVec::const_iterator end = rvec.end(),
                                            it = rvec.begin()+1;                    
        bool res = true;
        for (; res && it != end; ++it) 
            res = op(*(it-1), *it);
        setResult(result, res);
        return true;
    }

	STFUN(opLt) { SETFUNCNAME(opLt); return cmpr<lt>(result, rvec,ctxt,func_name); }
        STFUN(opGt) { SETFUNCNAME(opGt); return cmpr<gt>(result, rvec,ctxt,func_name); }
	STFUN(opEq) { SETFUNCNAME(opGt); return cmpr<eq>(result, rvec,ctxt,func_name); }

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
                c.setNowPtr ( ctxt.getNowPtr() ) ;
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

/// opTimeCalc(Time,HoursOffset[,MinOffset[,SecOffset]])	
    STFUN(opTimeCalc)   
    {
        SETFUNCNAME(opTimeCalc);
        try {
            int h = 0, m = 0, s = 0;
            switch (rvec.size()) {
            case 4:
                s = getAtomic<BarzerNumber>(rvec[3]).getInt();
            case 3:
                m = getAtomic<BarzerNumber>(rvec[2]).getInt();
            case 2: {
                h = getAtomic<BarzerNumber>(rvec[1]).getInt();
                const BarzerTimeOfDay &time = getAtomic<BarzerTimeOfDay>(rvec[0]);
                BarzerTimeOfDay out(time.getHH() + h, time.getMM() + m, time.getSS() + s);
                setResult(result, out);
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
           SETFUNCNAME(lookupMonth);
		if (!rvec.size()) {
			FERROR("Wrong number of agruments");
			return false;
		}
		try {
                        uint mnum = 0;                       
			const BarzerLiteral* bl = getAtomicPtr<BarzerLiteral>(rvec[0]);
                        const BarzerString* bs = getAtomicPtr<BarzerString>(rvec[0]);
                        const BarzerNumber* n = getAtomicPtr<BarzerNumber>(rvec[0]);
                        if (bl) mnum = gpools.dateLookup.lookupMonth(q_universe,bl->getId());
                        else if (bs) mnum = gpools.dateLookup.lookupMonth(q_universe,bs->getStr().c_str());
                        else if (n) mnum = ((n->isInt() && n->getInt() > 0 && n->getInt() < 13 )? n->getInt(): 0 );
                        else {  
                                FERROR("Wrong argument type");
                                return false;
                        }                       
                        if (!mnum) {
                              FERROR("Unknown month name given");
                              return false;
                        }
			BarzerNumber bn(mnum);
			setResult(result, bn);
			return true;
		} catch(boost::bad_get) {
			FERROR("Wrong argument type");
		}
		return false;
	}

	STFUN(lookupWday)
	{
           SETFUNCNAME(lookupWday);
		if (!rvec.size()) {
			FERROR("Wrong number of agruments");
			return false;
		}
		try {
                        const BarzerLiteral* bl = getAtomicPtr<BarzerLiteral>(rvec[0]);
                        const BarzerString* bs = getAtomicPtr<BarzerString>(rvec[0]);
			uint8_t mnum = 0;
                        if (bl) mnum = gpools.dateLookup.lookupWeekday(q_universe,bl->getId());
                        else if (bs)  mnum = gpools.dateLookup.lookupWeekday(q_universe,bs->getStr().c_str());
                        else {
                            FERROR("Wrong argument type");
                            return false;
                        }
                        if (!mnum) {
                              FERROR("Unknown weekday name given");
                              return false;
                        }
			BarzerNumber bn(mnum);
			setResult(result, bn);
			return true;
		} catch(boost::bad_get) {
			FERROR("Wrong argument type");
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
    // hasTopics [number - topicThreshold ] topic1 [, ..., topicN ]
    // returns total weight of all topics in barz, whose weight is > topicThreshold
	STFUN(hasTopics)
    {
        //SETFUNCNAME(hasTopic);

        int totalWeight = 0;
        
        int weightThreshold = 0;
        const BarzTopics& topicInfo = ctxt.getBarz().topicInfo;
        
		BarzelEvalResultVec::const_iterator ri = rvec.begin(); 
        if( ri != rvec.end() ) {
            const BarzerNumber* n  = getAtomicPtr<BarzerNumber>( *ri );
            if( n ) {
                weightThreshold = n->getInt();    
                ++ri;
            }
        }
		for (; ri != rvec.end(); ++ri) {
            const BarzerEntity* ent = getAtomicPtr<BarzerEntity>(*ri);
            if( ent ) {
                bool hasTopic = false;
                int weight = topicInfo.getTopicWeight( *ent, hasTopic );
                if( weight > weightThreshold )
                    totalWeight+= weight;
            }
        }

        if( totalWeight > 0 ) 
            setResult(result, BarzerNumber(totalWeight) );
        return true;
    }
    /// list, {class,subclass, filterClass, filterSubclass}[N]
	STFUN(topicFilterEList) //
    {
        //SETFUNCNAME(topicFilterEList);

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
            size_t rvec_size_1 = rvec.size()-3;
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
                        const TopicEntLinkage::BarzerEntitySet  * topEntSet= q_universe.getTopicEntities( topicEnt );
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

bool BELFunctionStorage::call(BarzelEvalContext& ctxt, const BTND_Rewrite_Function& fr, BarzelEvalResult &er,
									              const ay::skippedvector<BarzelEvalResult> &ervec,
									              const StoredUniverse &u) const
{
    uint32_t fid = fr.getNameId();
	const BELStoredFunMap::const_iterator frec = holder->funmap.find(fid);
	if (frec == holder->funmap.end()) {
		std::stringstream strstr;
		const char *str = u.getGlobalPools().internalString_resolve(fid);
                strstr << "No such function: " << (str ? str : "<unknown/>") ;//<< " (id: " << fid << ")";
                pushFuncError(ctxt, "", strstr.str().c_str() );
		return false;
	}
	return frec->second(holder, er, ervec, u, ctxt, fr );

}

}




