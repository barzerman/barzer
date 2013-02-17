/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#include <barzer_el_function_util.h>
#include <barzer_number.h>
#include <ay/ay_util.h>
#include <barzer_el_chain.h>
#include <barzer_el_rewriter.h>

namespace barzer {

namespace strutil {

namespace {

struct NumeralDta {
    const char* str;
    int num;
};

#define GNDECL(x,n) {#x,n}
/// never reorder this 
NumeralDta g_numeral[] = {
GNDECL(eight, 8),
GNDECL(eighteen, 18),
GNDECL(eigthy, 80),
GNDECL(eleven, 11),
GNDECL(fifteen, 15),
GNDECL(fifty, 50),
GNDECL(five, 5),
GNDECL(forty, 40),
GNDECL(four, 4),
GNDECL(fourteen, 14),
GNDECL(hundred, 100),
GNDECL(nine, 9),
GNDECL(nineteen, 19),
GNDECL(ninety, 90),
GNDECL(one, 1),
GNDECL(seven, 7),
GNDECL(seventeen, 17),
GNDECL(seventy, 70),
GNDECL(six, 6),
GNDECL(sixteen, 16),
GNDECL(sixty, 60),
GNDECL(ten, 10),
GNDECL(thirteen, 13),
GNDECL(thirty, 30),
GNDECL(three, 3),
GNDECL(twelve, 12),
GNDECL(twenty, 20),
GNDECL(two, 2)
};

inline bool operator<( const NumeralDta& l, const NumeralDta& r ) 
{
    return( strcmp( l.str, r.str ) < 0);
}
// returns a negative number 
inline int g_numeral_get( const char* s ) 
{
    NumeralDta chablon ={ s, 0 };
    const NumeralDta* res= std::lower_bound(  g_numeral, ARR_END(g_numeral), chablon );
    
    if( res && res != ARR_END(g_numeral) && !strcmp(res->str,s) ) {
        return (res->num );
    } else {
        return (-1);
    }
}

#undef GNDECL

} // anon namespace ends here 

int text_to_number( BarzerNumber& bn, const ay::char_cp_vec& tok, int langId) 
{
    bn.set(0);
    if( langId ) { // non english 
        /// should call out to lang util 
        return T2NERR_LANG;
    }

    /// english version
    switch( tok.size() ) {
    case 1: {
        int n = g_numeral_get( tok[0] );
        if( n< 0) 
            return T2NERR_JUNK;
        else { 
            bn.set( n );
            return T2NERR_OK;
        }
        break;}
    case 2: {

        int n0 = g_numeral_get( tok[0] );
        int err = T2NERR_OK;
        if( n0 < 0 ) {
            err = T2NERR_JUNK;
            n0 = 0;
        }
        int n1 = g_numeral_get( tok[1] );
        if( n1 < 0 ) {
            if( !err ) 
                err = T2NERR_JUNK;

            n1 = 0;
        }
        bn.set( n1 + n0 );
        break;}
    }
    return T2NERR_OK;
}

} // namespace strutil

namespace {

struct compare_less_vis : public boost::static_visitor<bool> {
    template <typename L, typename R> 
    bool operator()( const L& l, const R& r ) const
        { return false; }
    template <typename T>
    bool operator()( const T& l, const T& r ) const
        { return false; }
/// BarzerNumber
    bool operator()( const BarzerNumber& l, const BarzerNumber& r ) const
        { return l.isNan() || r.isNan()? false: l < r; }
///BarzerDate
    bool operator()( const BarzerDate& l, const BarzerDateTime& r ) const
        { return  l < r.getDate(); }
    bool operator()( const BarzerDate& l, const BarzerDate& r ) const
        { return l < r; }
///BarzerDateTime
    bool operator()( const BarzerDateTime& l, const BarzerDate& r ) const
        { return  l.getDate() < r; }       
    bool operator()( const BarzerDateTime& l, const BarzerTimeOfDay& r ) const
        { return  l.getTime() < r; }
    bool operator()( const BarzerDateTime& l, const BarzerDateTime& r ) const
        { return  l < r; }        
///BarzerTime
    bool operator()( const BarzerTimeOfDay& l, const BarzerDateTime& r ) const
        { return  l < r.getTime(); }
    bool operator()( const BarzerTimeOfDay& l, const BarzerTimeOfDay& r ) const
        { return l < r; }
///BarzerString
    bool operator()( const BarzerString& l, const BarzerString& r ) const
        { return  l.getStr() < r.getStr(); }       
};
struct compare_eq_vis : public boost::static_visitor<bool> {
    template <typename L, typename R> 
    bool operator()( const L& l, const R& r ) const
        { return false; }
    template <typename T>
    bool operator()( const T& l, const T& r ) const
        { return false; }

/// BarzerNumber
    bool operator()( const BarzerNumber& l, const BarzerNumber& r ) const
        { return l.isNan() || r.isNan()? false: l == r; }
///BarzerDate
    bool operator()( const BarzerDate& l, const BarzerDateTime& r ) const
        { return  l == r.getDate(); }
    bool operator()( const BarzerDate& l, const BarzerDate& r ) const
        { return l == r; }
///BarzerDateTime
    bool operator()( const BarzerDateTime& l, const BarzerDate& r ) const
        { return  l.getDate() == r; }       
    bool operator()( const BarzerDateTime& l, const BarzerTimeOfDay& r ) const
        { return  l.getTime() == r; }
    bool operator()( const BarzerDateTime& l, const BarzerDateTime& r ) const
        { return  l == r; }        
///BarzerTime
    bool operator()( const BarzerTimeOfDay& l, const BarzerDateTime& r ) const
        { return  l == r.getTime(); }
    bool operator()( const BarzerTimeOfDay& l, const BarzerTimeOfDay& r ) const
        { return l == r; }
///BarzerString
    bool operator()( const BarzerString& l, const BarzerString& r ) const
        { return  l.getStr() == r.getStr(); } 
///BarzerLiteral
    bool operator()( const BarzerLiteral& l, const BarzerLiteral& r ) const
        { return  l == r; }        
};

}
bool operator< ( const BarzelEvalResult& l, class BarzelEvalResult& r)
{
    const BarzelBeadAtomic* lAtomic = l.getSingleAtomic();
    if( !lAtomic ) 
        return false;
    const BarzelBeadAtomic* rAtomic = r.getSingleAtomic();
    if( !rAtomic ) 
        return false;

    return boost::apply_visitor( compare_less_vis(), lAtomic->getData(), rAtomic->getData() );
}


bool operator== ( const BarzelEvalResult& l, class BarzelEvalResult& r)
{
    const BarzelBeadAtomic* lAtomic = l.getSingleAtomic();
    if( !lAtomic ) 
        return false;
    const BarzelBeadAtomic* rAtomic = r.getSingleAtomic();
    if( !rAtomic ) 
        return false;

    return boost::apply_visitor( compare_eq_vis(), lAtomic->getData(), rAtomic->getData() );
}

}
