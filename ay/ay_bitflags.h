#ifndef AY_BITFLAGS_H
#define AY_BITFLAGS_H
#include <ay_headers.h>
#include <ay_debug.h>
#include <stdlib.h>
#include <iostream>
#include <stdint.h>

namespace ay {
// bitflags works around a nasty limitation of bitset which seems to alsways be 
// at least 4 bytes . so 7 bit bitset is 4 bytes long ... 
// works for bit sequences up to 256 bit long
// this should be used for simple collections of boolean flags

template <uint8_t SZ>
class bitflags {
	uint8_t buf[ (SZ-1)/8+1 ];
public:
	uint8_t getSz() const { return SZ; }
	const uint8_t * getBuf() const { return buf; }
	uint8_t getBufSz() const { return SZ; }

	bitflags() { memset( buf,0,sizeof(buf) ); }
	inline void unset( uint8_t bit )
		{ buf[ bit>>3 ] &= ~(1 << ( bit & 0x7 ) ); }
	inline void flip( )
		{ 
			for( uint8_t* b = buf; b != buf+sizeof(buf); ++b ) 
				*b = ~*b;
		}

	inline void set( uint8_t bit )
		{ buf[ bit>>3 ] |= (1 << ( bit & 0x7 ) ); }

	/// set bit-th bit
	inline void set( uint8_t bit, bool v )
	{
			if( v==true )
				buf[ bit>>3 ] |= (1 << ( bit & 0x7 ) );
			else 
				buf[ bit>>3 ] &= ~(1 << ( bit & 0x7 ) );
	}
	/// 0 all bits
	void reset() { memset( buf,0,sizeof(buf) ); }
	void clear() { reset(); }
	/// 1 all bits
	void set() { memset( buf,0xffffffff,sizeof(buf) ); }

	/// get bit-th bit value
	bool operator[]( uint8_t bit ) const
		{ return ( (buf[ bit>>3 ] & (1 << ( bit & 0x7 ))) ); }
    
    /// call this it's safer
    bool checkBit( uint8_t bit ) const
        {
            size_t b = ( bit>>3 );
            return( b< sizeof(buf)? ((buf[b] & (1 << ( bit & 0x7 )))) : false);
        }
	inline void flip( uint8_t bit )
	{
		uint8_t& b = buf[ bit>>3 ];
		uint8_t mask = (1 << ( bit & 0x7 ));

		if( b & mask ) 
			b &= ~mask;
		else 
			b |= mask;
	}
	inline std::ostream& print( std::ostream& fp ) const
	{
		for( uint8_t i = 0; i< SZ; ++i ) 
			fp << ( (*this)[i] ? '1' : '0' );
		return fp;
	}
};

template <uint8_t SZL, uint8_t SZR>
inline bool operator <(const bitflags<SZL>& l,const bitflags<SZR>& r ) 
{ return ( memcmp( l.getBuf(), r.getBuf(), std::min(l.getBufSz(),r.getBufSz()) ) < 0); }

template <uint8_t SZ>
inline std::ostream& operator <<( std::ostream& fp , const bitflags<SZ>& bs ) 
{ return bs.print( fp ); }

} // namespace ay 
#endif // AY_BITFLAGS_H
