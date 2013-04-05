
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#include <zurch_docidx.h>
#include <zurch_phrasebreaker.h>
#include <zurch_settings.h>
#include <barzer_universe.h>
#include <boost/filesystem.hpp>
#include <ay/ay_filesystem.h>
#include <ay/ay_util_time.h>
#include <ay_tag_markup_parser.h>
#include <boost/filesystem/fstream.hpp>
#include <boost/concept_check.hpp>
#include <barzer_server_request_filter.h>

namespace zurch {

namespace {

template <typename T>
struct PropFilter {
    std::string            propName;
    const barzer::ReqFilter*   filter;
    const ay::IdValIndex<T>*      index;

    PropFilter( const std::string& pn, const barzer::ReqFilter* f, const ay::IdValIndex<T>* i ) : 
        propName(pn),
        filter(f), 
        index(i)
    {}
    PropFilter( ): 
        filter(0), 
        index(0)
    {}

    bool operator()( uint32_t docId ) const
        { return ( index && filter && (*filter)(docId, *index) ); }
};

typedef boost::variant<
    PropFilter<int>,        // 0
    PropFilter<double>,     // 1
    PropFilter<std::string> // 2
> PropFilterVar;

struct Filter_visitor : public boost::static_visitor<bool> {
    uint32_t docId;
    Filter_visitor( uint32_t di ) : docId( di ) {}
    template <typename T>
    bool operator() ( const T& pf ) const 
        { return pf( docId ); }
};

typedef std::vector< PropFilterVar > PropFilterVarVec;

bool matchPropFilterVarVec( uint32_t docId, const PropFilterVarVec& vvec )
{
    Filter_visitor vis( docId );
    for( const auto& i : vvec ) {
        if( !boost::apply_visitor( vis, i ) ) {
            return false;
        }
    }
    return true;
}

void formPropFilterVarVec( PropFilterVarVec& vvec, const barzer::ReqFilterCascade& filter, const SimpleIdx& simpleIdx  )
{
    for( const auto& i : filter.filterMap ) {
        if( auto x = i.second.get<int>() ) {
            const SimpleIdx::int_t* ix = simpleIdx.ix_int( i.first );
            if( ix ) 
                vvec.push_back( PropFilterVar( PropFilter<int>(i.first, &(i.second),ix) ) );
        } else 
        if( auto x = i.second.get<double>() ) {
            const SimpleIdx::double_t* ix = simpleIdx.ix_double( i.first );
            if( ix ) 
                vvec.push_back( PropFilterVar( PropFilter<double>(i.first, &(i.second),ix) ) );
        } else
        if( auto x = i.second.get<std::string>() ) {
            const SimpleIdx::string_t* ix = simpleIdx.ix_string( i.first );
            if( ix ) 
                vvec.push_back( PropFilterVar( PropFilter<std::string>(i.first, &(i.second),ix) ) );
        }
    }
}

}

/// feature we keep track off (can be an entity or a token - potentially we will add more classes to it)
int DocFeature::serialize( std::ostream& fp ) const
{
	// TODO
    return 0;
}
int DocFeature::deserialize( std::istream& fp )
{
	// TODO
    return 0;
}

//// position of feature in the document
int FeatureDocPosition::serialize( std::ostream& fp ) const
{
    fp << std::dec << weight << " " << offset.first << " " << offset.second;
    return 0;
}
int FeatureDocPosition::deserialize( std::istream& fp )
{
    fp >> std::dec >> weight >> offset.first >> offset.second;
    return 0;
}
/// every document is a vector of ExtractedDocFeature 
int ExtractedDocFeature::serialize( std::ostream& fp ) const
{
    //feature.serialize(fp);
    docPos.serialize(fp << ",");
    return 0;
}
int ExtractedDocFeature::deserialize( std::istream& fp )
{
    //feature.deserialize( fp );
    char c;
    fp>> c;
    if( c!= ',' ) 
        return 1;
    docPos.deserialize(fp);

    return 0;
}

////  ann array of DocFeatureLink's is stored for every feature in the corpus 
int DocFeatureLink::serialize( std::ostream& fp ) const
{
    fp << std::hex << docId << " " << std::dec << weight;
    return 0;
}
int DocFeatureLink::deserialize( std::istream& fp )
{
    char c;
    fp >> std::hex >> docId >> c >> std::dec >> weight;
    return 0;
}

DocFeatureIndex::DocFeatureIndex() 
: m_meaningsCounter(0)
, d_classBoosts{ 0.5, 0.5, 0.5, 1, 1.5, 3 }
, m_considerFCount(true)
{}

DocFeatureIndex::~DocFeatureIndex() {}

/// given an entity from the universe returns internal representation of the entity 
/// if it can be found and null entity (isValid() == false) otherwise
barzer::BarzerEntity DocFeatureIndex::translateExternalEntity( const barzer::BarzerEntity& ent, const barzer::StoredUniverse& u ) const
{
    barzer::BarzerEntity newEnt;
    newEnt.eclass = ent.eclass;
    if( const char* s = u.getEntIdString(ent) ) 
        newEnt.tokId = d_stringPool.getId( s );
    
    return newEnt;
}

void DocFeatureIndex::addSynonyms(const barzer::StoredUniverse& universe)
{
	const auto& meanings = universe.meanings();
	for (const auto& meaning2words : meanings.getMeaningsToWordsDict())
	{
		const auto& universeIds = meaning2words.second;
		
		std::vector<std::string> words;
		words.reserve(universeIds.size());
		
		for (size_t i = 0; i < universeIds.size(); ++i)
			if (auto s = universe.getStringPool().resolveId(universeIds[i]))
				words.push_back(s);
		
		addSynonymsGroup(words);
	}
}

void DocFeatureIndex::addSynonymsGroup(const std::vector<std::string>& group)
{
	if (group.size() < 2)
		return;
	
	const barzer::WordMeaning meaning(m_meaningsCounter++);
	for (const auto& string : group)
		m_meanings.addMeaning(storeExternalString(string.c_str()), meaning);
}

void DocFeatureIndex::loadSynonyms(const std::string& filename, const barzer::StoredUniverse& universe)
{
	std::cout << "LOADING SYNONYMS FROM " << filename << std::endl;
	auto spell = universe.getBZSpell();
	
	std::ifstream istr(filename.c_str());
	while (istr)
	{
		std::string line;
		std::getline(istr, line);
		
		std::istringstream lineIstr(line);
		std::vector<std::string> words;
		while (lineIstr)
		{
			std::string word;
			lineIstr >> word;
			
			if (word.empty())
				continue;
			
			std::string stem;
			spell->stem(stem, word.c_str());
			words.push_back(stem.empty() ? word : stem);
		}
		if (words.empty())
			continue;
		
		std::cout << words.size() << ' ';
		addSynonymsGroup(words);
	}
	std::cout << std::endl;
}

bool DocFeatureIndex::hasTokenString(uint32_t id) const
{
	const DocFeature f(DocFeature::CLASS_TOKEN, id);
	
	return d_invertedIdx.find(NGram<DocFeature>(f)) != d_invertedIdx.end();
}

bool DocFeatureIndex::hasTokenString(const char *str) const
{
	const auto id = resolveExternalString(str);
	const DocFeature f(DocFeature::CLASS_TOKEN, id);
	
	return d_invertedIdx.find(NGram<DocFeature>(f)) != d_invertedIdx.end();
}

uint32_t DocFeatureIndex::storeOwnedEntity( const barzer::BarzerEntity& ent )
    { return d_entPool.produceIdByObj( ent ); }
uint32_t DocFeatureIndex::storeExternalEntity( const barzer::BarzerEntity& ent, const barzer::StoredUniverse& u )
{
    barzer::BarzerEntity newEnt;
    newEnt.eclass = ent.eclass;
    if( const char* s = u.getEntIdString(ent) ) 
        newEnt.tokId = d_stringPool.internIt(s);
    return d_entPool.produceIdByObj( newEnt );
}

uint32_t DocFeatureIndex::storeExternalString( const char* s )
{
    return d_stringPool.internIt(s);
}

uint32_t DocFeatureIndex::storeExternalString( const barzer::BarzerLiteral& l, const barzer::StoredUniverse& u )
{
    if( const char* s = l.toString(u).first ) {
        return d_stringPool.internIt(s);
    } else 
        return 0xffffffff;
}
uint32_t DocFeatureIndex::resolveExternalString( const barzer::BarzerLiteral& l, const barzer::StoredUniverse& u ) const
{
    if( const char* s = l.toString(u).first ) 
        return d_stringPool.getId(s);
    else
        return 0xffffffff;
}

void DocFeatureIndex::setStopWords(const std::vector<std::string>& words)
{
	for (const auto& word : words)
		m_stopWords.insert(storeExternalString(word.c_str()));
}

namespace
{
    size_t getLevenshteynDistFromSource( const barzer::BarzelBead& bead, const char* str, size_t str_sz ) 
    {
        std::string srcTok = bead.getSrcTokensString();
        ay::LevenshteinEditDistance lev; 
        size_t dist = barzer::Lang::getLevenshteinDistance( lev, srcTok.c_str(), srcTok.length(), str, str_sz );
        return dist;
    }

	FeatureDocPosition getPosFromCToks(const barzer::CTWPVec& ctoks)
	{
		FeatureDocPosition pos(-1);
		for (const auto& ctok : ctoks)
		{
			const auto& ttoks = ctok.first.getTTokens();
			for (const auto& ttok : ttoks)
			{
				if (pos.offset.first == static_cast<uint32_t>(-1))
					pos.offset.first = ttok.first.d_origOffset;
				pos.offset.second += ttok.first.d_origLength;
			}
		}
		return pos;
	}
	
	template<typename IndexType, typename EntGetter, typename StringGetter, typename RawStringGetter>
	int vecFiller(IndexType idx,
			EntGetter getEnt, StringGetter getStr, RawStringGetter getRawStr,
			ExtractedDocFeature::Vec_t& featureVec, barzer::Barz& barz)
	{
		const barzer::StoredUniverse* universe = barz.getUniverse() ;
		if( !universe ) 
			return 0;
		
		const size_t maxGramSize = 3;
		
		const auto& stopWords = idx->getStopWords();
		const auto& meanings = idx->getMeaningsStorage();
		
		featureVec.reserve(barz.getBeadList().size() * maxGramSize);

		for( auto i = barz.getBeadList().begin(); i!= barz.getBeadList().end(); ++i ) {
			const auto& fdp = getPosFromCToks(i->getCTokens());
			auto stemId = i->getStemStringId();
		    auto stemStr = universe->getStringPool().resolveId(stemId);
			if (stemId != 0xffffffff)
			{
				stemStr = universe->getStringPool().resolveId(stemId);
				if (stemStr)
					stemId = (idx->*getRawStr)(stemStr);
			}
			
			if( const barzer::BarzerLiteral* x = i->get<barzer::BarzerLiteral>() ) {
				const auto& ltrlStr = x->toString(*universe);
                if( !ltrlStr.first ) 
                    continue;
				uint32_t strId = stemId == 0xffffffff ? (idx->*getStr)( *x, *universe ) : stemId;
                bool correctionCancelled = false;

                if( i->hasSpellCorrections() ) {
                    size_t ltrlLev = getLevenshteynDistFromSource( *i, ltrlStr.first, ltrlStr.second );
                    std::string srcString = i->getSrcTokensString();

                    if( !strchr( srcString.c_str(), ' ' ) ) {
                        std::string stem; 
                        ay::LevenshteinEditDistance lev;

                        barz.getUniverse()->stem( stem, srcString.c_str() );
                        
                        if( stem.empty() ) 
                            stem = srcString;

                        uint32_t stemStrId  = idx->resolveExternalString(stem.c_str());    
                        if( stemStrId!= 0xffffffff && idx->hasTokenString( stemStrId) ) {
                            size_t stemLev = barzer::Lang::getLevenshteinDistance( lev, stem.c_str(), stem.length(), srcString.c_str(), srcString.length()) ;
                            if( stemLev <= ltrlLev ) { // if stem is close enough to the original string we go with stem
                                strId = stemStrId;
                                correctionCancelled = true;
                                i->setAtomicData( barzer::BarzerString(srcString) );
                                i->cancelCorrections();
                            }
                        }
                    }
                }
                const bool isPunct = x->isPunct() || (ltrlStr.second == 1 && ispunct(ltrlStr.first[0]));
				if ( !correctionCancelled && ( x->isStop() || isPunct || x->isBlank() || !ltrlStr.second) )
					continue;
                
				featureVec.push_back( 
					ExtractedDocFeature( 
						DocFeature( DocFeature::CLASS_STEM, strId ), 
						fdp
					) 
				);
			} else if( const barzer::BarzerEntity* x = i->getEntity() ) {
				uint32_t entId = (idx->*getEnt)( *x, *universe );
				featureVec.push_back( 
					ExtractedDocFeature( 
						DocFeature( DocFeature::CLASS_ENTITY, entId ), 
						fdp
					) 
				);
			} else if( const barzer::BarzerEntityList* x = i->getEntityList() ) {
				for (barzer::BarzerEntityList::EList::const_iterator li = x->getList().begin();li != x->getList().end(); ++li) {
					uint32_t entId = (idx->*getEnt)( *li, *universe );
					featureVec.push_back( 
						ExtractedDocFeature( 
							DocFeature( DocFeature::CLASS_ENTITY, entId ), 
							fdp
						) 
					);
				}
			} else if (const barzer::BarzerString* s = i->get<barzer::BarzerString>()) { 
				if (s->isFluff ())
					continue;
				
				const char *theString = (s->stemStr().length() ? s->stemStr().c_str(): s->getStr().c_str());
				uint32_t strId = (idx->*getRawStr)(theString);//d_stringPool.getId(theString); 
				
				const auto& wm = meanings.getMeanings(strId);
				if (wm.second == 1)
					featureVec.push_back(
						ExtractedDocFeature(
							DocFeature( DocFeature::CLASS_SYNGROUP, wm.first->id ),
							fdp
						)
					);
				else
					featureVec.push_back( 
						ExtractedDocFeature( 
							DocFeature( DocFeature::CLASS_STEM, strId ), 
							fdp
						) 
					);
			} else if (auto dt = i->get<barzer::BarzerDate>()) {
				featureVec.push_back(DocFeature(DocFeature::CLASS_DATETIME, dt->getTime_t()));
			} else if (auto dt = i->get<barzer::BarzerTimeOfDay>()) {
				featureVec.push_back(DocFeature(DocFeature::CLASS_DATETIME, dt->getSeconds()));
			} else if (auto dt = i->get<barzer::BarzerDateTime>()) {
				featureVec.push_back(DocFeature(DocFeature::CLASS_DATETIME, dt->getDate().getTime_t() + dt->getTime().getSeconds()));
			} else if (auto num = i->get<barzer::BarzerNumber>()) {
				featureVec.push_back(DocFeature(DocFeature::CLASS_NUMBER, num->getRealWiden() * 1000));
			}
		}
		
		auto unigramCount = featureVec.size();
		for (size_t gramSize = 2; gramSize <= maxGramSize; ++gramSize)
		{
			if (unigramCount + 1 > gramSize)
				for (size_t i = 0; i < unigramCount - 1; ++i)
				{
					const auto& last = featureVec[i + gramSize - 1].feature[0];
					if (last.featureClass == DocFeature::CLASS_STEM && stopWords.find(last.featureId) != stopWords.end())
						continue;
					
					const auto& source = featureVec[i];
					
					ExtractedDocFeature gram(source.feature[0], source.docPos);
					gram.docPos.offset = source.docPos.offset;
					for (size_t gc = 1; gc < std::min(gramSize, unigramCount - i); ++gc)
					{
						const auto& f = featureVec[i + gc];
						gram.feature.add(f.feature[0]);
						gram.docPos.offset.second = f.docPos.offset.second + f.docPos.offset.first - gram.docPos.offset.first;
					}
					featureVec.push_back(gram);
				}
		}
		
		for (auto pos = featureVec.begin(); pos < featureVec.begin() + unigramCount; )
		{
			const auto& f = pos->feature[0];
			if (f.featureClass == DocFeature::CLASS_STEM && stopWords.find(f.featureId) != stopWords.end())
			{
				pos = featureVec.erase(pos);
				--unigramCount;
			}
			else
				++pos;
		}
		
		return featureVec.size();
	}
}

int DocFeatureIndex::fillFeatureVecFromQueryBarz( ExtractedDocFeature::Vec_t& featureVec, barzer::Barz& barz ) const
{
	return vecFiller(this,
			&DocFeatureIndex::resolveExternalEntity,
			static_cast<uint32_t (DocFeatureIndex::*)(const barzer::BarzerLiteral&, const barzer::StoredUniverse&) const>(&DocFeatureIndex::resolveExternalString),
			static_cast<uint32_t (DocFeatureIndex::*)(const char*) const>(&DocFeatureIndex::resolveExternalString),
			featureVec, barz);
}
int DocFeatureIndex::getFeaturesFromBarz( ExtractedDocFeature::Vec_t& featureVec, barzer::Barz& barz, bool needToInternStems ) 
{
	return vecFiller(this,
			&DocFeatureIndex::storeExternalEntity,
			static_cast<uint32_t (DocFeatureIndex::*)(const barzer::BarzerLiteral&, const barzer::StoredUniverse&)>(&DocFeatureIndex::storeExternalString),
			static_cast<uint32_t (DocFeatureIndex::*)(const char*)>(&DocFeatureIndex::storeExternalString),
			featureVec, barz);
}

size_t DocFeatureIndex::appendOwnedEntity( uint32_t docId, const BarzerEntity& ent )
{
    ExtractedDocFeature::Vec_t featureVec;
    uint32_t eid = storeOwnedEntity(ent);
    featureVec.push_back( ExtractedDocFeature(DocFeature(DocFeature::CLASS_ENTITY,eid)) );
    return appendDocument( docId, featureVec, 0 );
}
size_t DocFeatureIndex::appendDocument( uint32_t docId, barzer::Barz& barz, size_t offset, DocFeatureLink::Weight_t weight )
{
    ExtractedDocFeature::Vec_t featureVec;
    if( !getFeaturesFromBarz(featureVec, barz, internStems()) ) 
        return 0;
	
	for (auto& f : featureVec)
		f.docPos.weight = weight;
    
    return appendDocument( docId, featureVec, offset );
}

/** This function keeps only one link per given doc ID, feature ID and weight,
 * increasing the link count when it encounters the same link another time.
 * 
 * Link weight participates in this uniqueness check so that we can take into
 * account links that have different weights but otherwise are identical (for
 * example, the same word encountered in document title would have higher
 * weight then the word deep in the text). This makes sense since we also take
 * link count in the example later in findDocument().
 */
size_t DocFeatureIndex::appendDocument( uint32_t docId, const ExtractedDocFeature::Vec_t& features, size_t offset )
{
	std::map<NGram<DocFeature>, size_t> sumFCount;
	for (const auto& f : features)
	{
        InvertedIdx_t::iterator fi = d_invertedIdx.find( f.feature );
        if( fi == d_invertedIdx.end() ) 
            fi = d_invertedIdx.insert({ f.feature, InvertedIdx_t::mapped_type() }).first;
		
		auto& vec = fi->second;

		const DocFeatureLink link(docId, f.docPos.weight);
		auto linkPos = std::find_if(vec.begin(), vec.end(),
				[&link] (const DocFeatureLink& other)
				{ return link.weight == other.weight && link.docId == other.docId; });
		if (linkPos == vec.end())
		{
			vec.push_back(link);
			linkPos = vec.end() - 1;
		}
		
		if (m_considerFCount)
			++linkPos->count;
		else
			linkPos->count = 1;
		
		auto pos = f.docPos;
		pos.offset.first += offset;
		linkPos->addPos(pos);
		
		auto sfci = sumFCount.find(f.feature);
		if (sfci == sumFCount.end())
			sfci = sumFCount.insert({ f.feature, 0 }).first;
		++sfci->second;
    }
    
    auto pos = d_doc2topFeature.insert({ docId, { NGram<DocFeature> (), 0 } }).first;
	
	for (const auto& pair : sumFCount)
		if (pair.second > pos->second.second)
			pos->second = pair;
	
    return features.size();
}

void DocFeatureIndex::setTitleLength(uint32_t docId, size_t titleLength)
{
	if (!titleLength)
		return;
	m_titleLengths[docId] = titleLength;
}

void DocFeatureIndex::setConsiderFeatureCount(bool consider)
{
	m_considerFCount = consider;
}

/// should be called after the last doc has been appended . 
void DocFeatureIndex::sortAll()
{
    for( InvertedIdx_t::iterator i = d_invertedIdx.begin(); i!= d_invertedIdx.end(); ++i ) 
        std::sort( i->second.begin(), i->second.end() ) ;
}

void DocFeatureIndex::findDocument( 
    DocFeatureIndex::DocWithScoreVec_t& out,
    const ExtractedDocFeature::Vec_t& fVec, 
    DocFeatureIndex::SearchParm& parm,
	const barzer::Barz& barz
) const
{
    PropFilterVarVec pfvv;
    if( parm.filterCascade ) {
        formPropFilterVarVec( pfvv, *(parm.filterCascade), d_docDataIdx.simpleIdx()  ) ;
    }

	std::map<uint32_t, double> doc2score;
	
	typedef std::pair<PosInfos_t::value_type, double> ScoredFeature_t;
	std::map<uint32_t, std::vector<ScoredFeature_t>> weightedPos;
	
	for (const auto& ngram : fVec)
	{
		const auto invertedPos = d_invertedIdx.find(ngram.feature);
		if (invertedPos == d_invertedIdx.end())
			continue;
		
		auto maxClass = DocFeature::CLASS_STEM;
		for (const auto& f : ngram.feature.getFeatures())
			if (f.featureClass > maxClass)
				maxClass = f.featureClass;
		
		const double classBoost = d_classBoosts[maxClass];
		const int sizeBoost = ngram.feature.size() * ngram.feature.size();
		const int length = ngram.docPos.offset.second;
		
		double lengthPenalty = 1;
		if (ngram.feature.size() == 1 && maxClass == DocFeature::CLASS_TOKEN)
		{
			if (ngram.docPos.offset.second == 1)
				lengthPenalty = 0.125;
			else if (ngram.docPos.offset.second == 2)
			{
				auto str = d_stringPool.resolveId(ngram.feature[0].featureId);
				if (str && (str[0] == 0xd0 || str[0] == 0xd1))
					lengthPenalty = 0.125;
			}
		}
		
		const auto& sources = invertedPos->second;
		
		const auto numSources = 1 + sources.size();
		
		for (const auto& link : sources)
		{
            if( !pfvv.empty()  &&  !matchPropFilterVarVec( link.docId, pfvv ) )
                continue;
			// dumb safeguard, but I think we don't want to fuck up later in logarithm.
			if (link.count <= 0)
				continue;
			
			const auto scoreAdd = lengthPenalty * sizeBoost * classBoost * (1 + link.weight) * (1 + std::log(link.count)) / numSources;
			
			if (parm.doc2pos)
			{
				auto ppos = weightedPos.find(link.docId);
				if (ppos == weightedPos.end())
					ppos = weightedPos.insert({ link.docId, std::vector<ScoredFeature_t>() }).first;
				ppos->second.push_back({ { link.position, link.length }, scoreAdd });
			}
			
			if (parm.f2trace)
			{
				auto fpos = parm.f2trace->find(link.docId);
				if (fpos == parm.f2trace->end())
					fpos = parm.f2trace->insert({ link.docId, TraceInfoMap_t::mapped_type() }).first;
				
				std::string resolved = "{";
				for (size_t i = 0; i < ngram.feature.size(); ++i)
				{
					if (i)
						resolved += ", ";
					resolved += resolveFeature(ngram.feature[i]);
				}
				resolved += "}";
				fpos->second.push_back({ ngram.feature, resolved, scoreAdd, link.weight, link.count, sources.size() });
			}
			
			auto pos = doc2score.find(link.docId);
			if (pos == doc2score.end())
				pos = doc2score.insert({ link.docId, 0 }).first;
			
			pos->second += scoreAdd;
		}
	}
	
	out.clear();
	out.reserve(doc2score.size());
	std::copy(doc2score.begin(), doc2score.end(), std::back_inserter(out));
	
	std::sort(out.begin(), out.end(),
			[] (const DocWithScore_t& l, const DocWithScore_t& r)
				{ return l.second > r.second; });
	if (out.size() > parm.maxBack)
		out.resize(parm.maxBack);
	
	if (parm.doc2pos)
	{
		bool first = true;
		for (const auto& docPair : out)
		{
			const auto docId = docPair.first;
			auto scored = weightedPos[docId];
			std::sort(scored.begin(), scored.end(),
					[](const ScoredFeature_t& l, const ScoredFeature_t& r) { return l.second > r.second; });
			
			auto pos = parm.doc2pos->insert({ docId, PosInfos_t() }).first;
			for (const auto& score : scored)
				pos->second.push_back(score.first);
		}
	}
}

namespace {

struct EntSerializer {
    char buf[256]; // these are tiny entities
    bool operator()( size_t& sz, std::istream& fp ) {
        fp.getline( buf, sizeof(buf), '\n' );
        buf[ sizeof(buf)-1 ] = 0;
        sz = atoi( buf ) ;
        return true;
    }
    bool operator()( barzer::BarzerEntity& ent, std::istream& fp ) 
        { fp >> std::dec >> ent.eclass.ec >> ent.eclass.subclass >> std::hex >> ent.tokId; return true; }
    bool operator()( std::ostream& fp, size_t sz ) const 
        { fp << sz << '\n'; return true; }
    bool operator()( std::ostream& fp, const barzer::BarzerEntity& ent ) const 
        { fp << std::dec << ent.eclass.ec << ent.eclass.subclass << std::hex << ent.tokId; return true;}

};
} // anonymous namespace 
int DocFeatureIndex::serialize( std::ostream& fp ) const
{
	/*
    fp << "BEGIN_STRINGPOOL\n";
    d_stringPool.serialize(fp);
    fp << "END_STRINGPOOL\n";

    fp << "BEGIN_ENTPOOL\n";
    {
    EntSerializer srlzr;
    d_entPool.serialize(srlzr,fp);
    }
    fp << "END_ENTPOOL\n";

    /// inverted index
    fp << "BEGIN_IDX\n";
    for( auto i = d_invertedIdx.begin(); i!= d_invertedIdx.end(); ++i ) {
        //i->first.serialize( fp << "F " << std::dec << i->second.size() << " ");
        fp << " [ ";
        /// serializing the vector for this feature
        const DocFeatureLink::Vec_t& vec = i->second; 
        for( auto j = vec.begin(); j!= vec.end(); ++j ) {
            if( j!= vec.begin() ) 
                fp << "\n";
            j->serialize(fp);
        }
        fp << " ]";
    }
    fp << "END_IDX\n";
	*/
    return 0;
}
int DocFeatureIndex::deserialize( std::istream& fp )
{
	/*
    std::string tmp;
    if( !(fp>> tmp) || tmp != "BEGIN_STRINGPOOL" ) return 1;
    d_stringPool.deserialize(fp);
    if( !(fp>> tmp) || tmp != "END_STRINGPOOL" ) return 1;

    if( !(fp>> tmp) || tmp != "BEGIN_ENTPOOL" ) return 1;
    {
    EntSerializer entSerializer;
    d_entPool.deserialize(entSerializer,fp);
    }
    if( !(fp>> tmp) || tmp != "END_ENTPOOL" ) return 1;

    if( !(fp>> tmp) || tmp != "BEGIN_IDX" ) return 1;
    size_t errCount = 0;
    DocFeature feature;
    while( fp >> tmp ) {
        if( tmp== "END_IDX" ) 
            break;
        if( tmp =="]" )  /// end of feature links vec ends (end of feature as well)
            continue;

        if( tmp =="[" ){ /// feature links vec starts
            
        } else if( tmp =="F" ) {  /// feature begins
            size_t sz = 0;
            fp >> sz ;
            if( !sz  ) {
                ++errCount;
                continue;
            }
            if( feature.deserialize( fp ) || !feature.isValid() ) {
                ++errCount;
                continue;
            }
            InvertedIdx_t::iterator fi = d_invertedIdx.find(feature);
            if( fi == d_invertedIdx.end() ) {
                fi = d_invertedIdx.insert( 
                    std::pair<DocFeature,DocFeatureLink::Vec_t>(
                        feature,
                        DocFeatureLink::Vec_t()
                    )
                ).first;
            }
            /// this could simply reserve sz but in case this is an artificially produced file with the same feature 
            /// spread across 
            size_t oldSz = fi->second.size();
            fi->second.resize( oldSz + sz );
            for( auto i = fi->second.begin()+oldSz, end = fi->second.end(); i!= end; ++i ) 
                i->deserialize( fp );
        }
    }
    return errCount;
	*/
	return 0;
}

std::ostream& DocFeatureIndex::printStats( std::ostream& fp ) const 
{
    return fp << "Inverse index size: " << d_invertedIdx.size() << std::endl;
}

std::string DocFeatureIndex::resolveFeature(const DocFeature& f) const
{
	switch (f.featureClass)
	{
	case DocFeature::CLASS_STEM:
	case DocFeature::CLASS_TOKEN:
		return d_stringPool.resolveId(f.featureId);
	case DocFeature::CLASS_SYNGROUP:
	{
		const auto words = m_meanings.getWords(f.featureId);
		return words.second ? d_stringPool.resolveId(*words.first) : "<unknown syngroup>";
	}
	case DocFeature::CLASS_ENTITY:
	{
		auto ent = d_entPool.getObjById(f.featureId);
		return ent ? d_stringPool.resolveId(ent->tokId) : "<no entity>";
	}
	case DocFeature::CLASS_DATETIME:
	{
		const auto secs = f.featureId % 86400;
		const auto datePart = f.featureId / 86400;
		barzer::BarzerDateTime dt;
		dt.date.setTime_t(datePart);
		dt.timeOfDay = barzer::BarzerTimeOfDay(secs);
		
		std::ostringstream sstr;
		sstr << dt;
		return sstr.str();
	}
	case DocFeature::CLASS_NUMBER:
	{
		const auto real = f.featureId / 1000.;
		const auto diff = real - f.featureId / 1000;
		
		std::ostringstream sstr;
		if (diff < std::numeric_limits<double>::epsilon())
			sstr << (f.featureId / 1000);
		else
			sstr << real;
		
		return sstr.str();
	}
	default:
		AYLOG(ERROR) << "unknown feature class " << f.featureClass;
		return 0;
	}
}

FeaturesStatItem DocFeatureIndex::getImportantFeatures(size_t count, double skipPerc) const
{
	FeaturesStatItem item;
	item.m_hrText = "important features";
	
	std::vector<FeaturesStatItem::GramInfo> scores;
	scores.reserve(d_invertedIdx.size());
	for (const auto& pair : d_invertedIdx)
	{
		const auto& links = pair.second;
		if (links.empty())
			continue;
		
		// http://www.sureiscute.com/images/50360e401d41c87726000130.jpg :)
		long double score = 1;
		size_t encounters = 0;
		for (const auto& link : links)
		{
			const auto fullPos = d_doc2topFeature.find(link.docId);
			score += std::log(static_cast<double>(link.count) / fullPos->second.second + 1);
			encounters += link.count;
		}
		score /= links.size();
		
		scores.push_back( FeaturesStatItem::GramInfo(pair.first, score, links.size(), encounters));
	}
	
	std::sort(scores.begin(), scores.end(),
			[] (const FeaturesStatItem::GramInfo& l, const FeaturesStatItem::GramInfo& r) { return l.score > r.score; });
	if (count)
	{
		auto start = scores.begin();
		std::advance(start, scores.size() * skipPerc);
		std::copy(start, std::min(start + count, scores.end()),
				std::back_inserter(item.m_values));
	}
	else
		scores.swap(item.m_values);
	
	return item;
}

DocFeatureLoader::DocFeatureLoader( DocFeatureIndex& index, const barzer::StoredUniverse& u ) : 
    d_universe(u),
    d_parser(u),
    d_index(index),
    m_curWeight(0),
    d_bufSz( DEFAULT_BUF_SZ ),
    d_xhtmlMode(ay::XHTML_MODE_HTML),
    d_loadMode(LOAD_MODE_TEXT)
{
    d_barz.setUniverse( &d_universe );
}

DocFeatureLoader::~DocFeatureLoader() {}


void DocFeatureLoader::addPieceOfDoc( uint32_t docId, const char* str )
{
    d_parser.parse( d_barz, str, d_qparm );
}
namespace {
/// this callback is invoked for every phrase in a document 
struct DocAdderCB {
    const uint32_t docId;
    DocFeatureLoader::DocStats stats;

    DocFeatureLoader& docLoader;
    const barzer::QuestionParm& qparm;
	
	const size_t baseOffset;
	size_t bytesHandled;
    
    DocAdderCB( uint32_t did, uint32_t offset, DocFeatureLoader& dl, const barzer::QuestionParm& qp ) : 
        docId(did),
        docLoader(dl), 
        qparm(qp),
        baseOffset(offset),
        bytesHandled(0)
    {}

    void operator() ( BarzerTokenizerCB_data& dta, PhraseBreaker& phraser, barzer::Barz&, size_t offset, const char *s, size_t sLen ) {
        if( !(++stats.numPhrases % 10000) ) {
            std::cerr << ".";
        }
        docLoader.parseTokenized();

        stats.numFeatureBeads += docLoader.index().appendDocument( docId, docLoader.barz(), offset + baseOffset, docLoader.getCurrentWeight() );
        stats.numBeads += docLoader.barz().getBeads().getList().size();
		
		bytesHandled += docLoader.addParsedDocContents(docId, std::string(s, sLen));
    }
};

struct DocAdderXhtmlCB {
	const uint32_t m_docId;
    DocFeatureLoader& loader;
    BarzerTokenizerCB<DocAdderCB>& docAdderCB;

	DocAdderXhtmlCB( DocFeatureLoader& ldr, BarzerTokenizerCB<DocAdderCB>& dacb, uint32_t docId )
	: m_docId(docId)
	, loader(ldr)
	, docAdderCB(dacb)
	{}

    void operator()( const ay::xhtml_parser_state& state, const char* s, size_t s_sz )
    {
        /// ATTENTION!!! here using state we can see whats in the tagStack  and boost confidence if needed etc
        /// we update loader.phraser().state with that

        std::string tmp(s, s_sz);
		ay::unicode_normalize_punctuation(tmp);
		ay::html::unescape_in_place(tmp);
        if( state.isCallbackText() ) 
            loader.phraser().breakBuf( docAdderCB, tmp.c_str(), tmp.size() );
    }
};

}

std::ostream& DocFeatureLoader::DocStats::print( std::ostream& fp ) const
{
    return (
        fp << "PH:" << numPhrases << ", BD:" << numBeads << ", FT:" << numFeatureBeads 
    );
}

std::map<uint32_t, size_t>::iterator DocFeatureLoader::getLastOffset(uint32_t docId) 
{
	auto lastOffsetPos = m_lastOffset.find(docId);
	if (lastOffsetPos == m_lastOffset.end())
		lastOffsetPos = m_lastOffset.insert({ docId, 0 }).first;

    return lastOffsetPos;
}
void DocFeatureLoader::parserSetup()
{
    d_parser.tokenizer.setMax( MAX_QUERY_LEN, MAX_NUM_TOKENS );
    d_parser.lexer.setMaxCTokensPerQuery( MAX_NUM_TOKENS/2 );
    d_parser.lexer.setDontSpell(true);
}
size_t DocFeatureLoader::addDocFromString( uint32_t docId, const std::string& str, DocFeatureLoader::DocStats& stats, bool reuseBarz )
{
    if( !reuseBarz ) {
        d_barz.clear();
        d_parser.tokenize_only( d_barz, str.c_str(), d_qparm );
        parseTokenized();
    }

    auto lastOffsetPos = getLastOffset( docId ); // lastOffsetPos guaranteed to be valid
    stats.numFeatureBeads += d_index.appendDocument( docId, d_barz, lastOffsetPos->second, getCurrentWeight() );
    stats.numBeads += d_barz.getBeads().getList().size();
		
	lastOffsetPos->second += addParsedDocContents(docId, str);
    return stats.numBeads;
}
size_t DocFeatureLoader::addDocFromStream( uint32_t docId, std::istream& fp, DocFeatureLoader::DocStats& stats )
{
    d_phraser.clear();
	
    auto lastOffsetPos = getLastOffset( docId );
	
    DocAdderCB adderCb( docId, lastOffsetPos->second, *this, qparm() );

    parserSetup();

    BarzerTokenizerCB<DocAdderCB> cb( adderCb, parser(), barz(), qparm() );

    if( d_loadMode== LOAD_MODE_TEXT )
        d_phraser.breakStream( cb, fp );
    else 
    if( d_loadMode== LOAD_MODE_XHTML ) {
        DocAdderXhtmlCB xhtmlCb( *this, cb, docId );
        ay::xhtml_parser<DocAdderXhtmlCB> parser( fp , xhtmlCb );

        parser.setMode( d_xhtmlMode );
        parser.parse();
        
    } else if( d_loadMode== LOAD_MODE_AUTO ) {
        AYLOG(ERROR) << "LOAD_MODE_AUTO unimplemented" << std::endl;
    }
        
    stats += adderCb.stats;
	
	lastOffsetPos->second += adderCb.bytesHandled;

    return stats.numBeads;
}

void DocFeatureLoader::addDocContents(uint32_t docId, const char* contents )
{
    m_docs[ docId ] = contents;
}

void DocFeatureLoader::addDocContents(uint32_t docId, const std::string& contents)
{
	auto pos = m_docs.find(docId);
	if (pos == m_docs.end())
		m_docs.insert({ docId, contents });
	else
		pos->second += contents;
}

const char* DocFeatureLoader::getDocContentsStrptr(uint32_t docId) const
{
    auto pos = m_docs.find(docId);
    return( pos == m_docs.end() ? 0 : pos->second.c_str() );
}

bool DocFeatureLoader::getDocContents (uint32_t docId, std::string& out) const
{
	auto pos = m_docs.find(docId);
	if (pos == m_docs.end())
		return false;
	else
	{
		out = pos->second;
		return true;
	}
}

size_t DocFeatureLoader::addParsedDocContents(uint32_t docId, const std::string& parsed )
{
	if (noChunks())
		return 0;
	
	auto pos = m_parsedDocs.find(docId);
	if (pos == m_parsedDocs.end())
	{
		m_parsedDocs.insert({ docId, parsed });
		return parsed.size();
	}
	else
	{
		pos->second += parsed;
		return parsed.size();
	}
}

	typedef std::pair<DocFeatureIndex::PosInfos_t::value_type, uint32_t> WPos_t;
	
namespace
{
	template<typename T>
	T dist (T t1, T t2)
	{
		return t1 > t2 ? t1 - t2 : t2 - t1;
	}
	
	uint32_t computeWeight(const DocFeatureIndex::PosInfos_t& positions, uint32_t pos, size_t chunkLength)
	{
		uint32_t result = 0;
		for (const auto& pair : positions)
			if (dist (pair.first, pos) < chunkLength / 2)
				++result;
		return --result;
	}
	
	std::vector<WPos_t> getKeyPoints(const DocFeatureIndex::PosInfos_t& positions, size_t chunkLength)
	{
		std::vector<WPos_t> weightedPositions;
		for (const auto& pair : positions)
		{
			const auto pos = pair.first;
			if (std::find_if(weightedPositions.begin(), weightedPositions.end(),
					[&pos](const WPos_t& other) { return other.first.first == pos; }) == weightedPositions.end())
				weightedPositions.push_back({ pair, computeWeight (positions, pos, chunkLength) });
		}
		
		std::sort(weightedPositions.begin(), weightedPositions.end(),
				[](const WPos_t& left, const WPos_t& right)
					{ return left.second > right.second; });
		
		for (auto i = weightedPositions.begin(); i < weightedPositions.end(); ++i)
		{
			const auto pos = i->first.first;
			auto other = i + 1;
			while (other < weightedPositions.end())
			{
				if (dist(other->first.first, pos) <= chunkLength)
					other = weightedPositions.erase(other);
				else
					++other;
			}
		}
		return weightedPositions;
	}
	
	DocFeatureIndex::PosInfos_t findInRange(size_t begin, size_t end, const DocFeatureIndex::PosInfos_t& positions)
	{
		DocFeatureIndex::PosInfos_t result;
		for (const auto& pair : positions)
			if (pair.first >= begin && pair.first + pair.second <= end)
				result.push_back(pair);
			
		return result;
	}
	
	void compactify(DocFeatureIndex::PosInfos_t& positions)
	{
		for (auto i = positions.begin(); i < positions.end() - 1; )
		{
			auto next = *(i + 1);
			if (i->first + i->second > next.first)
			{
				next.second += next.first - i->first;
				next.first = i->first;
				i = positions.erase(i);
			}
			else
				++i;
		}
	}
	
	void filterOverlaps(DocFeatureIndex::PosInfos_t& positions)
	{
		typedef DocFeatureIndex::PosInfos_t::value_type PosPair_t;
		std::sort(positions.begin(), positions.end(),
				[](const PosPair_t& l, const PosPair_t& r)
				{
					return l.first == r.first ? l.second > r.second : l.first < r.first;
				});
		for (auto i = positions.begin(); i < positions.end(); ++i)
		{
			const auto pos = i->first;
			const auto length = i->second;
			auto next = i + 1;
			while (next < positions.end ())
			{
				if (dist(next->first, pos) <= length)
					next = positions.erase(next);
				else
					++next;
			}
		}
	}
}

void DocFeatureLoader::getBestChunks(uint32_t docId, const DocFeatureIndex::PosInfos_t& positions,
		size_t chunkLength, size_t count, std::vector<Chunk_t>& chunks) const
{
    if( noChunks() ) 
        return;

	const auto pos = m_parsedDocs.find(docId);
	if (pos == m_parsedDocs.end())
	{
		// AYLOG(ERROR) << "no doc for " << docId;
		return;
	}
	
	const auto& weightedPositions = getKeyPoints(positions, chunkLength);
	
	const std::string& doc = pos->second;
	
	auto expandToSpace = [&doc] (size_t& endIdx) -> void
	{
		while (endIdx != doc.size() && !isspace(doc[endIdx])) ++endIdx;
	};
	
	for (auto i = weightedPositions.begin(), end = std::min(weightedPositions.end(), weightedPositions.begin() + count);
				i < end; ++i)
	{
		const size_t pos = i->first.first;
		
		auto startIdx = pos - std::min(pos, chunkLength / 2);
		while (startIdx && !isspace(doc[startIdx])) --startIdx;
		if (isspace(doc[startIdx]))
			++startIdx;
		
		auto endIdx = startIdx + chunkLength;
		expandToSpace(endIdx);
		
		auto inRange = findInRange(startIdx, endIdx, positions);
		filterOverlaps(inRange);
		
		Chunk_t chunk;
		auto prev = startIdx;
		for (auto& item : inRange)
		{
			if (item.first != prev)
			{
				auto substr = doc.substr(prev, item.first - prev);
				if (!substr.empty() && !isspace(substr[substr.size() - 1]))
					substr.push_back(' ');
				chunk.push_back({ substr, false });
			}
			size_t end = item.second + item.first;
			expandToSpace(end);
			chunk.push_back({ doc.substr(item.first, end - item.first), true });
			prev = end;
		}
		
		if (prev != endIdx)
		{
			auto substr = doc.substr(prev, endIdx - prev);
			if (!isspace(substr[0]))
				substr.insert(0, " ");
			chunk.push_back({ substr, false });
		}
		chunks.push_back(chunk);
	}
}

DocIndexLoaderNamedDocs::~DocIndexLoaderNamedDocs( ) {}
DocIndexLoaderNamedDocs::DocIndexLoaderNamedDocs( DocFeatureIndex& index, const barzer::StoredUniverse& u )
: DocFeatureLoader(index,u),d_entDocLinkIdx(*this)
{}
namespace fs = boost::filesystem;
void DocIndexLoaderNamedDocs::loadEntLinks( const char* fname )
{
    d_entDocLinkIdx.loadFromFile( fname );
}
void DocIndexLoaderNamedDocs::addAllFilesAtPath( const char* path )
{
    fs_iter_callback cb( *this );
	cb.storeFullDocs = hasContent();
    ay::dir_regex_iterate( cb, path, d_loaderOpt.regex.c_str(), d_loaderOpt.d_bits.checkBit(LoaderOptions::BIT_DIR_RECURSE) );
}

bool DocIndexLoaderNamedDocs::fs_iter_callback::operator()( boost::filesystem::directory_iterator& di, size_t depth )
{
    std::string docName;
    if( usePureFileNames ) {
        docName = di->path().parent_path().string() + "/" + di->path().filename().string();
    } else 
        docName = di->path().filename().string();
    uint32_t docId = index.addDocName( docName.c_str() );
    
    fs::ifstream fp(di->path());
    if( fp.is_open() )  {
        DocFeatureLoader::DocStats stats;
		
		ay::stopwatch timer;
		
        size_t totalNumBeads = index.addDocFromStream( docId, fp, stats );

		const auto size = boost::filesystem::file_size(di->path());
		const auto ksize = size / 1024.0;
		const auto time = timer.calcTimeAsDouble();
        stats.print( std::cerr << "ADDED DOC[" << docId << "]|" << docName << "|\t" ) << "\t\t nt: " << ksize / time << " KiB/sec" << "\n";
		
		if (storeFullDocs) {
			fp.seekg(0);
			auto buf = new char [size + 1];
			fp.get(buf, size + 1, 0);
			index.addDocContents(docId, std::string(buf));
			delete buf;
		}
    } else 
        std::cerr << "cant open " << di->path() << std::endl;
    return true;
}

const char* DocIndexAndLoader::getDocContentsByDocName(const char* n) const
{
    uint32_t docId = loader->getDocIdByName(n);
    if( docId == 0xffffffff ) 
        return 0;
    else 
        return loader->getDocContentsStrptr( docId );
}

void DocIndexAndLoader::init(const barzer::StoredUniverse& u)
{
    delete loader;
    delete index;

    index   = new DocFeatureIndex();
    loader  = new DocIndexLoaderNamedDocs(*index, u );
}

void BarzTokPrintCB::operator() ( BarzerTokenizerCB_data& dta, PhraseBreaker& phraser, barzer::Barz& barz, size_t, const char*, size_t )
{
    const auto& ttVec = barz.getTtVec();
    fp << "[" << count++ << "]:" << "(" << ttVec.size()/2+1 << ")";
    for( auto ti = ttVec.begin(); ti != ttVec.end(); ++ti )  {
        if( ti != ttVec.begin() )
            fp << " ";
        fp << ti->first.buf;
    }
    fp << std::endl;
}

} // namespace zurch
