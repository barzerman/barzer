
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#pragma once

#include <vector>
#include <algorithm>
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#include <boost/variant.hpp>
#include <ay/ay_char.h>
#include <ay/ay_string_pool.h>
#include <barzer_universe.h>

namespace barzer {

struct ExtractedStringFeature {
    std::string m_str;
    uint16_t m_offset;
	
	ExtractedStringFeature(const std::string& str, uint16_t off)
	: m_str(str)
	, m_offset(off)
	{
	}
};

inline bool operator<(const ExtractedStringFeature& left, const ExtractedStringFeature& right)
{
	return left.m_str < right.m_str;
}

/// we assume that any kind of feature is possible to epres with 3 values
/// for example - uint32_t for substring id , and two other 16bit numbers for position and length
/// or if the feature is a double we can store something else in these 8 bytes
///
/// for our string features m_strId is the string ID, m_u41 is string length
/// and m_u42 is string offset.
#pragma pack(push, 1)
struct StoredStringFeature {
	uint32_t m_strId;
	uint16_t m_u41;
	uint16_t m_u42;
	
	StoredStringFeature()
	: m_strId(0xffffffff)
	, m_u41(0)
	, m_u42(0)
	{
	}
	
	StoredStringFeature(uint32_t id, uint16_t u41, uint16_t u42)
	: m_strId(id)
	, m_u41(u41)
	, m_u42(u42)
	{
	}
};
#pragma pack(pop)

inline bool operator<(const StoredStringFeature& left, const StoredStringFeature& right)
{
	return left.m_strId < right.m_strId;
}

inline bool operator==(const StoredStringFeature& f1, const StoredStringFeature& f2)
{
	return f1.m_strId == f2.m_strId;
}

inline size_t hash_value(const StoredStringFeature& f)
{
	return (f.m_strId << 9) + f.m_u42;
}

typedef std::vector<ExtractedStringFeature> ExtractedStringFeatureVec;
typedef std::vector<StoredStringFeature>    StoredStringFeatureVec;
/// feature extractor classes
/// every class must have the following operator defined:
/// void operator()( StringFeatureVec&, const char* str, size_t str_len, int lang );

struct TFE_ngram {
	bool m_stem;
	bool m_makeSideGrams;
	
	size_t m_minGrams;
	size_t m_maxGrams;
	
	TFE_ngram() : m_stem(true), m_makeSideGrams(true), m_minGrams(3), m_maxGrams(3) {}
	
    void operator()( ExtractedStringFeatureVec&, const char* str, size_t str_len, int lang ) const;
};

struct TFE_bastard {
	void operator()( ExtractedStringFeatureVec&, const char* str, size_t str_len, int lang ) const;
};
/// end of feature extractor classes

#pragma pack(push, 1)
struct FeatureInfo
{
	uint32_t docId;
	uint16_t pos;
};
#pragma pack(pop)

inline bool operator==(const FeatureInfo& f1, const FeatureInfo& f2) { return f1.docId == f2.docId && f1.pos == f2.pos; }

typedef std::map<StoredStringFeature, std::vector<FeatureInfo>> InvertedFeatureMap;

struct TFE_TmpBuffers {
    StoredStringFeatureVec& storedVec;
    ExtractedStringFeatureVec& extractedVec;
	
    TFE_TmpBuffers( StoredStringFeatureVec&  sv, ExtractedStringFeatureVec& ev ) :
        storedVec(sv), extractedVec(ev) {}

	void clear()
	{
		storedVec.clear();
		extractedVec.clear();
	}
};
//// stores inverted index for a specific feature extractor
template <typename FE>
struct TFE_storage {
    InvertedFeatureMap d_fm;
    ay::UniqueCharPool *d_pool;
    FE d_extractor;

    typedef boost::unordered_set< uint32_t > StringSet;
    StringSet d_strSet;
	
	bool m_removeDuplicates;

    TFE_storage( ay::UniqueCharPool&  p) : d_pool(&p), m_removeDuplicates(true) {}
    void clear()
    {
        d_fm.clear();
        d_strSet.clear();
    }
    const InvertedFeatureMap::mapped_type* getSrcsForFeature(const StoredStringFeature& f) const
    {
		const auto pos = d_fm.find(f);
		return pos != d_fm.end() ? &pos->second : 0;
	}
	
	ay::UniqueCharPool* getPool() const { return d_pool; }
	
	const char* resolveFeature(const StoredStringFeature& f) const
	{
		auto str = d_pool->resolveId(f.m_strId);
		return str ? str : "<no str>";
	}

	void extractOnly(TFE_TmpBuffers& tmp, const char *str, size_t strLen, int lang) const
	{
		d_extractor(tmp.extractedVec, str, strlen(str), lang);
		
		if (m_removeDuplicates)
		{
			std::set<ExtractedStringFeature> encountered;
			for (auto it = tmp.extractedVec.begin(); it != tmp.extractedVec.end(); )
			{
				if (encountered.find(*it) != encountered.end())
					it = tmp.extractedVec.erase(it);
				else
				{
					encountered.insert(*it);
					++it;
				}
			}
		}
	}
	
	void extractSTF(TFE_TmpBuffers& bufs, const char *str, size_t strLen, int lang) const
	{
		bufs.extractedVec.clear();
		extractOnly(bufs, str, strLen, lang);
		StoredStringFeature stf;
		for (const auto& feature : bufs.extractedVec)
		{
			featureConvert(stf, feature);
			bufs.storedVec.push_back(stf);
		}
	}

    /// extracts all features and adds entries to d_fm strId is id of str - we're passing str
    void extractAndStore( TFE_TmpBuffers& tmp, uint32_t strId, const char* str, int lang )
    {
		if (d_strSet.find(strId) != d_strSet.end())
			return;
		
		d_strSet.insert(strId);
		
		tmp.clear();
		extractOnly(tmp, str, strlen(str), lang);
		
		StoredStringFeature stf;
		for (const auto& feature : tmp.extractedVec)
		{
			featureConvert(stf, feature);
			d_fm[stf].push_back({ strId, stf.m_u42 });
		}
    }

	/// performs simple lookup
	void featureConvert(StoredStringFeature& stf, const ExtractedStringFeature& feature) const
	{
		const auto id = d_pool->internIt(feature.m_str.c_str(), feature.m_str.size());
		stf = StoredStringFeature
		{
			id,
			static_cast<uint16_t>(feature.m_str.size()),
			static_cast<uint16_t>(feature.m_offset)
		};
	}

    FE& extractor() { return d_extractor; }
    const FE& extractor() const { return d_extractor; }
};

enum InvertedIdxVarEnum
{
	NGram,
	Bastard
};

typedef boost::variant<
		TFE_storage<TFE_ngram>,
		TFE_storage<TFE_bastard>
	> InvertedIdxVar;
typedef std::vector<InvertedIdxVar> InvertedIdxVarVec;

struct FeatureCorrectorWordData {
    uint32_t stemStrId;
    size_t   numGlyphsInStem;
    size_t   numGlyphs;
    int      lang;
    FeatureCorrectorWordData() : stemStrId(0xffffffff), numGlyphsInStem(0),numGlyphs(0),lang(LANG_UNKNOWN) {}
    // FeatureCorrectorWordData(uint32_t i, int l ) : stemStrId(i), lang(l){}
    bool hasStem() const { return stemStrId!= 0xffffffff; }
};

template<typename T>
class DataStorage
{
	boost::unordered_map<uint32_t, T> m_storage;
public:
	void add(uint32_t id, const T& t) { m_storage.insert({ id, t }); }
	T* get(uint32_t id) const
	{
		auto pos = m_storage.find(id);
		if (pos != m_storage.end())
			return &pos->second;
		else
			return nullptr;
	}
};

class FeaturedSpellCorrector
{
	InvertedIdxVarVec m_storages;
    boost::unordered_map< uint32_t, FeatureCorrectorWordData > d_wordDataMap;
public:
    const FeatureCorrectorWordData* getWordData(uint32_t i ) const
    {
        boost::unordered_map< uint32_t, FeatureCorrectorWordData >::const_iterator x = d_wordDataMap.find(i);
        return ( x == d_wordDataMap.end() ? 0: &(x->second) );
    }
    void setWordData( uint32_t i, const FeatureCorrectorWordData& fwd  ) 
    {
        boost::unordered_map< uint32_t, FeatureCorrectorWordData >::iterator x = d_wordDataMap.find(i);
        if( x == d_wordDataMap.end() ) 
            d_wordDataMap[i] = fwd;
        else 
            x->second = fwd;
    }


	enum class MatchStrategy
	{
		FirstWins,
		BestWins
	} m_matchStrategy;
	
	FeaturedSpellCorrector()
	: m_matchStrategy(MatchStrategy::BestWins)
	{
	}
	
	void init(ay::UniqueCharPool& p)
	{
		if (!m_storages.empty())
			return;
		
		m_storages.push_back(TFE_storage<TFE_ngram>(p));
		m_storages.push_back(TFE_storage<TFE_bastard>(p));
	}
	
	void addHeuristic(const InvertedIdxVar& var)
	{
		m_storages.push_back(var);
	}
	
	void addWord(uint32_t strId, const char *str, int lang, BZSpell& bzSpell);
	
	struct FeaturedMatchInfo
	{
		uint32_t m_strId;
		int m_levDist;
		
		FeaturedMatchInfo()
		: m_strId(0xffffffff)
		, m_levDist(255)
		{
		}
		
		FeaturedMatchInfo(uint32_t strId, int levDist)
		: m_strId(strId)
		, m_levDist(levDist)
		{
		}
	};
	FeaturedMatchInfo getBestMatch(const char *str, size_t strLen, int lang, size_t maxLevDist);
};


} // namespace barzer
