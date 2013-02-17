/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#pragma once
#include <iostream>
#define AYDEBUG(x) ( std::cerr << __FILE__ ":" << std::dec << __LINE__ << "(" <<__func__ <<")" << ": " #x << "= " << (x) << '\n' )
#define AYPRINT(x) ( std::cerr << __FILE__ ":" << __LINE__ << "(" <<__func__ <<")" << ": " #x << "= " << (x) << '\n' )

#define AYTRACE(x) ( std::cerr << __FILE__ ":" << __LINE__ << "(" <<__func__ <<")" << ": " #x  "\n" )
namespace ay {
}
