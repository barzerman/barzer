
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#pragma once
#include <limits>
#include <iostream>
#include <stdint.h>
#include <stdlib.h>
namespace barzer {
/// pure numbers
class BarzerNumber {
public:
	enum {
		NTYPE_NAN, //  not a number
		NTYPE_INT,
		NTYPE_REAL
	} ;

private:
	union {
		int64_t i;
		double  real;
	} n;

	uint8_t type;
	// if known length of original ascii string representation
	// this CAN BE NULL
	uint8_t d_asciiLen;

    uint32_t d_stringId; /// in case number's string representation exists
public:

	void 	setAsciiLen( uint8_t l ) { d_asciiLen = l ; }
	uint8_t getAsciiLen( ) const { return d_asciiLen; }

	BarzerNumber() 				: type(NTYPE_NAN), d_asciiLen(0),d_stringId(0xffffffff) {n.i = 0;}
	BarzerNumber( int i ) 		: type(NTYPE_INT), d_asciiLen(0),d_stringId(0xffffffff) {n.i = i;}
	BarzerNumber( uint32_t i ) 		: type(NTYPE_INT), d_asciiLen(0),d_stringId(0xffffffff) {n.i = i;}
	BarzerNumber( uint16_t i ) 		: type(NTYPE_INT), d_asciiLen(0),d_stringId(0xffffffff) {n.i = i;}
	BarzerNumber( uint8_t i ) 		: type(NTYPE_INT), d_asciiLen(0),d_stringId(0xffffffff) {n.i = i;}
	BarzerNumber( int64_t i ) 		: type(NTYPE_INT), d_asciiLen(0),d_stringId(0xffffffff) {n.i = i;}
	BarzerNumber( double i ) 	: type(NTYPE_REAL), d_asciiLen(0),d_stringId(0xffffffff) {n.real = i;}

	void set( int i ) { type= NTYPE_INT; n.i = i; }
	void set( int64_t i ) { type= NTYPE_INT; n.i = i; }
	void set( uint32_t i ) { type= NTYPE_INT; n.i = i; }
	void set( uint16_t i ) { type= NTYPE_INT; n.i = i; }
	void set( uint8_t i ) { type= NTYPE_INT; n.i = i; }
	void set( double x ) { type= NTYPE_REAL; n.real = x; }

	void clear() { type = NTYPE_NAN; }
	int64_t setInt() { return( type= NTYPE_INT, n.i = std::numeric_limits<int64_t>::quiet_NaN() ); }
	int64_t setInt( const char* s) { return( type= NTYPE_INT, n.i = atoi(s)); }
	int64_t setReal( const char* s) { return( type= NTYPE_REAL, n.real = atof(s)); }
	int64_t setReal( ) { return( type= NTYPE_REAL, n.i = std::numeric_limits<double>::quiet_NaN()); }

    void negate() { 
        if( NTYPE_INT == type) 
            n.i=-n.i;
        else if( NTYPE_REAL == type ) 
            n.real=-n.real;
    }
    /// figures out from string and sets 
    BarzerNumber& set( const char* s ); 

	BarzerNumber& operator+= (const BarzerNumber& num);
	BarzerNumber& operator-= (const BarzerNumber& num);
	BarzerNumber& operator*= (const BarzerNumber& num);
	BarzerNumber& operator/= (const BarzerNumber& num);
	operator bool () const { return getRealWiden(); }

	inline bool isNan() const { return type == NTYPE_NAN; }
	inline bool isInt() const { return type == NTYPE_INT; }
    inline bool isIntNaN() const { return ( type == NTYPE_INT && n.i == std::numeric_limits<int64_t>::quiet_NaN()); }
	inline bool isReal() const { return type == NTYPE_REAL; }
    inline bool isRealNaN() const { return ( type == NTYPE_REAL && n.real == std::numeric_limits<double>::quiet_NaN()); }

	inline int64_t 		getInt_unsafe() const { return( n.i  ); }
	inline double 	getReal_unsafe() const { return( n.real  ); }

	inline uint32_t 	getUint32() const { return( isInt() ? (uint32_t)(n.i) : 0xffffffff ); }

	inline int64_t 		getInt() const { return( isInt() ? n.i : n.real ); }
	inline double 	    getReal() const { return( isReal() ? n.real : n.i ); }

	inline double getRealWiden() const { return isReal() ? n.real : (isInt() ? n.i :0); }

	inline bool isInt_Nonnegative() const { return ( isInt() && n.i >= 0 );  }
	inline bool isReal_Nonnegative() const { return ( isReal() && n.i >= 0. );  }
	inline bool is_Nonnegative() const {
		return ( isInt() ?  n.i>=0 : ( isReal() ? n.real>=0. : false ) ); }

	inline bool isInt_Positive() const { return ( isInt() && n.i > 0 );  }
	inline bool isReal_Positive() const { return ( isReal() && n.i >0. );  }
	inline bool is_Positive() const {
		return ( isInt() ?  n.i>0 : ( isReal() ? n.real>0. : false ) ); }

	inline bool isInt_inRange( int64_t lo, int64_t hi ) const
		{ return ( isInt() && n.i> lo && n.i < hi ); }
	inline bool isReal_inRange( double lo, double hi ) const
		{ return ( isReal() && n.real> lo && n.real < hi ); }

	inline bool is_inRange( double lo, double hi ) const
	{
		return ( isReal() ?  (n.real> lo && n.real < hi) : (isInt() ? n.i> lo && n.i < hi: false ) );
	}

	// date specific checks
	inline bool cal_canbe_Month() const { return( isInt() ? (n.i>=1 && n.i<=12): false ) ; }
	inline bool cal_canbe_Quarter() const { return( isInt() ? (n.i>=1 && n.i<=4): false ) ; }
	/// rough check for day of the month
	inline bool cal_canbe_Day() const { return( isInt() ? (n.i>=1 && n.i<=31): false ) ; }
	inline std::ostream& print( std::ostream& fp ) const
	{
		if( isInt() )
			return fp << n.i ;
		else  if( isReal() )
			return fp << std::fixed << n.real ;
		else
			return fp << "NaN";
	}
	bool isEqual( const BarzerNumber& r ) const
	{
		if( type != r.type )
			return false;
		else {
			switch(type) {
			case NTYPE_NAN: return true;
			case NTYPE_INT: return ( n.i == r.n.i );
			case NTYPE_REAL: return ( n.real == r.n.real );
			default: return false; // should never be the case
			}
		}
	}

    void        setStringId( uint32_t stringId ) { d_stringId = stringId; }
    uint32_t    getStringId( ) const { return d_stringId; }
    bool        hasValidStringId() const { return (d_stringId!= 0xffffffff); }
};

inline bool operator == ( const BarzerNumber& l, const BarzerNumber& r )
	{ return l.isEqual(r); }

inline bool operator< (const BarzerNumber& l, const BarzerNumber& r)
{
	return l.getRealWiden() < r.getRealWiden();
}

inline BarzerNumber operator+ (const BarzerNumber& b1, const BarzerNumber& b2)
{
	BarzerNumber res = b1;
	res += b2;
	return res;
}

inline BarzerNumber operator- (const BarzerNumber& b1, const BarzerNumber& b2)
{
	BarzerNumber res = b1;
	res -= b2;
	return res;
}

inline BarzerNumber operator* (const BarzerNumber& b1, const BarzerNumber& b2)
{
	BarzerNumber res = b1;
	res *= b2;
	return res;
}

inline BarzerNumber operator/ (const BarzerNumber& b1, const BarzerNumber& b2)
{
	BarzerNumber res = b1;
	res /= b2;
	return res;
}

inline std::ostream& operator <<( std::ostream& fp, const BarzerNumber& n )
{ return n.print(fp); }

} // barzer namespace

namespace std
{
	template<>
	struct numeric_limits<barzer::BarzerNumber>
	{
	public:
		static const bool is_specialized = true;

		static barzer::BarzerNumber min() throw() { return barzer::BarzerNumber (std::numeric_limits<double>::min()); }
		static barzer::BarzerNumber max() throw() { return barzer::BarzerNumber (std::numeric_limits<double>::max()); }
		static const int digits = std::numeric_limits<double>::digits;
		static const int digits10 = std::numeric_limits<double>::digits;
		static const bool is_signed = std::numeric_limits<double>::is_signed;
		static const bool is_integer = std::numeric_limits<double>::is_integer;
		static const bool is_exact = std::numeric_limits<double>::is_exact;
		static const int radix = std::numeric_limits<double>::radix;
		static barzer::BarzerNumber epsilon() throw() { return std::numeric_limits<double>::epsilon(); }
		static barzer::BarzerNumber round_error() throw() { return std::numeric_limits<double>::round_error(); }

		static const int min_exponent = std::numeric_limits<double>::min_exponent;
		static const int min_exponent10 = std::numeric_limits<double>::min_exponent10;
		static const int max_exponent = std::numeric_limits<double>::max_exponent;
		static const int max_exponent10 = std::numeric_limits<double>::max_exponent10;

		static const bool has_infinity = std::numeric_limits<double>::has_infinity;
		static const bool has_quiet_NaN = std::numeric_limits<double>::has_quiet_NaN;
		static const bool has_signaling_NaN = std::numeric_limits<double>::has_signaling_NaN;
		static const float_denorm_style has_denorm = std::numeric_limits<double>::has_denorm;
		static const bool has_denorm_loss = std::numeric_limits<double>::has_denorm_loss;

		static barzer::BarzerNumber infinity() throw() { return std::numeric_limits<double>::infinity(); }
		static barzer::BarzerNumber quiet_NaN() throw() { return std::numeric_limits<double>::quiet_NaN(); }
		static barzer::BarzerNumber signaling_NaN() throw() { return std::numeric_limits<double>::signaling_NaN(); }
		static barzer::BarzerNumber denorm_min() throw() { return std::numeric_limits<double>::denorm_min(); }

		static const bool is_iec559 = std::numeric_limits<double>::is_iec559;
		static const bool is_bounded = std::numeric_limits<double>::is_bounded;
		static const bool is_modulo = std::numeric_limits<double>::is_modulo;

		static const bool traps = std::numeric_limits<double>::traps;
		static const bool tinyness_before = std::numeric_limits<double>::tinyness_before;
		static const float_round_style round_style = std::numeric_limits<double>::round_style;
	};
} // namespace barzer
