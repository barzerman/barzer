#include "ay_ngrams.h"
#include <iostream>

namespace ay
{
	NGramModel::NGramModel(size_t size)
	: m_gramSize(size)
	{
	}

	namespace
	{
		struct CallbackTrieDump
		{
			template<typename T>
			trie_visitor_continue_t operator()(const T& path) const
			{
				if (path.size() != 3)
					return TRIE_VISITOR_CONTINUE;

				for (typename T::const_iterator i = path.begin(); i != path.end(); ++i)
					std::cout << (*i)->first.c_str();
				std::cerr << " " << path.back()->second.data() << std::endl;
				return TRIE_VISITOR_CONTINUE;
			}
		};
	}

	void NGramModel::dump()
	{
		CallbackTrieDump cb;
		for (TriesDict_t::iterator i = m_tries.begin(), end = m_tries.end(); i != end; ++i)
		{
			std::cout << "Dump for topic " << i->first << ":" << std::endl;
			trie_visitor<UTF8Trie_t, CallbackTrieDump> vis(cb);
			vis.visit(i->second);
			std::cout << std::endl;
		}
	}

	void NGramModel::learn(const StringList_t& text, int topic)
	{
		uint64_t sum = 0;
		for (size_t i = 0, size = text.size(); i < size; ++i)
		{
			const std::string& word = text.at(i);
			if (word.size() >= m_gramSize)
			{
				addWord(word, topic);
				sum += word.size();
			}
		}

		m_totalSize[topic] += sum;
	}

	void NGramModel::getProbs(const std::string& word, std::map<int, double>& probs) const
	{
	}
	
	namespace
	{
		inline uint32_t increment(uint32_t val)
		{
			return val + 1;
		}
	}

	void NGramModel::addWord(const std::string& word, int topic)
	{
		const StrUTF8 src(word.c_str(), word.size());
		if (src.size() < m_gramSize)
			return;

		StrUTF8 mangled(" ");
		mangled.append(src);
		mangled.append(CharUTF8(" "));

		UTF8Trie_t& topicTrie = m_tries[topic];
		for (size_t i = 0; i < mangled.size() - m_gramSize + 1; ++i)
		{
			UTF8Trie_t *t = &topicTrie;
			for (size_t j = 0; j < m_gramSize; ++j)
				t = &t->addWithUpdate(mangled[i + j], increment);
		}
	}
}
