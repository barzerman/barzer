#pragma 

/// implements an incrementable string representation of 15 digit decimal integer (999,999,999,999,999)
/// the intended use is this: you start with an index and increment it by 1
/// 32bit unsigned is practically enough 
#include <stdint.h>
#include <stdio.h>
namespace ebson11 {

class StrRepDecimalU32 {
    char d_buf[ 16 ];
    int d_last;
    
public:
    StrRepDecimalU32() : d_last(0) {  d_buf[0] = '0'; d_buf[1] = 0; }

    void slowInit( uint32_t i ) { snprintf( d_buf, sizeof(d_buf), "%u", i ); }
    inline void increment() {
        if( d_buf[ d_last ] < '9' ) {
            ++d_buf[ d_last ];
            return;
        }
            
        for( int last = d_last; last >=0; --last ) {
            if( d_buf[ last ] == '9' )
                d_buf[ last ] = '0';
            else {
                ++d_buf[ last ];
                return;
            }
        }
        if( d_last == sizeof(d_buf) )
            return;
        // if we're here we need to carry one and add a digit
        for( size_t i = d_last+1; i> 0; --i ) 
            d_buf[i]= d_buf[i-1];
        d_buf[0] = '1';
        ++d_last;
        d_buf[ d_last+1 ] =0;
    }

    const char* c_str() const {  return d_buf; }
};

}
