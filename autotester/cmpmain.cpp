#include "autotester.h"
#include <fstream>

int main()
{
	std::ifstream istr("sample.xml");
	
	barzer::autotester::Autotester tst(istr);
}