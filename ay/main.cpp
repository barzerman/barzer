#include <iostream>
#include <iterator>
#include <algorithm>
#include <assert.h>
#include "ay_utf8.cpp"
#include "ay_stackvec.h"

using namespace ay;

template<typename T>
void dump(const T& t)
{
	std::cout << "Size: " << t.size() << "; elems = ";
	for (size_t i = 0; i < t.size(); ++i)
		std::cout << t[i] << "; ";
	std::cout << std::endl;
}

int main ()
{
	/*
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

    std::vector<uint32_t> utf32str;
	str.toUTF32( utf32str );
	nstr.setUTF32( utf32str );

	assert(str == nstr);
	
	str3.toUpper();
	std::cout << str3.c_str() << std::endl;
	*/
	StackVec<uint32_t> vec;
	vec.push_back(1);
	vec.push_back(2);
	vec.push_back(3);
	dump(vec);
	dump(StackVec<uint32_t>(vec));
	
	vec.push_back(4);
	vec.push_back(5);
	vec.push_back(6);
	vec.push_back(7);
	dump(vec);
	dump(StackVec<uint32_t>(vec));
	
	vec.push_back(8);
	dump(vec);
	dump(StackVec<uint32_t>(vec));
	
	vec.pop_back();
	vec.pop_back();
	dump(vec);
}
