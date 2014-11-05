#include <barzer_beni_storage.h>
#include <boost/range/iterator_range.hpp>


namespace barzer {

void NGramStorage::clear()
{
    m_gram.clear();
    m_storage.clear();
    m_storedVec.clear();
    m_extractedVec.clear();
}

NGramStorage::NGramStorage(ay::UniqueCharPool& p)
: m_gram(p)
, m_soundsLikeEnabled(false)
{
    m_gram.d_extractor.m_makeSideGrams = true;
    m_gram.d_extractor.m_stem = false;
    m_gram.d_extractor.m_minGrams = 3;
    m_gram.d_extractor.m_maxGrams = 3;
    //m_gram.m_removeDuplicates = false;
}

void NGramStorage::setSLEnabled(bool enabled) { m_soundsLikeEnabled = enabled; }


void NGramStorage::addWord(const char *srcStr, const T& data)
{
    if( ! *srcStr ) return;

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

void NGramStorage::getMatchesRange(
    StoredStringFeatureVec::const_iterator begin, 
    StoredStringFeatureVec::const_iterator end,
    std::vector<FindInfo>& out, size_t max, double minCov, 
    const SearchFilter_f& filter
) const
{
    const double srcFCnt = std::distance(begin, end);

    struct FeatureStatInfo
    {
        double counter;
        uint16_t fCount;
    };

    #warning this should be optimized: counterMap is always a vector size = total number of ngrams. can be a lot for unicode sets
    const size_t counterMap_size = m_gram.d_max + 1;
    if( !counterMap_size )
        return;
    std::vector<FeatureStatInfo> counterMap(counterMap_size);

    for (const auto& feature : boost::make_iterator_range(begin, end)) {
        const auto srcs = m_gram.getSrcsForFeature(feature);
        if (!srcs)
            continue;
    
        const double srcs_size = srcs->size();
        if( !srcs_size ) 
            continue;

        const auto imp = 1. / (srcs_size*srcs_size);
        for (const auto& sourceFeature : *srcs) {
            const auto source = sourceFeature.docId;
        
            if( source < counterMap_size ) {
                auto& info = counterMap[source];
                info.counter += imp * sourceFeature.counter;
                info.fCount += sourceFeature.counter;
            }
        }
    }

    typedef std::vector<std::pair<uint32_t, FeatureStatInfo>> Sorted_t;
    Sorted_t sorted;
    sorted.reserve(counterMap_size);
    const auto normalizedMinCov = minCov * srcFCnt;

    for (size_t i = 0; i < counterMap_size; ++i)
    {
        auto& info = counterMap[i];
        if (info.fCount < normalizedMinCov)
            continue;

        if (filter)
        {
            const auto data = m_storage.equal_range(i);
            if (data.first == m_storage.end())
                continue;

            bool found = false;
            for (const auto& d : boost::make_iterator_range(data.first, data.second))
                if ((info.fCount = filter (d.second, info.fCount)) >= normalizedMinCov) {
                    found = true;
                    break;
                }

            if (!found)
                continue;
        }

        sorted.push_back({ i, info });
    }

    std::sort(sorted.begin(), sorted.end(),
            [](const typename Sorted_t::value_type& v1, const typename Sorted_t::value_type& v2)
                { return v1.second.fCount > v2.second.fCount; });

    size_t countAdded = 0;
    for (const auto& item : sorted) {
        const size_t dist = 1; // we shouldnt need to compute levenshtein
        double cover = item.second.fCount / srcFCnt;
        if( m_soundsLikeEnabled)
            cover *= 0.98;

        if( cover >= minCov) {
            const auto dataPosRange = m_storage.equal_range(item.first);
            if( dataPosRange.first == m_storage.end() )
                continue;

            for( auto dataPos = dataPosRange.first; dataPos != dataPosRange.second; ++dataPos ) {
                out.push_back({
                        item.first,
                        &(dataPos->second),
                        item.second.counter,
                        dist,
                        cover
                });
                if( ++countAdded  >= max )
                    break;
            }
        }
    }
}

void NGramStorage::getMatches(
        const char *origStr, 
        size_t origStrLen, 
        std::vector<FindInfo>& out,
        size_t max, 
        double minCov, 
        const SearchFilter_f& filter
    ) const
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

    getMatchesRange(storedVec.begin(), storedVec.end(), out, max, minCov, filter);
}

auto NGramStorage::getIslands(const char* origStr, size_t origStrLen, StoredStringFeatureVec& storedVec) const -> Islands_t
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

	ExtractedStringFeatureVec extractedVec;
	TFE_TmpBuffers bufs( storedVec, extractedVec );
	m_gram.extractSTF(bufs, str.c_str(), str.size(), LANG_UNKNOWN);

	return searchRange4Island(storedVec.begin(), storedVec.end());
}

namespace {
	template<typename Cont> inline Cont intersectVectors(Cont smaller, const Cont& bigger)
	{
		for (auto i = smaller.begin(); i != smaller.end(); )
		{
			if (std::find_if(bigger.begin(), bigger.end(),
					[i](const typename Cont::value_type& f) { return f.docId == i->docId; }) != bigger.end())
				++i;
			else
				i = smaller.erase(i);
		}
		return smaller;
	}
} // end of anonymous namespace

auto NGramStorage::searchRange4Island(StoredStringFeatureVec::const_iterator begin, StoredStringFeatureVec::const_iterator end) const -> Islands_t
{
	const uint16_t minLength = 3;
	if (std::distance(begin, end) < minLength)
		return Islands_t ();

	const size_t degradationLength = 1;

	const auto startGram = begin + std::distance(begin, end) / 2;

	const auto pos = m_gram.d_fm.find(*startGram);
	auto currentDocs = pos == m_gram.d_fm.end() ?
			FeaturesDict_t() :
			pos->second;

	auto curLeft = startGram,
		 curRight = startGram,
		 bestLeft = startGram,
		 bestRight = startGram;
	
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
			const auto leftDocsPos = m_gram.d_fm.find(*curLeft);
			const auto& leftDocs = leftDocsPos == m_gram.d_fm.end() ?
					FeaturesDict_t() :
					leftDocsPos->second;
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
			const auto rightDocsPos = m_gram.d_fm.find(*curRight);
			const auto& rightDocs = rightDocsPos == m_gram.d_fm.end() ?
					FeaturesDict_t() :
					rightDocsPos->second;
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

	auto lefterIslands = searchRange4Island(begin, bestLeft);
	auto righterIslands = searchRange4Island(bestRight + 1, end);

	auto compareIslands = [] (const Island_t& left, const Island_t& right)
		{ return std::distance (left.first, left.second) < std::distance (right.first, right.second); };

	Island_t thisResult { bestLeft, bestRight };

	Islands_t sum;
	if (lefterIslands.empty())
		std::swap(sum, righterIslands);
	else if (righterIslands.empty())
		std::swap(sum, lefterIslands);
	else
		std::merge(lefterIslands.begin(), lefterIslands.end(),
				righterIslands.begin(), righterIslands.end(),
				std::back_inserter(sum), compareIslands);

	if (std::distance(bestLeft, bestRight) < minLength)
		return sum;
	
	const auto toInsert = std::lower_bound(sum.begin(), sum.end(), thisResult, compareIslands);
	sum.insert(toInsert, thisResult);
	return sum;
}

} //namespace barzer
