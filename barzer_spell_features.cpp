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
				for (size_t i = 0; i < str_len - gramSize; i += step)
				{
					tmp.assign(str + i, gramSize);
					outVec.push_back(ExtractedStringFeature(tmp, i / step));
				}
		}
		else
		{
			const ay::StrUTF8 utf8(str, str_len);
			if (utf8.size() >= nGramSymbs)
				for (size_t i = 0, size = utf8.size(); i < size - nGramSymbs; ++i)
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
}
