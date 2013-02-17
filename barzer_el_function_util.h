/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#pragma once
#include <ay/ay_util_char.h>
namespace barzer {
class BarzerNumber;
class BarzelEvalResult;
namespace strutil {

// text to number errors
enum {
    T2NERR_OK=0,
    T2NERR_JUNK, // non numeral words encountered
    T2NERR_LANG, // unsupported language 
    T2NERR_TOOCOMPLICATED // unsupported configuration (maybe there are too many tokens etc)
};
int text_to_number( BarzerNumber& , const ay::char_cp_vec&, int langId=0 /*language id*/) ;
} // namespace strutil 

bool operator== ( const BarzelEvalResult& l, class BarzelEvalResult& r);
bool operator< ( const BarzelEvalResult& l, class BarzelEvalResult& r);
inline bool operator> ( const BarzelEvalResult& l, class BarzelEvalResult& r)
{
    return !( (l<r) || (l==r) );
}
inline bool operator<= ( const BarzelEvalResult& l, class BarzelEvalResult& r)
    { return( l< r || l==r ); }
inline bool operator>= ( const BarzelEvalResult& l, class BarzelEvalResult& r)
    { return( l> r || l==r ); }

} // barzer namespace
