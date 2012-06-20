#pragma once

#include <vector>
#include <string>
#include <map>
#include "ay_trie.h"
#include "ay_utf8.h"

namespace ay
{
	typedef std::vector<std::string> StringList_t;
	class NGramModel
	{
		const size_t m_gramSize;

		typedef trie<ay::CharUTF8, uint32_t> UTF8Trie_t;
		typedef std::map<int, UTF8Trie_t> TriesDict_t;
		TriesDict_t m_tries;

		std::map<int, uint64_t> m_totalSize;
	public:
		NGramModel(size_t size = 3);

		void dump();

		void learn(const StringList_t& text, int topic);
		void getProbs(const std::string& word, std::map<int, double>& probs) const;
	private:
		void addWord(const std::string& word, int topic);
	};
}
