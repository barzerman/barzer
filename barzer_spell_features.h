#pragma once

#include <vector>
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#include <boost/variant.hpp>
#include <boost/tuple/tuple.hpp>
#include <ay/ay_char.h>
#include <ay/ay_string_pool.h>
#include <barzer_universe.h>

namespace barzer {

typedef std::vector< uint32_t > FeatureStrIdVec;

struct ExtractedStringFeature {
    std::string m_str;
    uint16_t m_offset;
	
	ExtractedStringFeature(const std::string& str, uint16_t off)
	: m_str(str)
	, m_offset(off)
	{
	}
};
/// we assume that any kind of feature is possible to epres with 3 values
/// for example - uint32_t for substring id , and two other 16bit numbers for position and length
/// or if the feature is a double we can store something else in these 8 bytes
///
/// for our string features m_strId is the string ID, m_u41 is string length
/// and m_u42 is string offset.
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

inline bool operator<(const StoredStringFeature& left, const StoredStringFeature& right)
{
	return ay::range_comp()
			.less_than(left.m_strId, left.m_u41, left.m_u42,
				right.m_strId, right.m_u41, right.m_u42);
}

inline bool operator==(const StoredStringFeature& f1, const StoredStringFeature& f2)
{
	return f1.m_strId == f2.m_strId &&
			f1.m_u41 == f2.m_u41 &&
			f1.m_u42 == f2.m_u42;
}

inline size_t hash_value(const StoredStringFeature& f)
{
	return f.m_strId + (f.m_u42 << 16);
}

typedef std::vector<ExtractedStringFeature> ExtractedStringFeatureVec;
typedef std::vector<StoredStringFeature>    StoredStringFeatureVec;

/// feature extractor classes
/// every class must have the following operator defined:
/// void operator()( StringFeatureVec&, const char* str, size_t str_len, int lang );

struct TFE_ngram {
    void operator()( ExtractedStringFeatureVec&, const char* str, size_t str_len, int lang ) const;
};

struct TFE_bastard {
	void operator()( ExtractedStringFeatureVec&, const char* str, size_t str_len, int lang ) const;
};
/// end of feature extractor classes

typedef std::map<StoredStringFeature, std::vector<uint32_t>> InvertedFeatureMap;

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

    TFE_storage( ay::UniqueCharPool&  p ) : d_pool(&p) {}
    
    const InvertedFeatureMap::mapped_type* getSrcsForFeature(const StoredStringFeature& f) const
    {
		const auto pos = d_fm.find(f);
		return pos != d_fm.end() ? &pos->second : 0;
	}
	
	ay::UniqueCharPool* getPool() const { return d_pool; }

	void extractOnly(TFE_TmpBuffers& tmp, const char *str, size_t strLen, int lang) const
	{
		d_extractor(tmp.extractedVec, str, strlen(str), lang);
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
		
		tmp.clear();
		extractOnly(tmp, str, strlen(str), lang);
		
		StoredStringFeature stf;
		for (const auto& feature : tmp.extractedVec)
		{
			featureConvert(stf, feature);
			d_fm[stf].push_back(strId);
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

class FeaturedSpellCorrector
{
	InvertedIdxVarVec m_storages;
public:
	enum class MatchStrategy
	{
		FirstWins,
		BestWins
	} m_matchStrategy;
	
	FeaturedSpellCorrector()
	: m_matchStrategy(MatchStrategy::FirstWins)
	{
	}
	
	void init(ay::UniqueCharPool& p)
	{
		m_storages.push_back(TFE_storage<TFE_ngram>(p));
		m_storages.push_back(TFE_storage<TFE_bastard>(p));
	}
	
	void addHeuristic(const InvertedIdxVar& var)
	{
		m_storages.push_back(var);
	}
	
	void addWord(uint32_t strId, const char *str, int lang);
	uint32_t getBestMatch(const char *str, size_t strLen, int lang);
};

/*
typedef boost::tuple<TFE_storage<TFE_ngram>, TFE_storage<TFE_bastard>> AllInvIdxStoragesTuple;

template<typename Tuple, typename F, size_t pos>
struct foreachStorageImpl
{
	foreachStorageImpl(Tuple& t, F f)
	{
		f(std::get<pos>(t));
		foreachStorageImpl<Tuple, F, pos - 1>(t, f);
	}
};

template<typename Tuple, typename F>
struct foreachStorageImpl<Tuple, F, 0>
{
	foreachStorageImpl(Tuple& t, F f)
	{
		f(std::get<0>(t));
	}
};

template<typename Tuple, typename F>
void foreachStorage(Tuple& t, F f)
{
	foreachStorageImpl<Tuple, F, boost::tuple_size<Tuple>::value - 1>(t, f);
}

template<typename Tuple, typename Heur, typename TElem, size_t pos>
struct HeurIdxHelper : HeurIdxHelper<Tuple, Heur, typename boost::tuple_element<pos - 1, Tuple>::type, pos - 1>
{
	static_assert(pos < boost::tuple_size<Tuple>::value, "tuple element not found");
};

template<typename Tuple, typename Heur, size_t pos>
struct HeurIdxHelper<Tuple, Heur, TFE_storage<Heur>, pos>
{
	enum class ResType : size_t { Result = pos };
};

template<typename Heur, typename Tuple>
TFE_storage<Heur>& findHeuristic(Tuple& t)
{
	return std::get<HeurIdxHelper<Tuple, Heur, typename boost::tuple_element<boost::tuple_size<Tuple>::value - 1, Tuple>::type, boost::tuple_size<Tuple>::value - 1>::ResType::Result>(t);
}

struct WordAdderLambda
{
	const uint32_t m_strId;
	const char * const m_str;
	const uint32_t m_lang;
	
	StoredStringFeatureVec m_storedVec;
    ExtractedStringFeatureVec m_extractedVec;
	TFE_TmpBuffers m_buf;
	
	WordAdderLambda(uint32_t strId, const char *str, uint32_t lang)
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

class FeaturedSpellCorrector
{
	AllInvIdxStoragesTuple m_storages;
public:
	FeaturedSpellCorrector(ay::UniqueCharPool& p)
	: m_storages(std::make_tuple(TFE_storage<TFE_ngram>(p), TFE_storage<TFE_bastard>(p)))
	{
	}
	
	void addWord(uint32_t strId, const char *str, int lang)
	{
		// that would be waaaaaaay more beautiful with real lambdas :(
		foreachStorage(m_storages, WordAdderLambda(strId, str, lang));
	}
};
*/

} // namespace barzer
