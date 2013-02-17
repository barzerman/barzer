
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#pragma once
namespace ay {

namespace utf8 {
inline bool isRussianString( const char* s )
{
	for( const uint8_t* c = (const uint8_t*)s; *c; c+=2 ) {
		if( (*c != 0xd1 && *c != 0xd0) || !*c ) return false;
	}
	return true;
}
} // namespace utf8

}
