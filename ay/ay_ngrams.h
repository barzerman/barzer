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
		UTF8Trie_t m_trie;
		
		uint64_t m_totalSize;
	public:
		NGramModel(size_t size = 3);

		void addWord(const char *word);
		void addWords(const StringList_t& text);

		double getProb(const char *word) const;

		void dump();
	};

	class TopicModelMgr
	{
		const size_t m_gramSize;

		typedef std::map<int, NGramModel> ModelsDict_t;
		ModelsDict_t m_models;
	public:
		TopicModelMgr(size_t size = 3);

		NGramModel& getModel(int topic);
	};
}
