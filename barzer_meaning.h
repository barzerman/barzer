#ifndef BARZER_MEANING_H
#define BARZER_MEANING_H

#include <unordered_map>
#include <ay/ay_stackvec.h>

namespace barzer
{
	typedef uint32_t WordMeaning;
	typedef std::pair<const WordMeaning*, size_t> WordMeaningBufPtr;

	class MeaningsStorage
	{
		typedef ay::StackVec<WordMeaning> MVec_t;

		std::unordered_map<uint32_t, MVec_t> m_word2meanings;
	public:
		MeaningsStorage();

		void addMeaning(uint32_t wordId, WordMeaning meaningId)
		{
			m_word2meanings[wordId].push_back(meaningId);
		}

		WordMeaningBufPtr getMeanings(uint32_t wordId) const
		{
			std::unordered_map<uint32_t, MVec_t>::const_iterator pos = m_word2meanings.find(wordId);
			return pos != m_word2meanings.end() ?
					std::make_pair(pos->second.getRawBuf(), pos->second.size()) :
					WordMeaningBufPtr();
		}
	};
} // namespace barzer 

#endif //  BARZER_MEANING_H
