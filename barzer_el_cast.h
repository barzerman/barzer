#pragma once

#include <barzer_el_chain.h>
#include <barzer_universe.h>
namespace barzer {

struct BarzerAtomicCast {
    const StoredUniverse*   d_universe;
    
    typedef enum : uint8_t { 
        CASTERR_OK, // no error
        // warnings
        CASTERR_CAST_TO_SMALLER, // casting to a smaller type
        CASTERR_INVALID_NUM_CHARS, // 
        // errors 
        CASTERR_FAIL,          // generally failed cast (just check return error value to be >= CASTERR_FAIL)
        CASTERR_UNIMPLEMENTED, // this particular cast unimplemented yet
        CASTERR_NONUMBER, // value cant be cast to a number
        CASTERR_UNIVERSE  // universe missing
    } Err_t;
    
    BarzerAtomicCast( const StoredUniverse* u=0 ) : 
        d_universe(u)
    {}

    static bool conversionSucceeded( Err_t e ) { return e< CASTERR_FAIL; }
    static bool conversionWarning( Err_t e ) { return ( e>CASTERR_OK && e< CASTERR_FAIL ); }
    static bool conversionError( Err_t e ) { return ( e>CASTERR_OK && e< CASTERR_FAIL ); }

    static const char* getErrTxt( Err_t e );

    template <typename T, typename V> Err_t convert( T& d, const V& s ) const { return CASTERR_UNIMPLEMENTED; }

    template <typename V> Err_t convert( BarzerNumber& n , const V& s ) const { return CASTERR_NONUMBER; }

    Err_t convert( BarzerNumber& n , const char* str, size_t str_len ) const;

    Err_t convert( BarzerNumber& n, const BarzerString& s)  const 
        { return convert( n,s.c_str(), s.length() ); }
    Err_t convert( BarzerNumber& , const BarzerLiteral& )  const ;
    Err_t convert( BarzerNumber& n, const BarzelBeadAtomic_var& v )  const;
};

} // namespace barzer
