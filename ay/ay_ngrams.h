#pragma once

#include <vector>
#include <string>
#include <map>
#include <boost/unordered_map.hpp>
#include "ay_trie.h"
#include "ay_utf8.h"

namespace ay
{
	typedef std::vector<std::string> StringList_t;

	namespace ASCII
	{
		class NGramModel
		{
			const size_t m_gramSize;

			typedef boost::unordered_map<uint32_t, uint64_t> EncDict_t;
			EncDict_t m_encounters;
			uint64_t m_totalSize;
		public:
			NGramModel(size_t size = 3);
			
			void addWord(const char *word);
			void addWords(const StringList_t& text);

			double getProb(const char *word) const;
		};
	}

	namespace UTF8
	{
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
	}

	template<typename Model>
	class BasicTopicModelMgr
	{
		const size_t m_gramSize;

		typedef std::map<int, Model> ModelsDict_t;
		ModelsDict_t m_models;
	public:
		inline BasicTopicModelMgr(size_t size = 3)
		: m_gramSize(size)
		{
		}

		inline Model& getModel(int topic)
		{
			typename ModelsDict_t::iterator pos = m_models.find(topic);
			if (pos == m_models.end())
				pos = m_models.insert(std::make_pair(topic, Model(m_gramSize))).first;
			return pos->second;
		}

		inline void getAvailableTopics(std::vector<int>& result) const
		{
			for (typename ModelsDict_t::const_iterator i = m_models.begin(), end = m_models.end(); i != end; ++i)
				result.push_back(i->first);
		}
	};

	typedef BasicTopicModelMgr<UTF8::NGramModel> TopicModelMgr;
}
