#include <barzer_el_function_holder.h>
#include <barzer_universe.h>

namespace barzer {

namespace funcHolder {
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
    void pushFuncError( BarzelEvalContext& ctxt, const char* funcName, const char* error, const char* sig)
    {
        std::stringstream ss;
        ss << "<funcerr func=\"" << ( funcName ? funcName: "" ) << "\">" << ( error ? error: "" ) ;
        if( sig ) {
            ss << "<sig>" << sig << "</sig>";
        }
        ss << "</funcerr>";

        ctxt.getBarz().barzelTrace.pushError( ss.str().c_str() );
    }
    
    void pushFuncError( BarzelEvalContext& ctxt, const char* funcName, boost::basic_format<char>& error, const char* sig)
    {
      pushFuncError(ctxt, funcName, error.str().c_str(), sig);
    }
// some utility stuff


// extracts a string from BarzerString or BarzerLiteral
namespace {
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
struct DateGetter_vis : public boost::static_visitor<const BarzerDate*>  {
    template <typename T>
    const BarzerDate* operator()( const T& d ) const { return 0; }

    const BarzerDate* operator()( const BarzerDate& d )     const { return &d; }
    const BarzerDate* operator()( const BarzerDateTime& d ) const { return &(d.date); }
    const BarzerDate* operator()( const BarzerRange& d )    const {
        const BarzerRange::Date* p = d.getDate();
        return ( p ? &(p->first) : 0);
    }
    const BarzerDate* operator()( const BarzerERC& d ) const { return (*this)( d.getRange() ); }
};
} // anon namespace

const char* extractString(const GlobalPools &u, const BarzelEvalResult &result)
{
	return boost::apply_visitor(StringExtractor(u), result.getBeadData());
}

void getString(std::string& str, const BarzelEvalResult &r, const StoredUniverse& u)
{

    if( const BarzerNumber *x = getAtomicPtr<BarzerNumber>(r) ) {
        x->toString( str );
    } else if( const BarzerLiteral* x = getAtomicPtr<BarzerLiteral>(r) ) {
       if( const char* y = u.getGlobalPools().string_resolve(x->getId()) )
        str.assign( y );
    } else if( const BarzerString* x = getAtomicPtr<BarzerString>(r) ) {
        str = x->getStr();
    }
}
const BarzerLiteral* getRvecLiteral( const BarzelEvalResultVec& rvec, size_t i )
{ return ( i < rvec.size() ? getAtomicPtr<BarzerLiteral>( rvec[i] ) : 0 ) ; }

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
const BarzerDate* getBarzerDateFromAtomic( const BarzelBeadAtomic* atomic )
{
    return boost::apply_visitor(DateGetter_vis(), atomic->getData());
}

bool setResult(BarzelEvalResult &result, bool data) {
    if (data) {
        setResult(result, BarzerNumber(1));
    } else {
        result.setBeadData(BarzelBeadBlank());
    }
    return data;
}

} // namespace funcHolder
using namespace  funcHolder;

void BELFunctionStorage_holder::addFun(const char *fname, const char *descr, Func fun) {
    const uint32_t fid = gpools.internString_internal(fname);
    addFun(fid, fname, descr, fun);
}

const char* BELFunctionStorage_holder::literalToCString( const BarzerLiteral& ltrl ) const
{
    return(
        ltrl.isInternalString() ? 
        gpools.internalString_resolve(ltrl.getId()) :
        gpools.string_resolve(ltrl.getId())
    );
}
const char* BELFunctionStorage_holder::getCharPtr( const BarzelEvalResultVec& rvec, size_t i ) const
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
FuncMap* BELFunctionStorage_holder::funmap = 0;
void BELFunctionStorage_holder::addFun(const uint32_t fid, const char* fname, const char* descr, Func fun)
{
    if( !funmap )
        funmap = new FuncMap;

    FuncInfo funcInfo( fname,descr );
    FuncData fdata( fun, funcInfo );
    auto x = funmap->insert(
        std::make_pair(
            fid, 
            fdata
        )
    );
    if( !x.second ) {
        std::cerr << fname << " already exists.. aborting execution\n";
    }
}
const StoredEntity* BELFunctionStorage_holder::fetchEntity(uint32_t tokId, const uint16_t cl, const uint16_t scl) const
{
    const StoredEntityUniqId euid(tokId, cl, scl);
    return gpools.dtaIdx.getEntByEuid(euid);
}
void BELFunctionStorage_holder::addAllFunctions( std::pair< const DeclInfo*, const DeclInfo*> range )
{
    for( const DeclInfo* i= range.first; i < range.second; ++i )
        addFun( *i );
}

BELFunctionStorage_holder::BELFunctionStorage_holder(GlobalPools &u) : gpools(u) 
{ }
    

}
