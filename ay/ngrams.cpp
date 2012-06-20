#include "ay_ngrams.cpp"
#include <fstream>

int main()
{
	std::vector<std::string> strings;
	std::ifstream istr("ngrams_sample/spanish.txt");
	while(istr)
	{
		std::string str;
		istr >> str;
		strings.push_back(str);
	}

	ay::NGramModel model;
	//model.learn({ "shit", "sheet", "heat", "neat", "leet", "beat" }, 0);
	model.learn(strings, 0);
	model.dump();
}