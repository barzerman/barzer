#include <iostream>
#include <iterator>
#include <algorithm>
#include "ay_utf8.h"

using namespace ay;

void print (CharUTF8 t)
{
	std::cout << t.getBuf ();
}

int main ()
{
	StrUTF8 lat ("хe");
	lat.swap (lat.begin () + 1, lat.begin ());
	StrUTF8 str ("$йу́хgbзд́а́€");
	StrUTF8 str2 ("йу́х");
	StrUTF8 str3 ("g€b");

	std::cout << lat.c_str () << std::endl;
	//std::for_each (str2.rbegin (), str2.rend (), print);

	/*
	str.print (std::cout);
	std::cout << std::endl;

	str.erase (str.begin () + 1, str.begin () + 4);
	str.print (std::cout);
	std::cout << std::endl;
	*/
}

