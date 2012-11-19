#include "barzer_spell_features.h"
#include "barzer_universe.h"
#include "barzer_spellheuristics.h"
#include "barzer_language.h"
#include "ay/ay_char.h"

namespace barzer
{
void TFE_ngram::operator()(ExtractedStringFeatureVec& outVec, const char *str, size_t str_len, int lang) const
{
	const size_t nGramSymbs = 3;
	const size_t stemThreshold = 4;
	
	std::string tmp;
	if (lang == LANG_ENGLISH || lang == LANG_RUSSIAN)
	{
		const size_t step = lang == LANG_ENGLISH ? 1 : 2;
		const size_t gramSize = step * nGramSymbs;
		
		std::string stemmed;
		if (str_len / step >= stemThreshold)
		{
			if (BZSpell::stem(stemmed, str, lang, stemThreshold))
				str = stemmed.c_str();
		}
		
		if (str_len >= gramSize)
			for (size_t i = 0, end = str_len - gramSize + 1; i < end; i += step)
			{
				tmp.assign(str + i, gramSize);
				outVec.push_back(ExtractedStringFeature(tmp, i / step));
			}
	}
	else
	{
		ay::StrUTF8 utf8(str, str_len);
		if (utf8.size() >= stemThreshold)
		{
			std::string stemmed;
			if (BZSpell::stem(stemmed, str, lang, stemThreshold))
				utf8.assign(stemmed.c_str());
		}
		
		if (utf8.size() >= nGramSymbs)
			for (size_t i = 0, end = utf8.size() - nGramSymbs + 1; i < end; ++i)
				outVec.push_back(ExtractedStringFeature(utf8.getSubstring(i, nGramSymbs), nGramSymbs));
	}
}

void TFE_bastard::operator()(ExtractedStringFeatureVec& outVec, const char *str, size_t str_len, int lang) const
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

namespace
{
	struct MatchResult
	{
		uint32_t m_strId;
		int m_dist;
		double m_confidence;
		
		MatchResult()
		: m_strId(0xffffffff)
		, m_dist(255)
		, m_confidence(0)
		{
		}
		
		MatchResult(uint32_t strId, int dist, double conf)
		: m_strId(strId)
		, m_dist(dist)
		, m_confidence(conf)
		{
		}
	};
	
	template<typename Pair>
	struct PairSortOrderer
	{
		bool operator()(const Pair& v1, const Pair& v2) const
		{
			return v1.second < v2.second;
		}
	};
	
	inline size_t diff_by_more_than( size_t x, size_t y, size_t delta )
	{
		return (x > y ? (x - y > delta) : (y - x > delta));
	}
	
	struct MatchVisitor : public boost::static_visitor<MatchResult>
	{
		const char *m_str;
		size_t m_strLen;
		const ay::StrUTF8 m_strUtf8;
		
		int m_lang;
		size_t m_maxLevDist;
		size_t m_glyphCount;
		
		MatchVisitor(const char *str, size_t strLen, int lang, size_t maxLevDist)
			: m_str(str)
			, m_strLen(strLen)
			, m_strUtf8(m_str, m_strLen)
			, m_lang(lang)
			, m_maxLevDist(maxLevDist)
			, m_glyphCount(m_strUtf8.getGlyphCount())
		{}
		
		template<typename T>
		MatchResult operator()(const TFE_storage<T>& storage) const
		{
			const int maxDist = (m_maxLevDist < 3 ? m_maxLevDist : 3);
			
			StoredStringFeatureVec storedVec;
			ExtractedStringFeatureVec extractedVec;
			TFE_TmpBuffers tmpBuf(storedVec, extractedVec);
			storage.extractSTF(tmpBuf, m_str, m_strLen, m_lang);
			
			typedef std::map<uint32_t, double> CounterMap_t;
			CounterMap_t counterMap;
			for (const auto& feature : storedVec)
			{
				const auto srcs = storage.getSrcsForFeature(feature);
				if (!srcs)
					continue;
				
				for (uint32_t source : *srcs)
				{
					auto pos = counterMap.find(source);
					if (pos == counterMap.end())
						pos = counterMap.insert(std::make_pair(source, 0)).first;
					pos->second += 1. / (srcs->size() * srcs->size());
				}
			}
			
			if (counterMap.empty())
				return MatchResult();
			
			std::vector<std::pair<uint32_t, double>> sorted;
			sorted.reserve(counterMap.size());
			std::copy(counterMap.begin(), counterMap.end(), std::back_inserter(sorted));
			
			std::sort(sorted.rbegin(), sorted.rend(), PairSortOrderer<std::pair<uint32_t, double>>());
			
			ay::LevenshteinEditDistance levDist;
			const size_t numGlyphs = m_strUtf8.getGlyphCount();
			//std::cout << __PRETTY_FUNCTION__ << " got num words: " << sorted.size() << std::endl;
			
			typedef std::pair<uint32_t, int> LevInfo_t;
			std::vector<LevInfo_t> levInfos;
			const size_t topNum = 2048;
            bool langIsTwoByte = Lang::isTwoByteLang(m_lang);
			for (size_t i = 0, takenCnt = 0, max = sorted.size(); i < max && takenCnt < topNum; ++i)
			{
				const char *str = storage.getPool()->resolveId(sorted[i].first);
				const size_t strLen = strlen(str);
				
				size_t numGlyphs = 0;
				auto lang = Lang::getLangAndLengthNoUniverse(numGlyphs, str, strLen);
				if (lang != m_lang)
					continue;
				
				if (diff_by_more_than(m_glyphCount, numGlyphs, maxDist))
					continue;
				else
					++takenCnt;
				
				int dist = 100;
				if (lang == LANG_ENGLISH) {
					dist = levDist.ascii_no_case(str, m_str);
				} else if (langIsTwoByte) {
					dist = levDist.twoByte(str, strLen / 2, m_str, m_strLen / 2);
				} else {
					dist = levDist.utf8(m_strUtf8, ay::StrUTF8(str, strLen));
                }
				if (dist > maxDist ||
						(dist == 2 && numGlyphs < 5) ||
						(dist == 3 && numGlyphs < 7))
					continue;

				levInfos.push_back(LevInfo_t(sorted[i].first, dist));
			}
			std::sort(levInfos.begin(), levInfos.end(), PairSortOrderer<LevInfo_t>());
			/*
			for (const auto& info : levInfos)
				std::cout << storage.getPool()->resolveId(info.first) << " " << info.second << std::endl;
			*/
			
			return !levInfos.empty() ?
					MatchResult(levInfos[0].first, levInfos[0].second, 1 / static_cast<double>(sorted.size())) :
					MatchResult();
		}
	};
}

FeaturedSpellCorrector::FeaturedMatchInfo FeaturedSpellCorrector::getBestMatch(const char *str, size_t strLen, int lang, size_t maxLevDist)
{
	if (lang < 0 || lang >= LANG_MAX)
	{
		AYLOG(ERROR) << "erroneous language " << lang << std::endl;
		return FeaturedMatchInfo();
	}
	
	MatchVisitor vis(str, strLen, lang, maxLevDist);
	if (m_matchStrategy == MatchStrategy::FirstWins)
	{
		for (const auto& storage : m_storages)
		{
			const auto& res = boost::apply_visitor(vis, storage);
			if (res.m_strId != 0xffffffff)
				return FeaturedMatchInfo(res.m_strId, res.m_dist);
		}
		return FeaturedMatchInfo();
	}
	else if (m_matchStrategy == MatchStrategy::BestWins)
	{
		MatchResult bestRes;
		for (const auto& storage : m_storages)
		{
			const auto& res = boost::apply_visitor(vis, storage);
			if (res.m_strId != 0xffffffff &&
					res.m_dist <= bestRes.m_dist &&
					res.m_confidence > bestRes.m_confidence)
				bestRes = res;
		}
		return FeaturedMatchInfo(bestRes.m_strId, bestRes.m_dist);;
	}
	else
	{
		AYLOG(ERROR) << "unknown match strategy " << static_cast<int>(m_matchStrategy);
		return FeaturedMatchInfo();
	}
}
}
