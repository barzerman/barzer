#include <barzer_el_cast.h>

namespace barzer {

namespace {

} // end of anon namespace 

const char* BarzerAtomicCast::getErrTxt( Err_t e )
{
    switch(e){
    case CASTERR_OK: return "no error"; 
    case CASTERR_CAST_TO_SMALLER: return "casting to smaller type"; 
    case CASTERR_FAIL: return "cast failed";
    case CASTERR_UNIMPLEMENTED: return "cast unimplemented";
    case CASTERR_NONUMBER: return "cannot cast to number";
    default: return "Unknown";
    }
}


BarzerAtomicCast::Err_t BarzerAtomicCast::convert( BarzerNumber& n , const char* str, size_t str_len ) const
{
    if( !str_len)
        return( n=BarzerNumber( (int)(0) ), CASTERR_OK );
    bool isNegative=false;
    const char* s = ( str[0]=='-' ? (isNegative=true, (str+1) ) : str );
    for( const char* c = s, *end_s= str+str_len; c< end_s; ++c ) {
        if( !isdigit(*c) ) {
            n.setReal(s);
            if( isNegative )
                n.negate();
            return CASTERR_OK;
        }
    }
    /// if we're here this means we havent seen any non digits past the first - 
    n.setInt(s);
    if( isNegative )
        n.negate();
        
    return CASTERR_OK;
}

BarzerAtomicCast::Err_t BarzerAtomicCast::convert( BarzerNumber& n , const BarzerLiteral& l )  const 
{
    if( !d_universe )
        return CASTERR_UNIVERSE;
    const char* s = d_universe->resolveLiteral(l);
    if( s ) 
        return convert( n, s, strlen(s) ) ;
    return ( n.setInt((int)0), CASTERR_OK );
}

BarzerAtomicCast::Err_t BarzerAtomicCast::convert( BarzerNumber& n, const BarzelBeadAtomic_var& v )  const
{
    struct VarVis : public boost::static_visitor<BarzerAtomicCast::Err_t> {
     const BarzerAtomicCast& caster;
     BarzerNumber& dest;
     VarVis( BarzerNumber& n, const BarzerAtomicCast& c ) : dest(n), caster(c) {}

     template <typename V>
     BarzerAtomicCast::Err_t operator()( const V& v ) { return caster.convert(dest, v );          }
    };
    VarVis vis( n, *this );
    return boost::apply_visitor( vis, v );
}
} // namespace barzer 
