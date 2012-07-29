#ifndef BARZER_MEANING_H
#define BARZER_MEANING_H

#include <boost/unordered_map.hpp>

#include <ay/ay_stackvec.h>

namespace barzer
{
	struct WordMeaning
	{
		uint32_t id;
		uint8_t prio;
	};
	typedef std::pair<const WordMeaning*, size_t> WordMeaningBufPtr;
	typedef std::pair<const uint32_t*, size_t> MeaningSetBufPtr;

	class MeaningsStorage
	{
		typedef ay::StackVec<WordMeaning> MVec_t;
		typedef ay::StackVec<uint32> WVec_t;

		boost::unordered_map<uint32_t, MVec_t> m_word2meanings;
		boost::unordered_map<uint32_t, WVec_t> m_meaning2words;
	public:
		MeaningsStorage();

		void addMeaning(uint32_t wordId, WordMeaning meaning)
		{
			m_word2meanings[wordId].push_back(meaning);
			m_meaning2words[meaning.id].push_back(wordId);
		}

		WordMeaningBufPtr getMeanings(uint32_t wordId) const
		{
			boost::unordered_map<uint32_t, MVec_t>::const_iterator pos = m_word2meanings.find(wordId);
			return pos != m_word2meanings.end() ?
					std::make_pair(pos->second.getRawBuf(), pos->second.size()) :
					WordMeaningBufPtr();
		}
		
		MeaningSetBufPtr getWords(uint32_t meaningId) const
		{
		}
	};
} // namespace barzer 

#endif //  BARZER_MEANING_H
