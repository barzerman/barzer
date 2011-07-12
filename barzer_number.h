#ifndef BARZER_NUMBER_H
#define BARZER_NUMBER_H

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
		int i;
		double  real;
	} n;

	uint8_t type;
	// if known length of original ascii string representation 
	// this CAN BE NULL
	uint8_t d_asciiLen;
public:

	void 	setAsciiLen( uint8_t l ) { d_asciiLen = l ; } 
	uint8_t getAsciiLen( ) const { return d_asciiLen; } 

	BarzerNumber() 				: type(NTYPE_NAN), d_asciiLen(0) {n.i = 0;}
	BarzerNumber( int i ) 		: type(NTYPE_INT), d_asciiLen(0) {n.i = i;}
	BarzerNumber( double i ) 	: type(NTYPE_REAL), d_asciiLen(0) {n.real = i;}

	void set( int i ) { type= NTYPE_INT; n.i = i; }
	void set( double x ) { type= NTYPE_REAL; n.real = x; }

	void clear() { type = NTYPE_NAN; }
	int setInt( const char* s) { return( type= NTYPE_INT, n.i = atoi(s)); }
	int setReal( const char* s) { return( type= NTYPE_REAL, n.i = atof(s)); }

	inline bool isNan() const { return type == NTYPE_NAN; }
	inline bool isInt() const { return type == NTYPE_INT; }
	inline bool isReal() const { return type == NTYPE_REAL; }

	inline int 		getInt_unsafe() const { return( n.i  ); }
	inline double 	getReal_unsafe() const { return( n.real  ); }

	inline uint32_t 	getUint32() const { return( isInt() ? (uint32_t)(n.i) : 0xffffffff ); }

	inline int 		getInt() const { return( isInt() ? n.i : 0 ); }
	inline double 	getReal() const { return( isReal() ? n.real : 0. ); }
	
	inline bool isInt_Nonnegative() const { return ( isInt() && n.i >= 0 );  }
	inline bool isReal_Nonnegative() const { return ( isReal() && n.i >= 0. );  }
	inline bool is_Nonnegative() const { 
		return ( isInt() ?  n.i>=0 : ( isReal() ? n.real>=0. : false ) ); }

	inline bool isInt_Positive() const { return ( isInt() && n.i > 0 );  }
	inline bool isReal_Positive() const { return ( isReal() && n.i >0. );  }
	inline bool is_Positive() const { 
		return ( isInt() ?  n.i>0 : ( isReal() ? n.real>0. : false ) ); }

	inline bool isInt_inRange( int lo, int hi ) const
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
			return fp << n.real ;
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
};

inline bool operator == ( const BarzerNumber& l, const BarzerNumber& r ) 
	{ return l.isEqual(r); }

inline std::ostream& operator <<( std::ostream& fp, const BarzerNumber& n )
{ return n.print(fp); }

} // barzer namespace 
#endif //  BARZER_NUMBER_H