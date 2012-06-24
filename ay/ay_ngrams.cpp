#include "ay_ngrams.h"
#include <iostream>

namespace ay
{
	NGramModel::NGramModel(size_t size)
	: m_gramSize(size)
	, m_totalSize(0)
	{
	}
	
	namespace
	{
		inline StrUTF8 mangle(const StrUTF8& src)
		{
			StrUTF8 mangled(" ");
			mangled.append(src);
			mangled.append(CharUTF8(" "));
			mangled.toLower();
			return mangled;
		}

		inline uint32_t increment(uint32_t val)
		{
			return val + 1;
		}
	}

	void NGramModel::addWord(const char *word)
	{
		const StrUTF8& mangled = mangle(StrUTF8(word));

		const size_t numGrams = mangled.size() - m_gramSize + 1;
		for (size_t i = 0; i < numGrams; ++i)
		{
			UTF8Trie_t *t = &m_trie;
			for (size_t j = 0; j < m_gramSize; ++j)
				t = &t->addWithUpdate(mangled[i + j], increment);
		}

		m_totalSize += numGrams;
	}

	void NGramModel::addWords(const StringList_t& text)
	{
		for (size_t i = 0, size = text.size(); i < size; ++i)
			addWord(text.at(i).c_str());
	}

	double NGramModel::getProb(const char* word) const
	{
		uint64_t totalWeight = 0;

		std::vector<CharUTF8> charBuf(m_gramSize);
		const StrUTF8& mangled = mangle(StrUTF8(word));
		for (size_t i = 0; i < mangled.size() - m_gramSize + 1; ++i)
		{
			for (size_t j = 0; j < m_gramSize; ++j)
				charBuf[j] = mangled[i + j];

			const UTF8Trie_t *result = m_trie.getLongestPath(charBuf.begin(), charBuf.end()).first;
			totalWeight += result ? result->data () : 0;
		}

		return static_cast<double> (totalWeight) / m_totalSize;
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
		trie_visitor<UTF8Trie_t, CallbackTrieDump> vis(cb);
		vis.visit(m_trie);
	}

	TopicModelMgr::TopicModelMgr(size_t size)
	: m_gramSize(size)
	{
	}

	NGramModel& TopicModelMgr::getModel(int topic)
	{
		ModelsDict_t::iterator pos = m_models.find(topic);
		if (pos == m_models.end())
			pos = m_models.insert(std::make_pair(topic, NGramModel(m_gramSize))).first;
		return pos->second;
	}
}
