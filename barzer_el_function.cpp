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
#include <barzer_el_function_holder.h>
#include <boost/format.hpp>
#include <barzer_el_matcher.h>
#include <barzer_beni.h>

namespace barzer {

struct BTND_Rewrite_Function;

using namespace funcHolder;

namespace {
// probably can replace it by something from stl
struct plus { template <class T> T operator()(T v1, T v2) { return v1 + v2; } };
struct minus { template <class T> T operator()(T v1, T v2) { return v1 - v2; } };
struct mult { template <class T> T operator()(T v1, T v2) { return v1 * v2; } };
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

} // anon namespace

namespace {

bool tryScaleRange( BarzelEvalResult &result, const ay::skippedvector<BarzelEvalResult> &rvec,
                        const StoredUniverse &q_universe, BarzelEvalContext& ctxt, const BTND_Rewrite_Function& fr, int mode /*RANGE_MOD_XXX*/ ) 
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
                const BarzerERC* erc = getAtomicPtr<BarzerERC>(arg0);
                if( erc ) {
                    BarzerRange newR(erc->getRange());
                    if( !newR.scale(*n,mode) ) 
                        FERROR("failed to scale the range");
                     else {
                        BarzerERC berc;
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
                const BarzerERC* erc = getAtomicPtr<BarzerERC>(rvec[0]);
                if( erc ) {
                    BarzerRange newR(erc->getRange());
                    if( !newR.scale(*n1, *n2,mode) ) 
                        FERROR("failed to scale the range");
                    else {
                        BarzerERC berc;
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
} // tryScaleRange

struct lt { template <class T> bool operator()(T v1, T v2) { return v1 < v2; } };
struct gt { template <class T> bool operator()(T v1, T v2) { return !(v1 < v2); } };
struct eq { template <class T> bool operator()(T v1, T v2) { return v1 == v2; } };

template <typename Op>    
bool cmpr(BarzelEvalResult &result,
            const ay::skippedvector<BarzelEvalResult> &rvec,
            BarzelEvalContext& ctxt,
            const char* func_name)
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

/// ACTUAL FUNCTIONS
	//enum { P_WEEK = 1, P_MONTH, P_YEAR, P_DECADE, P_CENTURY };
	// stored functions

	FUNC_DECL(test) {
		AYLOGDEBUG(rvec.size());
		AYLOGDEBUG(rvec[0].getBeadData().which());

		AYLOGDEBUG(result.isVec());
		return true;
	}
    /// generic getter
    FUNC_DECL(get) {
        SETFUNCNAME(call);
        const char* argStr = GETARGSTR();
        if( !argStr && rvec.size()>1) {
            const BarzerLiteral* ltrl = getAtomicPtr<BarzerLiteral>( rvec[1] ) ;
            if( ltrl ) {
                argStr =  h->literalToCString(*ltrl) ; 
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
    FUNC_DECL(set) {
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
    FUNC_DECL(getBeadSrcTok)
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
    FUNC_DECL(getLeftBead)
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
    FUNC_DECL(filterRegex)
    {
        /// parms: 0 - regex
        ///        1 - whats being filtered  - either a list of entities or an id-less entity (class/subclass) 
        ///       
        SETFUNCNAME(filterRegex);
        const char* argStr = GETARGSTR();
        /// argstr - can be 0, name, id, ... 
        if( rvec.size() > 1 ) {
            const char* rex = h->getCharPtr(rvec.vec(),0);
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

                        bool operator()( const StoredEntityUniqId& ent, uint32_t entId ) { lst.theList().push_back( ent ); return false; }
                    } cb;

                    h->gpools.getDtaIdx().entPool.iterateSubclassFilter( cb, filter, ent->getClass() );
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
    FUNC_DECL(call) {
        SETFUNCNAME(call);
        if( rvec.size() ) {
            uint32_t fid = getInternalStringIdFromLiteral( q_universe, getRvecLiteral(rvec.vec(),0) );
            const auto& funmap = *(h->funmap);
	        const FuncMap::const_iterator frec = funmap.find(fid);
	        if (frec == funmap.end()) {
		        std::stringstream strstr;
		        const char *str = q_universe.getGlobalPools().internalString_resolve(fid);
                strstr << "No such function: " << (str ? str : "<unknown/>") ;//<< " (id: " << fid << ")";
                // pushFuncError(ctxt, "", strstr.str().c_str() );
                FERROR( strstr.str().c_str() );
		        return true;
	        }
            // const char* argStr = GETARGSTR();
            uint32_t argStrId = getInternalStringIdFromLiteral( q_universe, getRvecLiteral(rvec.vec(),1) );

            BTND_Rewrite_Function rwrFunc;
            rwrFunc.setNameId(fid);
            if( argStrId )
                rwrFunc.setArgStrId(argStrId);

	        return frec->second.first(h, result, ay::skippedvector<BarzelEvalResult>(rvec.vec(),2), q_universe, ctxt, rwrFunc );
        }

        FERROR( "expects (ltrl:functionName ltrl:arg ... ). There must be arg literal (blank or non literal ok for default)" );
        return true;
    }
    FUNC_DECL(getEnt) {
        SETFUNCNAME(getEnt);
        if(rvec.size() )  {
            if( const BarzerEVR* evr  = getAtomicPtr<BarzerEVR>(rvec[0]) ) {
                setResult(result, evr->getEntity() );
                return true;
            } else { 
                const BarzerERC* erc  = getAtomicPtr<BarzerERC>(rvec[0]);
                const BarzerEntity* ent  = ( erc ? &(erc->getEntity()) : getAtomicPtr<BarzerEntity>(rvec[0]) );
                if( ent ) {
                    if( rvec.size() == 1 ) { /// this is a getter
                        setResult(result, *ent );
                        return true;
                    } else if( rvec.size() == 2 ) { // this is a setter
                        const BarzerERC* r_erc  = getAtomicPtr<BarzerERC>(rvec[1]);
                        const BarzerEntity* r_ent  = ( r_erc ? &(r_erc->getEntity()) : getAtomicPtr<BarzerEntity>(rvec[1]) );
    
                        if( r_ent ) {
                            setResult(result, *r_ent );
                            return true;
                        }
                    }
                }
            }
        }

        FERROR( "expects ( (ERC|Entity) [,entity] ) to get/set entity for erc or bypass" );
        return true;
    }
    FUNC_DECL(getRange){
        SETFUNCNAME(getRange);
        if(rvec.size() )  {
            const BarzerERC* erc  = getAtomicPtr<BarzerERC>(rvec[0]);
            const BarzerRange* range  = ( erc ? &(erc->getRange()) : getAtomicPtr<BarzerRange>(rvec[0]) );
            if( range ) {
                if( rvec.size() == 1 ) { /// this is a getter
                    setResult(result, *range );
                    return true;
                } else if( rvec.size() == 2 ) { // this is a setter
                    const BarzerERC* r_erc  = getAtomicPtr<BarzerERC>(rvec[1]);
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
    FUNC_DECL(isRangeEmpty)
    {
        SETFUNCNAME(isRangeEmpty);
        if( rvec.size() == 1 ) {
            const BarzerRange* range  = getAtomicPtr<BarzerRange>(rvec[0]);

            if( !range ) {
                const BarzerERC* erc  = getAtomicPtr<BarzerERC>(rvec[0]);

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
	FUNC_DECL(textToNum) {
        SETFUNCNAME(text2Num);
        int langId = 0; // language id . 0 means english
        ay::char_cp_vec tok;

        BarzerNumber bn;
		for (BarzelEvalResultVec::const_iterator ri = rvec.begin(); ri != rvec.end(); ++ri) {
            const BarzerLiteral* ltrl = getAtomicPtr<BarzerLiteral>( *ri ) ;
            if( ltrl )  {
                const char * str =  h->literalToCString(*ltrl);
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
	FUNC_DECL(mkEnityList) {
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

		bool operator()(const BarzerERC &combo) {
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

	FUNC_DECL(rangeFuzz)
    {
        SETFUNCNAME(rangeFuzz);
        
        if( rvec.size() != 1 ) {
            FERROR( "expect 1 parameters: numeric range (fuzz factor in argStr, default 20%)" );
            return false;
        }
        const char* argStr = GETARGSTR();
        double fuzzFactor = ( argStr ? fabs( (atof(argStr)/100.0) ) : .2 );

        const BarzerRange *x = getAtomicPtr<BarzerRange>(rvec[0]);
        const BarzerERC* erc = 0;
        
        if( !x ) {
            if( (erc = getAtomicPtr<BarzerERC>(rvec[0])) != nullptr ) 
                x = erc->getRangePtr();
        }
        if( !x || !x->isNumeric() ) {
            FERROR( "can only fuzz numeric ranges or ERC with num range" );
            return false;
        }

        BarzerRange newRange(*x);
        BarzerRange::Real* rr = newRange.set<BarzerRange::Real>() ;
        if( const BarzerRange::Integer* r = x->getInteger() ) {
            if( x->hasHi()) 
                rr->second = ( (double)(r->second) * ( 1+fuzzFactor ) );
            if( x->hasLo()) 
                rr->first = ( (double)(r->first) * ( 1-fuzzFactor ) );
        } else 
        if( const BarzerRange::Real* r = x->getReal() ) {
            if( x->hasHi()) 
                rr->second = ( (r->second) * ( 1.0+fuzzFactor ) );
            if( x->hasLo()) 
                rr->first = ( (r->first) * ( 1.0-fuzzFactor ) );
        }
        if( erc ) {
            BarzerERC newErc( *erc );
            newErc.setRange( newRange );
            setResult( result, newErc );
        }  else 
            setResult( result, newRange );
        return true;
    }
	FUNC_DECL(mkRange)
	{
        SETFUNCNAME(mkRange);
        const char* argStr = GETARGSTR();

		BarzerRange br;
		RangePacker rp(h->gpools, br,ctxt,func_name);
        rp.setArgStr(argStr);
		for (BarzelEvalResultVec::const_iterator ri = rvec.begin(); ri != rvec.end(); ++ri)
			if (!boost::apply_visitor(rp, ri->getBeadData())) return false;

		setResult(result, br);
		return true;
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

	FUNC_DECL(lookupEnt) // similar to mkEnt except entities of given class are looked up by id
    {
        SETFUNCNAME(lookupEnt);
        const char* argStr = GETARGSTR();

        enum {  
            LKUPMODE_ID,     // by entity id
            LKUPMODE_NAME,   // by entity name
            LKUPMODE_NAME_ID // both
        } lkMode= LKUPMODE_ID;
        
        size_t maxEnt = 0;
        if( argStr ) {
            if( const char* x = strstr( argStr, "NAME" ) ){
                if( !strncmp( x, "NAME_ID", (sizeof("NAME_ID")-1) ) ) 
                    lkMode = LKUPMODE_NAME_ID;
                else
                    lkMode = LKUPMODE_NAME;
            }
            size_t maxEnt = atoi(argStr);
        } 

        QuestionParm qparm;
        StoredEntityClass ec;
        std::string idStr;
        if( rvec.size() == 2 ) {
            ec.ec = q_universe.getUserId();
            if( const BarzerNumber *x = getAtomicPtr<BarzerNumber>(rvec[0]) ) 
                ec.subclass = (uint32_t) x->getInt();
            else 
                FERROR("first arg must be a number" );

            getString( idStr, rvec[1], q_universe );
        } else if( rvec.size() == 3 ) { 
            if( const BarzerNumber *x = getAtomicPtr<BarzerNumber>(rvec[0]) ) 
                ec.ec = (uint32_t) x->getInt();
            else 
                FERROR("first arg must be a number" );

            if( const BarzerNumber *x = getAtomicPtr<BarzerNumber>(rvec[1]) ) 
                ec.subclass = (uint32_t) x->getInt();
            else 
                FERROR("second arg must be a number" );
            getString( idStr, rvec[2], q_universe );
        }  else
            FERROR( "must have 2 or 3 args: [class,] subclass, idStr" );

        BENIFindResults_t beniResult;

        if( lkMode == LKUPMODE_ID || lkMode == LKUPMODE_NAME_ID ) {
            q_universe.entLookupBENISearch( beniResult, idStr.c_str(), ec, qparm );

            if( maxEnt && beniResult.size() > maxEnt )
                beniResult.resize( maxEnt );
        } 
        if( lkMode == LKUPMODE_NAME || lkMode == LKUPMODE_NAME_ID ) {
            if( !maxEnt || beniResult.size() < maxEnt ) {
                if( const SmartBENI*  beni = q_universe.getBeni() ) {
                    BENIFindResults_t beniResultName;
                    size_t maxCount = ( maxEnt ? (maxEnt-beniResult.size()) : 128 );
                    beni->search( beniResultName, idStr.c_str(), 0.3, 0, [&] (const BarzerEntity& i ) { return (ec == i.eclass); }, maxCount ); 
                    for( auto i = beniResultName.begin(); i!= beniResultName.end(); ++i ) {
                        if( !maxEnt || beniResult.size() < maxEnt ) 
                            beniResult.push_back( *i );
                     }
                }
            }
        }
        
        if( beniResult.empty() ) {
            setResult(result, BarzerEntity() );
            return true;
        } else 
        if( beniResult.size() == 1 )
            setResult(result, beniResult.front().ent);
        else {
            BarzerEntityList& newEntList = setResult(result, BarzerEntityList() );
            double maxCov = beniResult.front().coverage;
            for( const auto& i : beniResult ) {
                if( maxCov> i.coverage ) 
                    break;
                newEntList.addEntity( i.ent );
            }
        }
        return true;
    }

	FUNC_DECL(mkEnt) // makes Entity
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

		BarzerERC &erc;
		ERCPacker(BarzerERC &e, BarzelEvalContext& ctxt, const char* funcName, bool entitySet = false ) :
            num(entitySet? 1: 0), d_funcName(funcName), d_ctxt(ctxt), erc(e) {}
        
        
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
		bool operator()(const BarzerERC &other) {
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
	struct EVRPacker : public boost::static_visitor<bool> {
        std::string tuppleName;
        BarzerEVR& evr;
        EVRPacker( BarzerEVR& e ) : evr(e) {}
		bool operator()(const BarzelBeadAtomic &data) 
            { return boost::apply_visitor(*this, data.getData()); }
		bool operator()(const BarzelBeadBlank& v) 
            { return false; }
		bool operator()(const BarzelBeadExpression& v) 
            { return false; }
		bool operator()(const BarzerEVR& v) 
            { 
                for( auto i = v.data().begin(); i!= v.data().end(); ++i ) {
                    for( auto j = i->second.begin(); j!= i->second.end(); ++j ) {
                    evr.appendVarUnique( i->first, *j );
                    }
                }
                return true; 
            }

        template <typename T>
        bool operator()(const T& t) { evr.appendVarUnique( tuppleName, t ); return true;}

    };


	FUNC_DECL(mkEVR) // makes EVR
    {
        SETFUNCNAME(mkEVR);
        const char* argStr = GETARGSTR();
        BarzerEVR evr;
        if( !rvec.size() ) 
            return true;

		BarzelEvalResultVec::const_iterator ri = rvec.begin(); 
        const BarzerEVR* srcEvr = getAtomicPtr<BarzerEVR>(*ri);
        if( srcEvr ) {
            evr = *srcEvr;
        } else if( const BarzerEntity* ent = getAtomicPtr<BarzerEntity>(*ri) ) {
            evr.setEntity( *ent );
        } 
        EVRPacker packer( evr );
        if( argStr ) packer.tuppleName.assign(argStr);
		for ( ++ri; ri != rvec.end(); ++ri ) {
            boost::apply_visitor( packer, ri->getBeadData() );
        }
		setResult(result, evr);
        return true;
    }
	FUNC_DECL(mkERC) // makes EntityRangeCombo
	{
		//AYLOGDEBUG(rvec.size());
		//AYLOG(DEBUG) << rvec[0].getBeadData().which();
        SETFUNCNAME(mkERC);
            
        if( !rvec.size() ) {
		    setResult(result, BarzerERC());
            return true;
        }
        /// if first parameter is ent list
        if( const BarzerEntityList* entList  = getAtomicPtr<BarzerEntityList>(rvec[0]) ) {
            if( entList->getList().size() ) {
                result.getBeadDataVec().reserve( entList->getList().size() );
                for( const auto& i: entList->getList() )  {
		            BarzelEvalResultVec::const_iterator ri = rvec.begin();
		            for (++ri; ri != rvec.end(); ++ri) {
		                BarzerERC erc(i);
                        ERCPacker v(erc,ctxt,func_name,true);
                        boost::apply_visitor(v, ri->getBeadData());
                        result.pushBeadData( erc );
                    }
                }
            } else
                setResult(result, BarzerERC());
        } else {
		    BarzerERC erc;
		    ERCPacker v(erc,ctxt,func_name);
		    for (BarzelEvalResultVec::const_iterator ri = rvec.begin(); ri != rvec.end(); ++ri) {
			    if (!boost::apply_visitor(v, ri->getBeadData())) {
                        FERROR("ERC failed");
                        return false;
			    }
            } 
		    setResult(result, erc);
        }
		return true;
	}

	FUNC_DECL(mkErcExpr)
	{
        SETFUNCNAME(mkErcExpr);
        const char* argStr = GETARGSTR();
		if (rvec.size() < 2) {
            FERROR("needs at least 2 arguments");
			return false;
		}
    
        BarzerRange defaultRange;
        if( const BarzerRange* r = getAtomicPtr<BarzerRange>(rvec[0]) ) { // special case range, ent, ent
            defaultRange = *r;
        }

        BarzerERCExpr expr;
        expr.setLogic( argStr );
        for( auto j = rvec.begin(); j!= rvec.end(); ++j ) {
            const auto& i = *j;

            if( const BarzerERC *x = getAtomicPtr<BarzerERC>(i) )
                expr.addClause( *x );
            else if( const BarzerERCExpr *x = getAtomicPtr<BarzerERCExpr>(i) )
                expr.addClause( *x );
            else if( const BarzerEntity *x = getAtomicPtr<BarzerEntity>(i) ) {
                BarzerERC erc;
                erc.setRange( defaultRange );
                erc.setEntity(*x);
                expr.addClause( erc );
            } else if( j != rvec.begin() || !isAtomic<BarzerRange>(i) ) {
                std::stringstream sstr;
                sstr << "parameter " << j-rvec.begin() << " has invalid type";
                FERROR( sstr.str().c_str() );
            }
        }
        setResult(result, expr);

		return true;
	}

	FUNC_DECL(mkLtrl)
	{
        SETFUNCNAME(mkLtrl);
        FERROR("mkLtrl deprecated and disabled");
		return false;
	}

	// set "stop" type on the incoming literal.
	FUNC_DECL(mkFluff)
	{
        SETFUNCNAME(mkFluff);
        const char* argStr = GETARGSTR();
        if( argStr && !strcmp( argStr, "concat" ) ) {
            BarzerString outBarzerStr ;
            std::string& outStr = outBarzerStr.getStr();
            bool prevWasBlank = false;
            for ( const auto& rvit : rvec ) {
                const auto& beadList = ctxt.matchInfo.getBeadList();

                const BarzelBeadDataVec &vec = rvit.getBeadDataVec();
                result.boostConfidence();
                size_t j = 0;
                for (const auto& i : vec ) {
                    if( const BarzerLiteral *x = getAtomicPtr<BarzerLiteral>(i) ) {
                        if( x->isBlank())  {
                            prevWasBlank= true;
                            continue;
                        }
			            const char* str = q_universe.getGlobalPools().string_resolve(x->getId());
                        if(str) {
                            if( prevWasBlank )
                                outStr.push_back( ' ' );
                            outStr.append( str );
                        }
                        
                    } else 
                    if( const BarzerNumber *x = getAtomicPtr<BarzerNumber>(i) ) {
                        BarzerString ns;
                        x->convert(ns);
                        if( prevWasBlank ) {
                            outStr.push_back( ' ' );
                            prevWasBlank = false;
                        }
                        outStr.append(ns.getStr());
                    } else
                    if( const BarzerString *x = getAtomicPtr<BarzerString>(i) ) {
                        outStr.append(x->getStr());
                        if( prevWasBlank ) {
                            outStr.push_back( ' ' );
                            prevWasBlank = false;
                        }
                    } else 
                        prevWasBlank = true;
                }
            }
            setResult(result, outBarzerStr.setFluff() );
            return true;
        }
        BarzerLiteral &ltrl = setResult(result, BarzerLiteral());
        ltrl.setStop();
        for ( const auto& rvit : rvec ) {
            const auto& beadList = ctxt.matchInfo.getBeadList();

            const BarzelBeadDataVec &vec = rvit.getBeadDataVec();
            result.boostConfidence();
            size_t j = 0;

            for (const auto& i : vec ) {
                if( const BarzerLiteral *x = getAtomicPtr<BarzerLiteral>(i) ) {
                    if( x->isString() || x->isCompound() ) {
                        BarzerLiteral ltrl;
                        ltrl.setStop();
                        ltrl.setId( x->getId());
                        result.pushOrSetBeadData( j++, ltrl );
                    }
                } else if( const BarzerString *x = getAtomicPtr<BarzerString>(i) ) {
                    result.pushOrSetBeadData( j++, BarzerString(*x).setFluff() );
                } else if( const BarzerNumber *x = getAtomicPtr<BarzerNumber>(i) ) {
                    BarzerString bs;
                    x->convert(bs);
                    bs.setFluff();
                    result.pushOrSetBeadData( j++, bs );
                } 
            }
            return true;
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

	FUNC_DECL(mkExprTag) {
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
	FUNC_DECL(mkExprAttrs) {
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

	FUNC_DECL(entId)
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
	FUNC_DECL(entClass)
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
	FUNC_DECL(entSetSubclass)
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
	FUNC_DECL(entSubclass)
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

	FUNC_DECL(getTokId)
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
		bool operator()(const BarzerERC &erc)
			{ return operator()(erc.getRange()); }
		bool operator()(const BarzelBeadAtomic &data)
			{ return boost::apply_visitor(*this, data.getData()); }
		// not applicable
		template<class T> bool operator()(const T&) {
            pushFuncError(d_ctxt, d_funcName, "Type mismatch" );
			return false;
		}

	};

	FUNC_DECL(getLow)
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

	FUNC_DECL(getHigh)
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
    FUNC_DECL(opPlus)
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
    FUNC_DECL(opAdd) { return stfun_opPlus(h,result,rvec,q_universe,ctxt,fr ); }

    FUNC_DECL(opMinus)
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
    FUNC_DECL(opSub) { return stfun_opMinus(h,result,rvec,q_universe,ctxt,fr ); }
    
    FUNC_DECL(opMult)
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

    FUNC_DECL(opDiv) // no checks for division by zero yet
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
    
	FUNC_DECL(opLt) { SETFUNCNAME(opLt); return cmpr<lt>(result, rvec,ctxt,func_name); }
        FUNC_DECL(opGt) { SETFUNCNAME(opGt); return cmpr<gt>(result, rvec,ctxt,func_name); }
	FUNC_DECL(opEq) { SETFUNCNAME(opGt); return cmpr<eq>(result, rvec,ctxt,func_name); }

    // concatenates all parameters as one list
	FUNC_DECL(listCat) { //
        BarzelBeadDataVec& resultVec = result.getBeadDataVec();
        for( BarzelEvalResultVec::const_iterator i = rvec.begin(); i!= rvec.end(); ++i ) {
            const BarzelBeadDataVec& v = i->getBeadDataVec();
            for( BarzelBeadDataVec::const_iterator vi = v.begin(); vi !=v.end(); ++vi )
                resultVec.push_back( *vi );
        }
        return true;
    }
	FUNC_DECL(setUnmatch) { //
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
	FUNC_DECL(typeFilter) { //
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
	FUNC_DECL(arrIndex) { //
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
    FUNC_DECL(arrSz) {
        BarzerNumber bn;
        setResult(result, BarzerNumber(
            (int)( rvec.size() ? rvec[0].getBeadDataVec().size() : 0 )
        ));
        return true;
    }
	// string
	FUNC_DECL(strConcat) { // strfun_strConcat(&result, &rvec)
        SETFUNCNAME(strConcat);
		if (rvec.size()) {
			StrConcatVisitor scv(h->gpools,ctxt,func_name);
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

	FUNC_DECL(lookupMonth)
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
                        if (bl) mnum = h->gpools.dateLookup.lookupMonth(q_universe,bl->getId());
                        else if (bs) mnum = h->gpools.dateLookup.lookupMonth(q_universe,bs->getStr().c_str());
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

	FUNC_DECL(lookupWday)
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
                        if (bl) mnum = h->gpools.dateLookup.lookupWeekday(q_universe,bl->getId());
                        else if (bs)  mnum = h->gpools.dateLookup.lookupWeekday(q_universe,bs->getStr().c_str());
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
    /// mkEntList( entities and entity lists in any order )
	FUNC_DECL(mkEntList) // (BarzerEntityList, EntityList, BarzerNumber[, BarzerNumber[, BarzerNumber]])
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

	FUNC_DECL(filterEList) // (BarzerEntityList, BarzerNumber[, BarzerNumber[, BarzerNumber]])
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

BELFunctionStorage_holder::DeclInfo g_funcs[] = {

    FUNC_DECLINFO_INIT(test, "test function"),

    FUNC_DECLINFO_INIT(mkRange, "makes range (1 or 2 parms - number, date etc.)"),
    FUNC_DECLINFO_INIT(rangeFuzz, "fuzzes range by given percentage (argstr)"),
    FUNC_DECLINFO_INIT(mkEnt, ""),
    FUNC_DECLINFO_INIT(lookupEnt, ""),
    FUNC_DECLINFO_INIT(mkERC, ""),
    FUNC_DECLINFO_INIT(mkEVR, ""),
    FUNC_DECLINFO_INIT(mkErcExpr, ""),
    FUNC_DECLINFO_INIT(mkFluff, ""),

    FUNC_DECLINFO_INIT(mkExprTag, ""),
    FUNC_DECLINFO_INIT(mkExprAttrs, ""),
    // caller
    FUNC_DECLINFO_INIT(call, ""),

    //setter
    FUNC_DECLINFO_INIT(set, ""),
            
    // getters
    FUNC_DECLINFO_INIT(getLow, ""), // (BarzerRange)
    FUNC_DECLINFO_INIT(getTokId, ""),        // (BarzerLiteral|BarzerEntity)
    FUNC_DECLINFO_INIT(getHigh, ""), // (BarzerRange)
    FUNC_DECLINFO_INIT(isRangeEmpty, ""), // (BarzerRange or ERC) - returns true if range.lo == range.hi

    FUNC_DECLINFO_INIT(getLeftBead, ""),  // returns closest bead on the left of the type matching arg . null otherwise
    FUNC_DECLINFO_INIT(getBeadSrcTok, ""),  // a string contactenated with spaces from scrtokens of all beads in substitution sequence
    // arith
    FUNC_DECLINFO_INIT(textToNum, ""),
    FUNC_DECLINFO_INIT(opPlus, ""),
    FUNC_DECLINFO_INIT(opAdd, ""),
    FUNC_DECLINFO_INIT(opMinus, ""),
    FUNC_DECLINFO_INIT(opSub, ""),
    FUNC_DECLINFO_INIT(opMult, ""),
    FUNC_DECLINFO_INIT(opDiv, ""),
    //logic
    FUNC_DECLINFO_INIT(opLt, ""),
    FUNC_DECLINFO_INIT(opGt, ""),
    FUNC_DECLINFO_INIT(opEq, ""),
    // string
    FUNC_DECLINFO_INIT(strConcat, ""),
    // lookup
    FUNC_DECLINFO_INIT(lookupMonth, ""),
    FUNC_DECLINFO_INIT(lookupWday, ""),

    /// array
    FUNC_DECLINFO_INIT(arrSz, ""),
    FUNC_DECLINFO_INIT(arrIndex, ""),
    FUNC_DECLINFO_INIT(typeFilter, ""),
    FUNC_DECLINFO_INIT(filterRegex, ""),
    FUNC_DECLINFO_INIT(setUnmatch, ""), // bead

    // --
    FUNC_DECLINFO_INIT(filterEList, ""), // filters entity list by class/subclass (BarzerEntityList, BarzerNumber[, BarzerNumber[, BarzerNumber]])
    FUNC_DECLINFO_INIT(mkEntList, ""), // (BarzerEntity, ..., BarzerEntity) will also accept BarzerEntityList

    // ent properties
    FUNC_DECLINFO_INIT(entClass, ""), // (Entity)
    FUNC_DECLINFO_INIT(entSubclass, ""), // (Entity)
    FUNC_DECLINFO_INIT(entId, ""), // (Entity)
    FUNC_DECLINFO_INIT(entSetSubclass, ""), // (Entity,new subclass)

    // erc properties
    FUNC_DECLINFO_INIT(getEnt, ""), // ((EVR|ERC|Entity)[,entity]) -- when second parm passed replaces entity with it
    FUNC_DECLINFO_INIT(getRange, ""), // ((ERC|Entity)[,range]) -- when second parm passed replaces range with it
    
    /// generic getter 
    FUNC_DECLINFO_INIT(get, "")
};

    //// END OF ACTUAL FUNCTIONS
	#undef FERROR
    #undef SETSIG
	#undef FUNC_DECL
} /// anon namespace 


void BELFunctionStorage::loadAllFunctions()
{
    for( const auto& i : g_funcs ) {
        holder->addFun( i );
    }
}
BELFunctionStorage::BELFunctionStorage(GlobalPools &gp, bool initFunctions) : globPools(gp),
		holder(initFunctions ? new BELFunctionStorage_holder(gp):0) 
{ 
    if( holder ) {
        loadAllFunctions( );
        funcHolder::loadAllFunc_date(holder);
        funcHolder::loadAllFunc_topic(holder);
    }
}

BELFunctionStorage::~BELFunctionStorage()
{
	delete holder;
}

const FuncMap& BELFunctionStorage::getFuncMap() const
    { return holder->getFuncMap(); }
bool BELFunctionStorage::call(BarzelEvalContext& ctxt, const BTND_Rewrite_Function& fr, BarzelEvalResult &er,
									              const ay::skippedvector<BarzelEvalResult> &ervec,
									              const StoredUniverse &u) const
{
    uint32_t fid = fr.getNameId();
	const FuncMap::const_iterator frec = holder->funmap->find(fid);
	if (frec == holder->funmap->end()) {
		std::stringstream strstr;
		const char *str = u.getGlobalPools().internalString_resolve(fid);
                strstr << "No such function: " << (str ? str : "<unknown/>") ;//<< " (id: " << fid << ")";
                pushFuncError(ctxt, "", strstr.str().c_str() );
		return false;
	}
	return frec->second.first(holder, er, ervec, u, ctxt, fr );

}

void BELFunctionStorage::help_list_funcs_json( std::ostream& os, const GlobalPools& gp ) 
{
    if( !gp.funSt )
        return;
	os << "[\n";
	bool isFirst = true;
    const auto& funmap = gp.funSt->holder->getFuncMap();
	for( auto i = funmap.begin(); i!= funmap.end(); ++i ) {
        const auto& funcInfo = i->second.second;
		if (!isFirst)
			os << ",\n";
		isFirst = false;

		os << "{ ";
		os << "\"name\": \"" << funcInfo.name << "\"";
        if( !funcInfo.descr.empty() )
		    ay::jsonEscape(funcInfo.descr.c_str(), os << ", \"desc\": ", "\"") << " ";
		os << "}";
	}
	os << "]\n";
}

}




