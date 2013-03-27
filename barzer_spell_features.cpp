
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#include "barzer_spell_features.h"
#include "barzer_universe.h"
#include "barzer_spellheuristics.h"
#include "barzer_language.h"
#include "ay/ay_char.h"

namespace barzer
{
void TFE_ngram::operator()(ExtractedStringFeatureVec& outVec, const char *str, size_t str_len, int lang) const
{
	const size_t stemThreshold = 4;
	
	for (size_t nGramSymbs = m_minGrams; nGramSymbs <= m_maxGrams; ++nGramSymbs)
	{
		outVec.reserve(outVec.size() + str_len - nGramSymbs + 3);
		
		std::string tmp;
		if (lang == LANG_ENGLISH || lang == LANG_RUSSIAN)
		{
			const size_t step = lang == LANG_ENGLISH ? 1 : 2;
			const size_t gramSize = step * nGramSymbs;
			
			std::string stemmed;
			if (m_stem && str_len / step >= stemThreshold)
			{
				if (BZSpell::stem(stemmed, str, lang, stemThreshold))
					str = stemmed.c_str();
			}
			
			if (str_len >= gramSize)
				for (size_t i = 0, end = str_len - gramSize + 1; i < end; i += step)
				{
					tmp.assign(str + i, gramSize);
					outVec.push_back(ExtractedStringFeature(tmp, i / step + 1));
				}
			
			if (m_makeSideGrams && str_len >= gramSize - 1)
			{
				tmp = ' ';
				tmp.append(str, gramSize - 1);
				outVec.push_back(ExtractedStringFeature(tmp, 0));
				
				tmp.assign(str + str_len - gramSize, gramSize - 1);
				tmp.append(" ");
				outVec.push_back(ExtractedStringFeature(tmp, 2 + (str_len - gramSize + 1) / step));
			}
		}
		else
		{
			ay::StrUTF8 utf8(str, str_len);
			if (m_stem && utf8.size() >= stemThreshold)
			{
				std::string stemmed;
				if (BZSpell::stem(stemmed, str, lang, stemThreshold))
					utf8.assign(stemmed.c_str());
			}
			
			if (utf8.size() >= nGramSymbs)
				for (size_t i = 0, end = utf8.size() - nGramSymbs + 1; i < end; ++i)
					outVec.push_back(ExtractedStringFeature(utf8.getSubstring(i, nGramSymbs), i + 1));
				
			if (m_makeSideGrams && utf8.size() >= nGramSymbs - 1)
			{
				std::string tmp(" ");
				tmp.append(utf8.getSubstring(0, nGramSymbs - 1));
				outVec.push_back(ExtractedStringFeature(tmp, 0));
				
				tmp.assign(utf8.getSubstring(utf8.size() - nGramSymbs + 1, nGramSymbs - 1));
				tmp.append(" ");
				outVec.push_back(ExtractedStringFeature(tmp, utf8.size() - nGramSymbs + 2));
			}
		}
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

void FeaturedSpellCorrector::addWord(uint32_t strId, const char* str, int lang, BZSpell& bzSpell )
{
	WordAdderVis adderVis(strId, str, lang);
    /// need to memoize stem and lang so that we dont have to do it all over again
    if( !getWordData(strId) ) {
        std::string stem;
        FeatureCorrectorWordData wd;
        wd.lang = lang;
        /// here we probly need to do a deep deep stem  
        wd.numGlyphs = Lang::getNumGlyphs(lang, str, strlen(str));
        if( bzSpell.stem( stem, str, wd.lang ) ) {
            wd.stemStrId = bzSpell.getUniverse().getGlobalPools().string_intern( stem.c_str() ) ;
            wd.numGlyphsInStem=Lang::getNumGlyphs(lang,stem.c_str(), stem.length());
        }
        setWordData( strId, wd );
    }
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
        bool m_langIsTwoByte;
		size_t m_maxLevDist;
		size_t m_glyphCount;
        FeaturedSpellCorrector& d_corrector;
        std::string m_stem;
        bool m_gotStemmed;
		ay::StrUTF8 m_stemStrUtf8;
	    mutable ay::LevenshteinEditDistance m_levDist;
		
        inline size_t get_lev_dist( const char* str, size_t strLen, const char* str1, size_t strLen1, ay::StrUTF8& strUtf8, const ay::StrUTF8& str1Utf8 ) const
        {
            if (m_lang == LANG_ENGLISH) {
                return m_levDist.ascii_no_case(str, str1);
            } else if (m_langIsTwoByte) {
                return m_levDist.twoByte(str, strLen / 2, str1, strLen1 / 2);
            } else {
                strUtf8.assign( str, str+strLen );
                return m_levDist.utf8(strUtf8, str1Utf8 ) ; 
            }
        }
		MatchVisitor(const char *str, size_t strLen, int lang, size_t maxLevDist, FeaturedSpellCorrector& corrector)
			: m_str(str)
			, m_strLen(strLen)
			, m_strUtf8(m_str, m_strLen)
			, m_lang(lang)
			, m_langIsTwoByte(Lang::isTwoByteLang(lang))
			, m_maxLevDist(maxLevDist)
			, m_glyphCount(m_strUtf8.getGlyphCount())
			, d_corrector(corrector)
            , m_gotStemmed(BZSpell::stem(m_stem, str, lang, 3))
		{
            if( m_gotStemmed )
                m_stemStrUtf8= ay::StrUTF8(m_stem.c_str(),m_stem.length());
        }
		
		template<typename T>
		MatchResult operator()(const TFE_storage<T>& storage) const
		{
			const size_t maxDist = (m_maxLevDist < 3 ? m_maxLevDist : 3);
			
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
				
				for (const auto& sourceFeature : *srcs)
				{
					const auto source = sourceFeature.docId;
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
			
			//std::cout << __PRETTY_FUNCTION__ << " got num words: " << sorted.size() << std::endl;
			
			typedef std::pair<uint32_t, int> LevInfo_t;
			std::vector<LevInfo_t> levInfos;
			const size_t topNum = 2048;
            bool langIsTwoByte = Lang::isTwoByteLang(m_lang);
            ay::StrUTF8 currentStem;
			for (size_t i = 0, takenCnt = 0, max = sorted.size(); i < max && takenCnt < topNum; ++i)
			{
                uint32_t strId = sorted[i].first;
                const FeatureCorrectorWordData* wd = d_corrector.getWordData( strId );
                if( !wd ) { // this should never happen
                    AYLOG(ERROR) << "wd is null\n";
                    continue;
                }

				const char *str = storage.getPool()->resolveId(strId);
				const size_t strLen = strlen(str);
			    
				if (wd->lang != m_lang)
					continue;
				
				if (diff_by_more_than(m_glyphCount, wd->numGlyphs, maxDist))
					continue;
				else
					++takenCnt;
				
				int dist = 100;
				if (m_lang == LANG_ENGLISH) {
					dist = m_levDist.ascii_no_case(str, m_str);
				} else if (langIsTwoByte) {
					dist = m_levDist.twoByte(str, strLen / 2, m_str, m_strLen / 2);
				} else {
					dist = m_levDist.utf8(m_strUtf8, ay::StrUTF8(str, strLen));
                }
                if( dist < 0 ) 
                    dist = 0;

				if ((size_t)dist > maxDist || (dist == 2 && wd->numGlyphs < 5) || (dist == 3 && wd->numGlyphs < 7))
					continue;

                // if more than 1 character is different we compare stems 
                if(dist > 1 && m_gotStemmed && wd->hasStem()) {
                    const char *stemStr = storage.getPool()->resolveId(wd->stemStrId);
                    if( !stemStr ) {
                    AYLOG(ERROR) << "stemStr is null\n";
                    }
                    const size_t stemStrLen = strlen(stemStr);

                    size_t stemDist = get_lev_dist( stemStr, stemStrLen, m_stem.c_str(), m_stem.length(), currentStem, m_stemStrUtf8 ); 
                    if (stemDist > maxDist || (stemDist == 2 && wd->numGlyphsInStem <= 5) || (stemDist == 3 && wd->numGlyphsInStem <= 7))
                        continue;
                }
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
	
	MatchVisitor vis(str, strLen, lang, maxLevDist, *this);
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
