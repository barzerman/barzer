#include "barzer_number.h"

namespace barzer
{
	BarzerNumber& BarzerNumber::operator+= (const BarzerNumber& num)
	{
		d_asciiLen = 0;
		d_stringId = 0;

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
		d_stringId = 0;

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
		d_stringId = 0;

		if (isNan() || num.isNan())
			clear();
		else
			set (getRealWiden() / num.getRealWiden());
		return *this;
	}
}
