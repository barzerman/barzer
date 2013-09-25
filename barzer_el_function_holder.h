#pragma once

#include <barzer_el_function.h>
#include <boost/format.hpp>


namespace barzer {

struct BELFunctionStorage_holder {
	GlobalPools &gpools;

    struct DeclInfo {
        Func f;
        const char* name;
        const char* descr;
        
        DeclInfo() : f(0), name(0), descr(0) {}
        DeclInfo( Func x, const char* n, const char* d=0, bool dissed= false) :
            f(x), name(n), descr(d? d: "") {}
    };

	FuncMap funmap;
    void addAllFunctions( std::pair< const DeclInfo*, const DeclInfo*> range );

    const FuncMap& getFuncMap() const { return funmap; }

	BELFunctionStorage_holder(GlobalPools &u);
    
	void addFun(const char *fname, const char *descr, Func fun);
	void addFun( const DeclInfo& di )
        { addFun( di.name, di.descr, di.f ); }
    const char* literalToCString( const BarzerLiteral& ltrl ) const;
    const char* getCharPtr( const BarzelEvalResultVec& rvec, size_t i ) const;
	void addFun(const uint32_t fid, const char* fname, const char* descr, Func fun);
	const StoredEntity* fetchEntity(uint32_t tokId, const uint16_t cl, const uint16_t scl) const;
}; /// holder 
#define FUNC_DECLINFO_INIT(n,d) BELFunctionStorage_holder::DeclInfo(stfun_##n, #n, d)
#define GETARGSTR() (q_universe.getGlobalPools().internalString_resolve(fr.getArgStrId()))
#define GETRVECSTR(i) (q_universe.getGlobalPools().string_resolve( getRvecLiteral(rvec,i) ))

#define FUNC_DECL(n) bool stfun_##n( const BELFunctionStorage_holder* h, BarzelEvalResult &result,\
                                const ay::skippedvector<BarzelEvalResult> &rvec,\
                                const StoredUniverse &q_universe, BarzelEvalContext& ctxt, const BTND_Rewrite_Function& fr ) 

#define SETSIG(x) const char *sig = #x
#define SETFUNCNAME(x) const char *func_name = #x

#define FERROR(x) pushFuncError( ctxt, func_name, x )
namespace funcHolder {
uint32_t   getInternalStringIdFromLiteral( const GlobalPools& g, const BarzerLiteral* ltrl );
uint32_t   getInternalStringIdFromLiteral( const StoredUniverse& u, const BarzerLiteral* ltrl );
void pushFuncError( BarzelEvalContext& ctxt, const char* funcName, const char* error, const char* sig=0 );
void pushFuncError( BarzelEvalContext& ctxt, const char* funcName, boost::basic_format<char>& error, const char* sig=0 );

// extracts a string from BarzerString or BarzerLiteral
const char* extractString(const GlobalPools &u, const BarzelEvalResult &result);

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
template<class T> bool isAtomic(const BarzelBeadData &dta) { return (getAtomicPtr<T>(dta) != nullptr); }

template<class T> const T* getAtomicPtr(const BarzelEvalResult &res) { return getAtomicPtr<T>( res.getBeadData() ); }

template<class T> bool isAtomic(const BarzelEvalResult &res) { return isAtomic<T>( res.getBeadData() ); }
void getString(std::string& str, const BarzelEvalResult &r, const StoredUniverse& u);

template<class T> const BarzerNumber& getNumber(const T &v)
    { return getAtomic<BarzerNumber>(v); }

const BarzerLiteral* getRvecLiteral( const BarzelEvalResultVec& rvec, size_t i );

uint32_t getTokId(const char* tokStr, const StoredUniverse &u);
uint32_t getTokId(const BarzerLiteral &l, const StoredUniverse &u);

const BarzerDate* getBarzerDateFromAtomic( const BarzelBeadAtomic* atomic );

template<class T> T& setResult(BarzelEvalResult &result, const T &data) {
	result.setBeadData(BarzelBeadAtomic());
	boost::get<BarzelBeadAtomic>(result.getBeadData()).setData(data);
	BarzelBeadAtomic &a = boost::get<BarzelBeadAtomic>(result.getBeadData());
	return boost::get<T>(a.getData());
}

bool setResult(BarzelEvalResult &result, bool data);

typedef boost::function<bool(const BarzerEntity&, uint32_t, uint32_t, uint32_t)> ELCheckFn;

inline static bool checkClass(const BarzerEntity& ent, uint32_t cl, uint32_t scl = 0, uint32_t id = 0)
    { return ent.eclass.ec == (uint32_t)cl; }
inline static bool checkSC(const BarzerEntity& ent, uint32_t cl, uint32_t scl, uint32_t id = 0)
    { return ent.eclass.subclass == (uint32_t)scl && checkClass(ent, cl); }
inline static bool checkId(const BarzerEntity& ent, uint32_t cl, uint32_t scl, uint32_t id)
    { return ent.tokId == id && checkSC(ent, cl, scl); }
} // namespace funcHolder

} // namespace barzer
