#include "ay_ngrams.h"
#include <iostream>
#include <alloca.h>

namespace ay
{
	namespace ASCII
	{
		NGramModel::NGramModel()
		: m_totalSize(0)
		{
		}

		void NGramModel::addWord (const char *word)
		{
			const size_t wordLen = std::strlen(word);
			if (wordLen < 2)
				return;

			union
			{
				uint32_t m_int;
				char buf[4];
			} u;
			u.m_int = 0;

			const size_t numGrams = wordLen - 2;
			for (size_t i = 0; i < numGrams; ++i)
			{
				u.buf[0] = word[i];
				u.buf[1] = word[i + 1];
				u.buf[2] = word[i + 2];

				EncDict_t::iterator pos = m_encounters.find(u.m_int);
				if (pos == m_encounters.end())
					m_encounters.insert(std::make_pair(u.m_int, static_cast<uint64_t>(1)));
				else
					++pos->second;
			}

			if (wordLen >= 2)
			{
				u.buf[0] = ' ';
				u.buf[1] = word[0];
				u.buf[2] = word[1];

				EncDict_t::iterator pos = m_encounters.find(u.m_int);
				if (pos == m_encounters.end())
					m_encounters.insert(std::make_pair(u.m_int, static_cast<uint64_t>(1)));
				else
					++pos->second;
				
				u.buf[0] = word[wordLen - 2];
				u.buf[1] = word[wordLen - 1];
				u.buf[2] = ' ';

				pos = m_encounters.find(u.m_int);
				if (pos == m_encounters.end())
					m_encounters.insert(std::make_pair(u.m_int, static_cast<uint64_t>(1)));
				else
					++pos->second;
			}

			m_totalSize += numGrams + 2;
		}

		void NGramModel::addWords (const StringList_t& text)
		{
			for (size_t i = 0, size = text.size(); i < size; ++i)
				addWord(text.at(i).c_str());
		}

		double NGramModel::getProb (const char *word) const
		{
			uint64_t totalWeight = 0;

			const size_t wordLen = std::strlen(word);
			if (wordLen < 2)
				return 0;

			union
			{
				uint32_t m_int;
				char buf[4];
			} u;
			u.m_int = 0;

			const size_t numGrams = wordLen - 2;
			for (size_t i = 0; i < numGrams; ++i)
			{
				u.buf[0] = word[i];
				u.buf[1] = word[i + 1];
				u.buf[2] = word[i + 2];

				EncDict_t::const_iterator pos = m_encounters.find(u.m_int);
				if (pos != m_encounters.end())
					totalWeight += pos->second;
			}

			if (wordLen >= 2)
			{
				u.buf[0] = ' ';
				u.buf[1] = word[0];
				u.buf[2] = word[1];

				EncDict_t::const_iterator pos = m_encounters.find(u.m_int);
				if (pos != m_encounters.end())
					totalWeight += pos->second;
				
				u.buf[0] = word[wordLen - 2];
				u.buf[1] = word[wordLen - 1];
				u.buf[2] = ' ';

				pos = m_encounters.find(u.m_int);
				if (pos != m_encounters.end())
					totalWeight += pos->second;
			}

			return static_cast<double> (totalWeight) / m_totalSize;
		}
	}

	namespace UTF8
	{
		NGramModel::NGramModel()
		: m_totalSize(0)
		{
		}

		namespace
		{
			inline std::vector<uint64_t> hashString(const StrUTF8& src)
			{
				std::vector<uint64_t> result (src.size() + 2, 0x20);
				for (size_t i = 0, sz = src.size(); i < sz; ++i)
					result[i + 1] = src[i].toUTF32() & 0xFFFF;
				return result;
			}
		}

		void NGramModel::addWord(const char *word)
		{
			const std::vector<uint64_t>& hashes = hashString(StrUTF8(word));

			const size_t numGrams = hashes.size() - 2;
			for (size_t i = 0; i < numGrams; ++i)
			{
				const uint64_t val = hashes[i] +
						(hashes[i + 1] << 16) +
						(hashes[i + 2] << 32);

				EncDict_t::iterator pos = m_encounters.find(val);
				if (pos == m_encounters.end())
					m_encounters.insert(std::make_pair(val, static_cast<uint64_t>(1)));
				else
					++pos->second;
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

			const std::vector<uint64_t>& hashes = hashString(StrUTF8(word));
			for (size_t i = 0; i < hashes.size() - 2; ++i)
			{
				const uint64_t val = hashes[i] +
						(hashes[i + 1] << 16) +
						(hashes[i + 2] << 32);

				EncDict_t::const_iterator pos = m_encounters.find(val);
				if (pos != m_encounters.end())
					totalWeight += pos->second;
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
	}

	void evalAllLangs (UTF8TopicModelMgr* utf8, ASCIITopicModelMgr* ascii, const char* str, std::vector< std::pair< int, double > >& probs)
	{
		std::vector<int> langs;

		utf8->getAvailableTopics(langs);
		for (size_t i = 0; i < langs.size(); ++i)
		{
			const int lang = langs[i];
			probs.push_back(std::make_pair(lang, utf8->getModel(lang).getProb(str)));
		}

		langs.clear();
		ascii->getAvailableTopics(langs);
		for (size_t i = 0; i < langs.size(); ++i)
		{
			const int lang = langs.at(i);
			probs.push_back(std::make_pair(lang, ascii->getModel(lang).getProb(str)));
		}
	}
}
