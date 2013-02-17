
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#pragma once

#include <cmath>
namespace ay {

template <typename T> 
inline bool epsilon_equals( const T x, const T y, double epsilon ) 
    { return ( 2*fabs(fabs(x)-fabs(y))  < epsilon*(fabs(x)+fabs(y)) ); }

}
