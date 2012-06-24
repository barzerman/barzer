#include "ay_ngrams.cpp"
#include "ay_utf8.cpp"
#include <fstream>

std::vector<std::string> loadFile(const std::string& filename)
{
	std::vector<std::string> strings;
	std::ifstream istr(filename);
	while(istr)
	{
		std::string str;
		istr >> str;
		strings.push_back(str);
	}
	return strings;
}

int main()
{
	//model.addWords({ "shit", "sheet", "heat", "neat", "leet", "beat" });

	ay::TopicModelMgr mgr;
	mgr.getModel(0).addWords(loadFile("ngrams_sample/spanish.txt"));
	mgr.getModel(1).addWords(loadFile("ngrams_sample/english.txt"));
	mgr.getModel(2).addWords(loadFile("ngrams_sample/french.txt"));

	std::cout << mgr.getModel(0).getProb("leechcraft") << " " << mgr.getModel(1).getProb("leechcraft") << " " << mgr.getModel(2).getProb("leechcraft") << std::endl;
	std::cout << mgr.getModel(0).getProb("barzer") << " " << mgr.getModel(1).getProb("barzer") << " " << mgr.getModel(2).getProb("barzer") << std::endl;
}