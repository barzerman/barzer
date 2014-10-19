#pragma once
#include <barzer_entity.h>
#include <barzer_spell_features.h>
#include <boost/range/iterator_range.hpp>
#include <zurch_docidx_types.h>
#include <ay_util_time.h>
#include <boost/function.hpp>

namespace barzer {

struct BeniEntFormat {
      enum : size_t {
          TOK_CLASS=0,
          TOK_SUBCLASS,
          TOK_ID,
          TOK_RELEVANCE,
          TOK_NAME,
          ///
          TOK_TOPIC_CLASS,
          TOK_TOPIC_SUBCLASS,
          TOK_TOPIC_ID,
          TOK_MAX
      };
      char d_sep;
      static const char* tokName(size_t t);
      std::vector<size_t> d_extraNameTok; // when not blank values of offsets these tokens will be also indexed as name
      size_t d_tokTranslation[TOK_MAX];
      static const char* getTokName( size_t t );
      size_t getTokOffset( size_t tok ) const { return d_tokTranslation[tok];}
      // argument format of the benient tag
      // format: field name - one of the (id, name, extra, relevance)
      void initFormat( const std::string& formatStr );
      BeniEntFormat();
      std::ostream& print( std::ostream& fp ) const;
      void clear();
};

template<typename T>
class NGramStorage
{
	TFE_storage<TFE_ngram> m_gram;

	boost::unordered_multimap<uint32_t, T> m_storage;

    //// this is temporary junk
	StoredStringFeatureVec m_storedVec;
	ExtractedStringFeatureVec m_extractedVec;
    /// end of

	bool m_soundsLikeEnabled;
public:
	typedef boost::function<bool (T)> SearchFilter_f;

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
		m_gram.d_extractor.m_makeSideGrams = true;
		m_gram.d_extractor.m_stem = false;
		m_gram.d_extractor.m_minGrams = 3;
		m_gram.d_extractor.m_maxGrams = 3;
		//m_gram.m_removeDuplicates = false;
	}

	void setSLEnabled(bool enabled=true) { m_soundsLikeEnabled = enabled; }


	void addWord(const char *srcStr, const T& data)
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

	const char* resolveFeature(const StoredStringFeature& f) const
	{
		return m_gram.resolveFeature(f);
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
			std::vector<FindInfo>& out, size_t max, double minCov, const SearchFilter_f& filter = SearchFilter_f()) const
	{
		const double srcFCnt = std::distance(begin, end);

		struct FeatureStatInfo
		{
			double counter;
			uint16_t fCount;
		};
	
		std::vector<FeatureStatInfo> counterMap(m_gram.d_max + 1);

		for (const auto& feature : boost::make_iterator_range(begin, end))
		{
			const auto srcs = m_gram.getSrcsForFeature(feature);
			if (!srcs)
				continue;
		
			const auto imp = 1. / (srcs->size() * srcs->size());
			for (const auto& sourceFeature : *srcs)
			{
				const auto source = sourceFeature.docId;
			
				auto& info = counterMap[source];
				info.counter += imp * sourceFeature.counter;
				info.fCount += sourceFeature.counter;
			}
		}
	
		if (counterMap.empty())
			return;

		typedef std::vector<std::pair<uint32_t, FeatureStatInfo>> Sorted_t;
		Sorted_t sorted;
		sorted.reserve(counterMap.size());
		const auto normalizedMinCov = minCov * srcFCnt;

		for (size_t i = 0; i < counterMap.size(); ++i)
		{
			const auto& info = counterMap[i];
			if (info.fCount < normalizedMinCov)
				continue;

			if (filter)
			{
				const auto data = m_storage.equal_range(i);
				if (data.first == m_storage.end())
					continue;

				bool found = false;
				for (const auto& d : boost::make_iterator_range(data.first, data.second))
					if (filter (d.second))
					{
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
		for (const auto& item : sorted)
		{
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

	void getMatches(const char *origStr, size_t origStrLen, std::vector<FindInfo>& out,
			size_t max, double minCov, const SearchFilter_f& filter = SearchFilter_f()) const
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

	typedef std::pair<StoredStringFeatureVec::const_iterator, StoredStringFeatureVec::const_iterator> Island_t;
	typedef std::vector<Island_t> Islands_t;

	Islands_t getIslands(const char *origStr, size_t origStrLen, StoredStringFeatureVec&) const;
private:
	Islands_t searchRange4Island(StoredStringFeatureVec::const_iterator, StoredStringFeatureVec::const_iterator) const;
};

template<typename T>
auto NGramStorage<T>::getIslands(const char* origStr, size_t origStrLen, StoredStringFeatureVec& storedVec) const -> Islands_t
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

namespace
{
	template<typename Cont>
	Cont intersectVectors(Cont smaller, const Cont& bigger)
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
}

template<typename T>
auto NGramStorage<T>::searchRange4Island(StoredStringFeatureVec::const_iterator begin, StoredStringFeatureVec::const_iterator end) const -> Islands_t
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

class StoredUniverse;


typedef boost::function<bool (BarzerEntity)> BENIFilter_f;

class BENI {
    ay::UniqueCharPool d_charPool;

    NGramStorage<BarzerEntity> d_storage;
	std::map<BarzerEntity, std::string> d_backIdx;
public:
    ay::UniqueCharPool& charPool() { return d_charPool; }
    const ay::UniqueCharPool& charPool() const { return d_charPool; }

    StoredUniverse& d_universe;

	const NGramStorage<BarzerEntity>& getStorage() const;

	void addWord(const std::string& str, const BarzerEntity&);

    /// add all entities by name for given class
    void addEntityClass( const StoredEntityClass& ec );
    /// returns max coverage
    double search( BENIFindResults_t&, const QuestionParm& qparm, const char* str, double minCov, const BENIFilter_f& = BENIFilter_f ()) const;

    void clear() { d_storage.clear(); }
    BENI( StoredUniverse& u );

	BENI(const BENI&) = delete;
	BENI& operator=(const BENI&) = delete;

    static bool normalize_old( std::string& out, const std::string& in, const StoredUniverse* ) ;
    static bool normalize( std::string& out, const std::string& in, const StoredUniverse* ) ;
    void setSL( bool );
};

class SmartBENI {
    enum {
        NAME_MODE_OVERRIDE,
        NAME_MODE_SKIP
    };
    BENI d_beniStraight,  // beni without soundslike normalization
         d_beniSl;        // beni with sounds like normalization
    /// if d_isSL == true d_beniSl has sounds like processed ngrams .. otherwise it's blank
    bool d_isSL;
    StoredUniverse& d_universe;
    StoredUniverse* d_zurchUniverse;

    std::vector< std::pair<boost::regex, std::string> > d_mandatoryRegex;

    void search_single_query(
        BENIFindResults_t& out,
        const QuestionParm& qparm,
        const char* query,
        double minCov,
        Barz* barz,
        const BENIFilter_f& filter,
        size_t maxCount
        ) const;
    void search_post_processing(
        BENIFindResults_t& out,
        double minCov,
        Barz* barz,
        const BENIFilter_f& filter,
        size_t maxCount
        ) const;
public:
    bool hasSoundslike() const { return d_isSL; }
    BENI& beniStraight() { return  d_beniStraight; }
    BENI& beniSl()       { return d_beniSl; }

    typedef boost::unordered_multimap< uint32_t, BarzerEntity > Doc2EntMap;
    Doc2EntMap d_zurchDoc2Ent;

    void clear();
    SmartBENI( StoredUniverse& u );

    void addEntityClass( const StoredEntityClass& ec );
    /// reads info from path . if path ==0 - from stdin
    /// assumes the format as class|subclass|id|searchable name[|topic class|topic subclass|topic id]
    /// when class or subclass are blank the value is taken from defaults
    /// one netity per line. line cant be longer than 1024 bytes
    /// lines with leading # are skipped as comments
    /// if searchable name is blank this line will be used only for entity topic linkage
    /// if topic id is blank nothing will be done with topics
    size_t addEntityFile( const char* path, const char* modeStr, const StoredEntityClass& /*entity defaults*/, const StoredEntityClass& /* topic defaults */ );

    size_t readFormattedEntityFile( const char* path, const BeniEntFormat& format, const char* modeStr, const StoredEntityClass& /*entity defaults*/, const StoredEntityClass& /* topic defaults */ );

    bool hasMandatoryRegex() const {return !d_mandatoryRegex.empty(); }
    void applyMandatoryRegex( std::string& dest ) const;
    void addMandatoryRegex( const std::string& find, const std::string& replace );

    void search( BENIFindResults_t&, const QuestionParm& qparm, const char* str, double minCov, Barz* barz, const BENIFilter_f& = BENIFilter_f (), size_t maxCount=128) const;

	BENI& getPrimaryBENI();

    void linkEntToZurch( const BarzerEntity& ent, uint32_t docId )
        { d_zurchDoc2Ent.insert( { docId, ent } ); }
    void clearZurchLinks()
        { d_zurchDoc2Ent.clear(); }

    template <typename CB>
    void getEntLinkedToZurchDoc( const CB& cb, uint32_t docId ) const
    {
        auto r = d_zurchDoc2Ent.equal_range(docId);
        for( auto i = r.first; i != r.second; ++i )
            cb( i->second );
    }

    // returns the number of entities added to out
    size_t getZurchEntities( BENIFindResults_t& out, const zurch::DocWithScoreVec_t& vec ) const;

    void setZurchUniverse( StoredUniverse* u ) { d_zurchUniverse= u; }

    StoredUniverse* getZurchUniverse() { return d_zurchUniverse; }
    const StoredUniverse* getZurchUniverse() const { return d_zurchUniverse; }
    void zurchEntities( BENIFindResults_t& out, const char* str, const QuestionParm& qparm );
};

class SubclassBENI {
	StoredUniverse& m_universe;

	std::map<StoredEntityClass, BENI*> m_benies;
public:
	SubclassBENI(StoredUniverse& uni)
	: m_universe (uni)
	{
	}

	SubclassBENI(const SubclassBENI&) = delete;
	SubclassBENI& operator=(const SubclassBENI&) = delete;

	~SubclassBENI();

	void clear();

	void addSubclassIds(const StoredEntityClass&, const char *pattern = 0, const char *replace = 0);
	const BENI* getBENI(const StoredEntityClass& x) const
    {
	    const auto pos = m_benies.find(x);
	    return pos == m_benies.end() ? nullptr : pos->second;
    }

    enum {
        ERR_OK,
        ERR_NO_BENI, /// beni for subclass not found

        ERR_MAX
    };
    // returns ERR_XXX
    int search( BENIFindResults_t& out, const QuestionParm& qparm, const char* query, const StoredEntityClass& sc, double minCov=.3 ) const;
};

} // namespace barzer
