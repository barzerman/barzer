#include "barzer_spell_features.h"
#include "barzer_universe.h"
#include "barzer_spellheuristics.h"
#include "barzer_language.h"

namespace barzer
{
void TFE_ngram::operator()(ExtractedStringFeatureVec& outVec, const char *str, size_t str_len, int lang)
{
	const size_t nGramSymbs = 3;
	
	std::string tmp;
	if (lang == LANG_ENGLISH || lang == LANG_RUSSIAN)
	{
		const size_t step = lang == LANG_ENGLISH ? 1 : 2;
		const size_t gramSize = step * nGramSymbs;
		if (str_len >= gramSize)
			for (size_t i = 0, end = str_len - gramSize + 1; i < end; i += step)
			{
				tmp.assign(str + i, gramSize);
				outVec.push_back(ExtractedStringFeature(tmp, i / step));
			}
	}
	else
	{
		const ay::StrUTF8 utf8(str, str_len);
		if (utf8.size() >= nGramSymbs)
			for (size_t i = 0, end = utf8.size() - nGramSymbs + 1; i < end; ++i)
				outVec.push_back(ExtractedStringFeature(utf8.getSubstring(i, nGramSymbs), nGramSymbs));
	}
}

void TFE_bastard::operator()(ExtractedStringFeatureVec& outVec, const char *str, size_t str_len, int lang)
{
	std::string out;
	
	if (lang == LANG_ENGLISH)
		ChainHeuristic(EnglishSLHeuristic(0), RuBastardizeHeuristic(0)).transform(str, str_len, out);
	else if (lang == LANG_RUSSIAN)
		RuBastardizeHeuristic(0).transform(str, str_len, out);
	
	if (!out.empty())
		outVec.push_back(ExtractedStringFeature(out, 0));
}

namespace
{
	struct WordAdderVis : public boost::static_visitor<void>
	{
		uint32_t m_strId;
		const char *m_str;
		int m_lang;
		
		StoredStringFeatureVec m_storedVec;
		ExtractedStringFeatureVec m_extractedVec;
		TFE_TmpBuffers m_buf;
		
		WordAdderVis(uint32_t strId, const char *str, int lang)
		: m_strId(strId)
		, m_str(str)
		, m_lang(lang)
		, m_buf(m_storedVec, m_extractedVec)
		{
		}
		
		template<typename T>
		void operator()(TFE_storage<T>& storage)
		{
			m_buf.clear();
			storage.extractAndStore(m_buf, m_strId, m_str, m_lang);
		}
	};
}

void FeaturedSpellCorrector::addWord(uint32_t strId, const char* str, int lang)
{
	WordAdderVis adderVis(strId, str, lang);
	for (auto& heur : m_storages)
		boost::apply_visitor(adderVis, heur);
}
}
