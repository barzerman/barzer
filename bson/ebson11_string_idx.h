#pragma 

/// implements an incrementable string representation of 20 digit decimals (max int 64 is 18,446,744,073,709,551,615)
/// the intended use is this: you start with an index and increment it by 1
/// 32bit unsigned is practically enough 
#include <stdint.h>
#include <stdio.h>
namespace ebson11 {

class StrRepDecimal {
    char d_buf[ 21 ];
    char* d_first;
public:
    StrRepDecimal() : 
        d_first(d_buf+sizeof(d_buf)-2)
        { 
            *d_first= '0';
            d_first[1]=0;
        }

    void slowInit( uint32_t i ) { snprintf( d_buf, sizeof(d_buf), "%u", i ); }
    inline void increment() {
        for( char* p = (d_buf+sizeof(d_buf)-2); p >=d_first; --p ) {
            if( *p>= '9' )
                *p= '0';
            else {
                ++(*p);
                return;
            }
        }
        if( d_first > d_buf )
            *(--d_first) = '1';
    }

    const char* c_str() const {  return d_first; }
};

}
