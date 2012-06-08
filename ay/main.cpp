#include <iostream>
#include <iterator>
#include <algorithm>
#include <assert.h>
#include "ay_utf8.cpp"

using namespace ay;

int main ()
{
	StrUTF8 lat ("хe");
	StrUTF8 str ("$йу́хgbзд́а́€");
	StrUTF8 str2 ("йу́х");
	StrUTF8 str3 ("g€b");

	CharUTF8 b1("$");
	CharUTF8 b2("¢");
	CharUTF8 b3("€");
	assert(b1 == CharUTF8::fromUTF32(b1.toUTF32()));
	assert(b2 == CharUTF8::fromUTF32(b2.toUTF32()));
	assert(b3 == CharUTF8::fromUTF32(b3.toUTF32()));

	StrUTF8 nstr;
	nstr.setUTF32(str.toUTF32());
	assert(str == nstr);
	
	str3.toUpper();
	std::cout << str3.c_str() << std::endl;
}

