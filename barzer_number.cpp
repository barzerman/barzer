
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#include <barzer_number.h>
#include <string.h>
#include <barzer_parse_types.h>

namespace barzer
{

void BarzerNumber::convert( BarzerString& str ) const
{
    std::stringstream sstr;
    print( sstr );
    str.setStr( sstr.str() );
}


BarzerNumber operator+ (const BarzerNumber& b1, const BarzerNumber& b2)
{
	BarzerNumber res = b1;
	res += b2;
	return res;
}

BarzerNumber operator- (const BarzerNumber& b1, const BarzerNumber& b2)
{
	BarzerNumber res = b1;
	res -= b2;
	return res;
}

BarzerNumber operator* (const BarzerNumber& b1, const BarzerNumber& b2)
{
	BarzerNumber res = b1;
	res *= b2;
	return res;
}

BarzerNumber operator/ (const BarzerNumber& b1, const BarzerNumber& b2)
{
	BarzerNumber res = b1;
	res /= b2;
	return res;
}

std::ostream& operator <<( std::ostream& fp, const BarzerNumber& n )
{ return n.print(fp); }
	std::ostream& BarzerNumber::print( std::ostream& fp ) const
	{
		if( isInt() )
			return fp << n.i ;
		else  if( isReal() )
			return fp << std::fixed << n.real ;
		else
			return fp << "NaN";
	}
	bool BarzerNumber::isEqual( const BarzerNumber& r ) const
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
    BarzerNumber& BarzerNumber::set( const char* s )
    {
        if( strchr(s,'.') )  
            set(atof(s)) ;
        else 
            set(atoi(s)) ;
        return *this;
    }
	BarzerNumber& BarzerNumber::operator+= (const BarzerNumber& num)
	{
		d_asciiLen = 0;
		d_stringId = 0xffffffff;

		if (isNan() || num.isNan())
			clear();
		else if (isInt() && num.isInt())
			set (getInt_unsafe() + num.getInt_unsafe());
		else
			set (getRealWiden() + num.getRealWiden());
		return *this;
	}

	BarzerNumber& BarzerNumber::operator-= (const BarzerNumber& num)
	{
		*this += (num * BarzerNumber(-1));
		return *this;
	}

	BarzerNumber& BarzerNumber::operator*= (const BarzerNumber& num)
	{
		d_asciiLen = 0;
		d_stringId = 0xffffffff;

		if (isNan() || num.isNan())
			clear();
		else if (isInt() && num.isInt())
			set (getInt_unsafe() * num.getInt_unsafe());
		else
			set (getRealWiden() * num.getRealWiden());
		return *this;
	}

	BarzerNumber& BarzerNumber::operator/= (const BarzerNumber& num)
	{
		d_asciiLen = 0;
		d_stringId = 0xffffffff;

		if (isNan() || num.isNan())
			clear();
		else
			set (getRealWiden() / num.getRealWiden());
		return *this;
	}
}
