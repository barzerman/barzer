#pragma once
#include <barzer_entity.h>
#include <barzer_spell_features.h>
#include <boost/range/iterator_range.hpp>

namespace barzer {

template<typename T>
class NGramStorage {
	TFE_storage<TFE_ngram> m_gram;
	
	boost::unordered_multimap<uint32_t, T> m_storage;
	
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
	
	void setSLEnabled(bool enabled=true) { m_soundsLikeEnabled = enabled; }
    

	void addWord(const char *srcStr, const T& data)
	{
		std::string str(srcStr);
		if (m_soundsLikeEnabled)
		{
			std::string tmp;
			EnglishSLHeuristic(0).transform(srcStr, str.size(), tmp);

		    const auto utfLength = ay::StrUTF8::glyphCount(tmp.c_str(), tmp.c_str() + tmp.size());
            if( utfLength > 4 ) {
                str.clear();
                for( auto i : tmp ) { if( !isspace(i) ) str.push_back(i); }
            } else
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
	
	void getMatchesRange(StoredStringFeatureVec::const_iterator begin, StoredStringFeatureVec::const_iterator end,
			std::vector<FindInfo>& out, size_t max, double minCov) const
	{
		const double srcFCnt = std::distance(begin, end);
		
		typedef std::map<uint32_t, double> CounterMap_t;
		CounterMap_t counterMap;
		
		struct FeatureStatInfo
		{
			uint16_t fCount;
			uint16_t firstFeature;
			uint16_t lastFeature;
		};
		std::map<uint32_t, FeatureStatInfo> doc2fCnt;
		for (const auto& feature : boost::make_iterator_range(begin, end))
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
		
		size_t curItem = 0;
		ay::LevenshteinEditDistance lev;
		// const auto utfLength = ay::StrUTF8::glyphCount(str.c_str(), str.c_str() + str.size());
        size_t countAdded = 0;
		for (const auto& item : sorted)
		{
            auto dataPosRange = m_storage.equal_range(item.first);
            if( dataPosRange.first == m_storage.end() ) 
                continue;
			
			const auto resolvedResult = m_gram.d_pool->resolveId(item.first);
			const auto resolvedLength = std::strlen(resolvedResult);
			
            const  size_t dist = 1; // we shouldnt need to compute levenshtein
			const auto& info = doc2fCnt[item.first];
            double cover = info.fCount / srcFCnt;
            if( m_soundsLikeEnabled) 
                cover *= 0.95;

            if( cover >= minCov) {
                for( auto dataPos = dataPosRange.first; dataPos != dataPosRange.second; ++dataPos ) {
			        out.push_back({
					        item.first,
					        &(dataPos->second),
					        item.second,
					        dist,
					        cover
				    });
                 
                    if( ++countAdded  >= max )
                        break;
                }
            }
		}
	}
	
	void getMatches(const char *origStr, size_t origStrLen, std::vector<FindInfo>& out, size_t max, double minCov ) const
	{
		std::string str(origStr, origStrLen);
		if (m_soundsLikeEnabled)
		{
			std::string tmp;
			EnglishSLHeuristic(0).transform(origStr, origStrLen, tmp);
		    const auto utfLength = ay::StrUTF8::glyphCount(tmp.c_str(), tmp.c_str() + tmp.size());
            if( utfLength > 4 ) {
                str.clear();
                for( auto i : tmp ) { if( !isspace(i) ) str.push_back(i); }
            } else 
			    str = tmp;
		}
		
        StoredStringFeatureVec storedVec;
        ExtractedStringFeatureVec extractedVec;
        TFE_TmpBuffers bufs( storedVec, extractedVec );
		
		m_gram.extractSTF(bufs, str.c_str(), str.size(), LANG_UNKNOWN);
		
		getMatchesRange(storedVec.begin(), storedVec.end(), out, max, minCov);
	}
	
	std::pair<StoredStringFeatureVec::const_iterator, StoredStringFeatureVec::const_iterator>
		getIslands(const char *origStr, size_t origStrLen) const;
private:
	std::pair<StoredStringFeatureVec::const_iterator, StoredStringFeatureVec::const_iterator>
		searchRange4Island(StoredStringFeatureVec::const_iterator, StoredStringFeatureVec::const_iterator);
};

template<typename T>
std::pair<StoredStringFeatureVec::const_iterator, StoredStringFeatureVec::const_iterator>
	NGramStorage<T>::getIslands(const char* origStr, size_t origStrLen) const
{
	std::string str(origStr, origStrLen);
	if (m_soundsLikeEnabled)
	{
		std::string tmp;
		EnglishSLHeuristic(0).transform(origStr, origStrLen, tmp);
		const auto utfLength = ay::StrUTF8::glyphCount(tmp.c_str(), tmp.c_str() + tmp.size());
		if( utfLength > 4 ) {
			str.clear();
			for( auto i : tmp ) { if( !isspace(i) ) str.push_back(i); }
		} else 
			str = tmp;
	}
	
	StoredStringFeatureVec storedVec;
	ExtractedStringFeatureVec extractedVec;
	TFE_TmpBuffers bufs( storedVec, extractedVec );
	m_gram.extractSTF(bufs, str.c_str(), str.size(), LANG_UNKNOWN);
	return searchRange4Island(storedVec.begin(), storedVec.end());
}

namespace
{
	template<typename T>
	std::vector<T> intersectVectors(std::vector<T> smaller, const std::vector<T>& bigger)
	{
		for (auto i = smaller.begin(); i != smaller.end(); )
		{
			if (std::find(bigger.begin(), bigger.end(), *i) != bigger.end())
				++i;
			else
				i = smaller.erase(i);
		}
		return smaller;
	}
}

template<typename T>
std::pair<StoredStringFeatureVec::const_iterator, StoredStringFeatureVec::const_iterator>
	NGramStorage<T>::searchRange4Island(StoredStringFeatureVec::const_iterator begin, StoredStringFeatureVec::const_iterator end)
{
	const size_t degradationLength = 1;
	
	const auto rarestGram = std::min_element (begin, end,
			[this](const StoredStringFeature& l, const StoredStringFeature& r) -> bool
			{
				const auto leftPos = m_gram.d_fm.find(l);
				if (leftPos == m_gram.d_fm.end())
					return false;
				const auto rightPos = m_gram.d_fm.find(r);
				if (rightPos == m_gram.d_fm.end())
					return true;
				return leftPos->second.size() < rightPos->second.size();
			});
	if (rarestGram == end)
		return { end, end };
	
	auto currentDocs = m_gram.d_fm.find(*rarestGram)->second;
	
	auto curLeft = rarestGram,
		 curRight = rarestGram,
		 bestLeft = rarestGram,
		 bestRight = rarestGram;
		 
	size_t curLeftDegradations = 0,
		   curRightDegradations = 0;
	
	bool growLeft = curLeft != begin;
	bool growRight = curRight + 1 != end;
	
	while (growLeft || growRight)
	{
		if (curLeft == begin)
			growLeft = false;
		if (curRight + 1 == end)
			growRight = false;
		
		if (growLeft)
		{
			--curLeft;
			const auto& leftDocs = m_gram.d_fm.find(curLeft)->second;
			auto xSect = intersectVectors(currentDocs, leftDocs);
			
			if (xSect.empty())
			{
				if (++curLeftDegradations > degradationLength)
					growLeft = false;
			}
			else
			{
				bestLeft = curLeft;
				std::swap(currentDocs, xSect);
			}
		}
		if (growRight)
		{
			++curRight;
			const auto& rightDocs = m_gram.d_fm.find(curLeft)->second;
			auto xSect = intersectVectors(currentDocs, rightDocs);
			
			if (xSect.empty())
			{
				if (++curRightDegradations > degradationLength)
					growRight = false;
			}
			else
			{
				bestRight = curRight;
				std::swap(currentDocs, xSect);
			}
		}
	}
	
	auto lefterIsland = searchRange4Island(begin, bestLeft); // or maybe till rarestGram, needs discussion
	auto righterIsland = searchRange4Island(bestRight + 1, end); // or maybe till rarestGram, needs discussion
	
	const auto thisDist = std::distance(bestLeft, bestRight);
	const auto leftDist = std::distance(lefterIsland.first, lefterIsland.second);
	const auto rightDist = std::distance(righterIsland.first, righterIsland.second);
	
	auto bestRecursiveDist = std::max(leftDist, rightDist);
	auto bestRecursive = leftDist > rightDist ? lefterIsland : righterIsland;
	
	return bestRecursiveDist > thisDist ? bestRecursive  : std::make_pair(bestLeft, bestRight);
}

class StoredUniverse;

class BENI {
    ay::UniqueCharPool d_charPool;
public:
    ay::UniqueCharPool& charPool() { return d_charPool; }
    const ay::UniqueCharPool& charPool() const { return d_charPool; }

    NGramStorage<BarzerEntity> d_storage;
    StoredUniverse& d_universe;

    /// add all entities by name for given class 
    void addEntityClass( const StoredEntityClass& ec );
    /// returns max coverage
    double search( BENIFindResults_t&, const char* str, double minCov) const;

    void clear() { d_storage.clear(); }
    BENI( StoredUniverse& u );
    
    static bool normalize( std::string& out, const std::string& in ) ;
    void setSL( bool );
};
class SmartBENI {
    BENI d_beniStraight,  // beni without soundslike normalization
         d_beniSl;        // beni with sounds like normalization
    /// if d_isSL == true d_beniSl has sounds like processed ngrams .. otherwise it's blank
    bool d_isSL;
public:
    void clear();
    SmartBENI( StoredUniverse& u );

    void addEntityClass( const StoredEntityClass& ec );
    void search( BENIFindResults_t&, const char* str, double minCov) const;
};

} // namespace barzer
