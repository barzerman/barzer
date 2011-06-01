#ifndef AY_DEBUG_H
#define AY_DEBUG_H

#include <iostream>
#define AYDEBUG(x) ( std::cerr << __FILE__ ":" << std::dec << __LINE__ << "(" <<__func__ <<")" << ": " #x << "= " << (x) << '\n' )
#define AYPRINT(x) ( std::cerr << __FILE__ ":" << __LINE__ << "(" <<__func__ <<")" << ": " #x << "= " << (x) << '\n' )

#define AYTRACE(x) ( std::cerr << __FILE__ ":" << __LINE__ << "(" <<__func__ <<")" << ": " #x  "\n" )
namespace ay {
}
#endif // AY_DEBUG_H
