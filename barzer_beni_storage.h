#pragma once
#include <boost/unordered_map.hpp>
#include <string>
#include <vector>
#include <barzer_entity.h>
#include <barzer_spell_features.h>
#include <boost/function.hpp>

namespace barzer {
class NGramStorage
{
    typedef BarzerEntity T;
	TFE_storage<TFE_ngram> m_gram;

	boost::unordered_multimap<uint32_t, T> m_storage;

    //// this is temporary junk
	StoredStringFeatureVec m_storedVec;
	ExtractedStringFeatureVec m_extractedVec;
    /// end of

	bool m_soundsLikeEnabled;
public:
	typedef boost::function<double (T, double)> SearchFilter_f;

    void clear();

	NGramStorage(ay::UniqueCharPool& p);

	void setSLEnabled(bool enabled=true);

	void addWord(const char *srcStr, const T& data);
	const char* resolveFeature(const StoredStringFeature& f) const { return m_gram.resolveFeature(f); }

	struct FindInfo
	{
		uint32_t m_strId;
		const T *m_data;
		double m_relevance;
		size_t m_levDist;
		double m_coverage;
	};
	void getMatchesRange(
        StoredStringFeatureVec::const_iterator begin, 
        StoredStringFeatureVec::const_iterator end,
        std::vector<FindInfo>& out, size_t max, double minCov, 
        const SearchFilter_f& filter = SearchFilter_f()
    ) const;

	void getMatches(
        const char *origStr, 
        size_t origStrLen, 
        std::vector<FindInfo>& out,
        size_t max, 
        double minCov, 
        const SearchFilter_f& filter = SearchFilter_f()
    ) const;

	typedef std::pair<StoredStringFeatureVec::const_iterator, StoredStringFeatureVec::const_iterator> Island_t;
	typedef std::vector<Island_t> Islands_t;

	Islands_t getIslands(const char *origStr, size_t origStrLen, StoredStringFeatureVec&) const;
private:
	Islands_t searchRange4Island(StoredStringFeatureVec::const_iterator, StoredStringFeatureVec::const_iterator) const;
};

} // namespace barzer
