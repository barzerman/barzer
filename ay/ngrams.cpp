#include "ay_ngrams.cpp"
#include "ay_utf8.cpp"
#include <fstream>
#include <sys/time.h>

inline long getDiff(timeval prev, timeval now)
{
	return 1000000 * (now.tv_sec - prev.tv_sec) + now.tv_usec - prev.tv_usec;
}

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

template<typename ModelMgr>
void runTests (const std::vector<std::string>& words, ModelMgr& mgr)
{
	std::vector<int> topics;
	mgr.getAvailableTopics(topics);

	timeval prev, now;
	gettimeofday(&prev, 0);
	
	const int rep = 10;
	for (int k = 0; k < rep; ++k)
	{
		for (size_t j = 0; j < topics.size(); ++j)
		{
			typename ModelMgr::ModelType_t& model = mgr.getModel(topics[j]);
			for (size_t i = 0; i < words.size(); ++i)
			{
				const std::string& word = words.at(i);
				volatile double spProb = model.getProb(word.c_str());
			}
		}
	}
	gettimeofday(&now, 0);
	std::cout << "guessed " << words.size ()
			<< " words in " << getDiff(prev, now) / static_cast<double> (rep) << " μs, "
			<< static_cast<double> (getDiff(prev, now)) / words.size() / topics.size() / rep << " μs per word" << std::endl;
}

int main()
{
	ay::BasicTopicModelMgr<ay::UTF8::NGramModel> mgr;
	//ay::BasicTopicModelMgr<ay::ASCII::NGramModel> mgr;
	timeval prev, now;
	gettimeofday(&prev, 0);
	mgr.getModel(0).addWords(loadFile("ngrams_sample/english.txt"));
	mgr.getModel(1).addWords(loadFile("ngrams_sample/french.txt"));
	mgr.getModel(2).addWords(loadFile("ngrams_sample/spanish.txt"));
	gettimeofday(&now, 0);
	std::cout << "initial learning took " << getDiff(prev, now) << " μs" << std::endl;


	runTests(loadFile("ngrams_sample/spanish.txt"), mgr);
	runTests(loadFile("ngrams_sample/english.txt"), mgr);
	runTests(loadFile("ngrams_sample/french.txt"), mgr);

	
	std::vector<int> topics;
	mgr.getAvailableTopics(topics);

	std::string str;
	while (true)
	{
		std::cout << "Enter string:" << std::endl;
		std::getline(std::cin, str);
		if (str.empty())
			break;

		for (size_t i = 0; i < topics.size(); ++i)
		{
			ay::UTF8::NGramModel& model = mgr.getModel(topics[i]);
			std::cout << "score for topic " << topics[i] << ": " << model.getProb(str.c_str()) << std::endl;
		}

		std::cout << std::endl;
	}
	
}
