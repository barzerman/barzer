#ifndef AY_BITFLAGS_H
#define AY_BITFLAGS_H
#include <ay_headers.h>
#include <ay_debug.h>
#include <stdlib.h>
#include <iostream>
#include <stdint.h>
#include <map>

namespace ay {
// bitflags works around a nasty limitation of bitset which seems to alsways be 
// at least 4 bytes . so 7 bit bitset is 4 bytes long ... 
// works for bit sequences up to 256 bit long
// this should be used for simple collections of boolean flags

template <uint8_t SZ>
class bitflags {
	uint8_t buf[ (SZ-1)/8+1 ];
public:
    enum { NUMOFBITS = SZ } ;
	uint8_t getSz() const { return SZ; }
	uint8_t size() const { return SZ; }
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
    bool check( uint8_t bit ) const { return checkBit(bit); }
    bool checkAnyBit( uint8_t b1, uint8_t b2 ) const { return (checkBit(b1)||checkBit(b2)); }
    bool checkAnyBit( uint8_t b1, uint8_t b2, uint8_t b3 ) const { return (checkBit(b1)||checkBit(b2)||checkBit(b3)); }
    bool checkAnyBit( uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4 ) const { return (checkBit(b1)||checkBit(b2)||checkBit(b3)||checkBit(b4)); }
    bool checkAnyBit( uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4, uint8_t b5 ) const { return (checkBit(b1)||checkBit(b2)||checkBit(b3)||checkBit(b4)||checkBit(b5)); }

    bool checkAllBit( uint8_t b1, uint8_t b2 ) const { return (checkBit(b1)&&checkBit(b2)); }
    bool checkAllBit( uint8_t b1, uint8_t b2, uint8_t b3 ) const { return (checkBit(b1)&&checkBit(b2)&&checkBit(b3)); }
    bool checkAllBit( uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4 ) const { return (checkBit(b1)&&checkBit(b2)&&checkBit(b3)&&checkBit(b4)); }
    bool checkAllBit( uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4, uint8_t b5 ) const { return (checkBit(b1)&&checkBit(b2)&&checkBit(b3)&&checkBit(b4)&&checkBit(b5)); }

    bool checkAnyBit( ) const
    {
        const uint8_t* x = buf;

        if( sizeof(x) == 1 ) {
            return *x;
        } else if( sizeof(x) == 2 ) {
            return x[0] || x[1];
        } else if( sizeof(x) == 3 ) {
            return ( x[0] || x[1] || x[2] );
        } else if( sizeof(x) == 4 ) {
            return ( x[0] || x[1] || x[2] || x[4] );
        } else {
            for( auto i = x, i_end = i+sizeof(x); i< i_end; ++i ) {
                if( *i )
                    return true;
            }
            return false;
        }
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

/// this interface is for slow mnemonic way of working with a particular set of bitflags
/// bits can be "named" and then accessed using the names
/// this is not a memory efficient structure nor is it particularly fast but because 
/// bitmaps are extremely unlikely to have a lot of bits this would work

template <typename Bitflags>
class named_bits {
    typedef std::map< std::string, uint8_t > NameToBitMap;
    typedef std::map< uint8_t, std::string > BitToNameMap;
    NameToBitMap nbmap;
    BitToNameMap bnmap;
    Bitflags& bf;
public:
    named_bits( Bitflags& b ) : bf(b) {}
    Bitflags& theBits() { return bf; }
    const Bitflags& theBits() const { return bf; }

    void name( uint8_t b, const char* n )
    { 
        std::string s(n);
         
        nbmap[ s ] = b; 
        bnmap[ b ] = s;
    }

    std::string getBitName(uint8_t b) const
    {
        auto i = bnmap.find(b);
        return( i == bnmap.end() ? std::string() : i->second );
    }

    void set( const std::string& name, bool val=true )
    {
        auto i = nbmap.find( name );
        if( i != nbmap.end() && i->second < bf.size() ) 
            bf.set( i->second, val );
    }
    bool check( const std::string& name )
    {
        auto i = nbmap.find(name);
        return( i!= nbmap.end() ? bf.checkBit(i->second) : false);
    }
    
    std::ostream& print( std::ostream& os ) const
    {
        for( auto i = bnmap.begin() ; i!= bnmap.end(); ++i ) {
            os << static_cast<int>(i->first) << ":" << i->second << ( bf.checkBit(i->first) ? "\tON" : "\tOFF" ) << std::endl;
        }
        return os;
    }
};

template <typename B>
inline std::ostream& operator<< ( std::ostream& os, const named_bits<B>& b )
{
    return b.print( os );
}


} // namespace ay 
#endif // AY_BITFLAGS_H
