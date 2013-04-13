#pragma once
#include <barzer_entity.h>
#include <barzer_spell_features.h>

namespace barzer {

template<typename T>
class NGramStorage {
	TFE_storage<TFE_ngram> m_gram;
	
	boost::unordered_map<uint32_t, T> m_storage;
	
    //// this is temporary junk
	StoredStringFeatureVec m_storedVec;
	ExtractedStringFeatureVec m_extractedVec;
    /// end of 
	
	bool m_soundsLikeEnabled;
public:
    void clear() 
    {
        m_gram.clear();
        m_storage.clear();
        m_storedVec.clear();
        m_extractedVec.clear();
    }

	NGramStorage(ay::UniqueCharPool& p)
	: m_gram(p)
	, m_soundsLikeEnabled(false)
	{
		m_gram.d_extractor.m_makeSideGrams = false;
		m_gram.d_extractor.m_stem = false;
		m_gram.d_extractor.m_minGrams = 3;
		m_gram.d_extractor.m_maxGrams = 3;
	}
	
	void setSLEnabled(bool enabled) { m_soundsLikeEnabled = enabled; }
	
	void addWord(const char *srcStr, const T& data)
	{
		std::string str(srcStr);
		if (m_soundsLikeEnabled)
		{
			std::string tmp;
			EnglishSLHeuristic(0).transform(srcStr, str.size(), tmp);
			str = tmp;
		}
		
	    TFE_TmpBuffers bufs( m_storedVec, m_extractedVec );
		bufs.clear();
		
		auto strId = m_gram.d_pool->internIt(str.c_str());
		m_gram.extractAndStore(bufs, strId, str.c_str(), LANG_UNKNOWN);
		m_storage.insert({ strId, data });
	}
	
	struct FindInfo
	{
		uint32_t m_strId;
		const T *m_data;
		double m_relevance;
		size_t m_levDist;
		double m_coverage;
	};
	
	void getMatches(const char *origStr, size_t origStrLen, std::vector<FindInfo>& out, size_t max = 16, size_t topLev = 4) const
	{
		std::string str(origStr, origStrLen);
		if (m_soundsLikeEnabled)
		{
			std::string tmp;
			EnglishSLHeuristic(0).transform(origStr, origStrLen, tmp);
			str = tmp;
		}
		
        StoredStringFeatureVec storedVec;
        ExtractedStringFeatureVec extractedVec;
        TFE_TmpBuffers bufs( storedVec, extractedVec );
		
		m_gram.extractSTF(bufs, str.c_str(), str.size(), LANG_UNKNOWN);
		
		const double srcFCnt = storedVec.size();
		
		typedef std::map<uint32_t, double> CounterMap_t;
		CounterMap_t counterMap;
		
		struct FeatureStatInfo
		{
			uint16_t fCount;
			uint16_t firstFeature;
			uint16_t lastFeature;
		};
		std::map<uint32_t, FeatureStatInfo> doc2fCnt;
		for (const auto& feature : storedVec)
		{
			const auto srcs = m_gram.getSrcsForFeature(feature);
			if (!srcs)
				continue;
			
			for (const auto& sourceFeature : *srcs)
			{
				const auto source = sourceFeature.docId;
				
				auto pos = counterMap.find(source);
				if (pos == counterMap.end())
					pos = counterMap.insert({ source, 0 }).first;
				pos->second += 1. / (srcs->size() * srcs->size());
				
				auto docPos = doc2fCnt.find(source);
				if (docPos == doc2fCnt.end())
					docPos = doc2fCnt.insert({ source, { 0, static_cast<uint16_t>(-1), 0 } }).first;
				++docPos->second.fCount;
				
			}
		}
		
		if (counterMap.empty())
			return;
		
		std::vector<std::pair<uint32_t, double>> sorted;
		sorted.reserve(counterMap.size());
		std::copy(counterMap.begin(), counterMap.end(), std::back_inserter(sorted));
		
		std::sort(sorted.begin(), sorted.end(),
				[](const CounterMap_t::value_type& v1, const CounterMap_t::value_type& v2)
					{ return v1.second > v2.second; });
		
		if (sorted.size() > max)
			sorted.resize(max);
		
		out.reserve(sorted.size());
		
		size_t curItem = 0;
		ay::LevenshteinEditDistance lev;
		const auto utfLength = ay::StrUTF8::glyphCount(str.c_str(), str.c_str() + str.size());
		for (const auto& item : sorted)
		{
			auto dataPos = m_storage.find(item.first);
			
			const auto resolvedResult = m_gram.d_pool->resolveId(item.first);
			const auto resolvedLength = std::strlen(resolvedResult);
			
			const auto dist = ++curItem > topLev ?
				0 :
				barzer::Lang::getLevenshteinDistance(lev, str.c_str(), str.size(), resolvedResult, resolvedLength);
			
			const auto& info = doc2fCnt[item.first];
			out.push_back({
					item.first,
					(dataPos == m_storage.end() ? 0 : &dataPos->second),
					item.second,
					dist,
					info.fCount / srcFCnt
				});
		}
	}
};
class StoredUniverse;

class BENI {
    ay::UniqueCharPool d_charPool;
public:
    NGramStorage<BarzerEntity> d_storage;
    StoredUniverse& d_universe;

    /// add all entities by name for given class 
    void addEntityClass( const StoredEntityClass& ec );
    void search( BENIFindResults_t&, const char* str, double minCov) const;

    void clear() { d_storage.clear(); }
    BENI( StoredUniverse& u ) : 
        d_storage(d_charPool), d_universe(u) {}
    
    static bool normalize( std::string& out, const std::string& in ) ;
};

} // namespace barzer
