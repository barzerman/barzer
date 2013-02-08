#include <zurch_docidx.h>
#include <zurch_phrasebreaker.h>
#include <zurch_settings.h>
#include <barzer_universe.h>
#include <boost/filesystem.hpp>
#include <ay/ay_filesystem.h>
#include <ay/ay_util_time.h>
#include <boost/filesystem/fstream.hpp>
#include <boost/concept_check.hpp>

namespace zurch {

namespace {

inline const char* decodeFeatureClassSerializationId( int x ) {
    if( x == DocFeature::CLASS_ENTITY ) return "e";
    else if( x == DocFeature::CLASS_TOKEN ) return "t";
    else if( x == DocFeature::CLASS_STEM ) return "s";
    else return "s";
}
DocFeature::class_t encodeFeatureClassSerializationId( const char c )
{
    if( c == 'e' ) return DocFeature::CLASS_ENTITY;
    else if( c == 't' ) return DocFeature::CLASS_TOKEN;
    else if( c == 's' ) return DocFeature::CLASS_STEM;
    else return DocFeature::CLASS_STEM;
}

}

/// feature we keep track off (can be an entity or a token - potentially we will add more classes to it)
int DocFeature::serialize( std::ostream& fp ) const
{
    fp << decodeFeatureClassSerializationId( featureClass ) << " " << std::hex << featureId;
    return 0;
}
int DocFeature::deserialize( std::istream& fp )
{
    char s;
    fp >> s;
    featureClass = encodeFeatureClassSerializationId(s);
    fp >> std::hex >> featureId;
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
, d_classBoosts{ 0.5, 0.5, 1, 5 }
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
    if( const char* s = l.toString(u).first ) 
        return d_stringPool.internIt(s);
    else 
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
	template<typename IndexType, typename EntGetter, typename StringGetter, typename RawStringGetter>
	int vecFiller(IndexType idx,
			EntGetter getEnt, StringGetter getStr, RawStringGetter getRawStr,
			const std::set<uint32_t>& stopWords,
			const barzer::MeaningsStorage& meanings,
			ExtractedDocFeature::Vec_t& featureVec, const barzer::Barz& barz)
	{
		const barzer::StoredUniverse* universe = barz.getUniverse() ;
		if( !universe ) 
			return 0;
		
		size_t curOffset =0;
		
		const size_t maxGramSize = 3;
		
		featureVec.reserve(barz.getBeadList().size() * maxGramSize);

		for( auto i = barz.getBeadList().begin(); i!= barz.getBeadList().end(); ++i, ++curOffset ) {
			if( const barzer::BarzerLiteral* x = i->get<barzer::BarzerLiteral>() ) {
				const auto& pair = x->toString(*universe);
				if (!x->isPunct() && pair.second && !(pair.second == 1 && (isspace(pair.first[0]) || ispunct(pair.first[0]))))
				{
					uint32_t strId = (idx->*getStr)( *x, *universe );
					featureVec.push_back( 
						ExtractedDocFeature( 
							DocFeature( DocFeature::CLASS_TOKEN, strId ), 
							FeatureDocPosition(curOffset)
						) 
					);
				}
			} else if( const barzer::BarzerEntity* x = i->getEntity() ) {
				uint32_t entId = (idx->*getEnt)( *x, *universe );
				featureVec.push_back( 
					ExtractedDocFeature( 
						DocFeature( DocFeature::CLASS_ENTITY, entId ), 
						FeatureDocPosition(curOffset)
					) 
				);
			} else if( const barzer::BarzerEntityList* x = i->getEntityList() ) {
				for (barzer::BarzerEntityList::EList::const_iterator li = x->getList().begin();li != x->getList().end(); ++li) {
					uint32_t entId = (idx->*getEnt)( *li, *universe );
					featureVec.push_back( 
						ExtractedDocFeature( 
							DocFeature( DocFeature::CLASS_ENTITY, entId ), 
							FeatureDocPosition(curOffset)
						) 
					);
				}
			} else if (const barzer::BarzerString* s = i->get<barzer::BarzerString>()) { 
				const char *theString = (s->stemStr().length() ? s->stemStr().c_str(): s->getStr().c_str());
				uint32_t strId = (idx->*getRawStr)(theString);//d_stringPool.getId(theString); 
				
				const auto& wm = meanings.getMeanings(strId);
				if (wm.second == 1)
					featureVec.push_back(
						ExtractedDocFeature(
							DocFeature( DocFeature::CLASS_SYNGROUP, wm.first->id ),
							FeatureDocPosition(curOffset)
						)
					);
				else
					featureVec.push_back( 
						ExtractedDocFeature( 
							DocFeature( DocFeature::CLASS_STEM, strId ), 
							FeatureDocPosition(curOffset)
						) 
					);
			}
		}
		
		auto unigramCount = featureVec.size();
		for (size_t gramSize = 2; gramSize <= maxGramSize; ++gramSize)
		{
			if (unigramCount + 1 > gramSize)
				for (size_t i = 0; i < unigramCount - gramSize + 1; ++i)
				{
					const auto& last = featureVec[i + gramSize - 1].feature[0];
					if (last.featureClass == DocFeature::CLASS_STEM && stopWords.find(last.featureId) != stopWords.end())
						continue;
					
					const auto& source = featureVec[i];
					
					ExtractedDocFeature gram(source.feature[0], source.docPos);
					for (size_t gc = 1; gc < gramSize; ++gc)
						gram.feature.add(featureVec[i + gc].feature[0]);
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

int DocFeatureIndex::fillFeatureVecFromQueryBarz( ExtractedDocFeature::Vec_t& featureVec, const barzer::Barz& barz ) const
{
	return vecFiller(this,
			&DocFeatureIndex::resolveExternalEntity,
			static_cast<uint32_t (DocFeatureIndex::*)(const barzer::BarzerLiteral&, const barzer::StoredUniverse&) const>(&DocFeatureIndex::resolveExternalString),
			static_cast<uint32_t (DocFeatureIndex::*)(const char*) const>(&DocFeatureIndex::resolveExternalString),
			m_stopWords,
			m_meanings,
			featureVec, barz);
}
int DocFeatureIndex::getFeaturesFromBarz( ExtractedDocFeature::Vec_t& featureVec, const barzer::Barz& barz, bool needToInternStems ) 
{
	return vecFiller(this,
			&DocFeatureIndex::storeExternalEntity,
			static_cast<uint32_t (DocFeatureIndex::*)(const barzer::BarzerLiteral&, const barzer::StoredUniverse&)>(&DocFeatureIndex::storeExternalString),
			static_cast<uint32_t (DocFeatureIndex::*)(const char*)>(&DocFeatureIndex::storeExternalString),
			m_stopWords,
			m_meanings,
			featureVec, barz);
}

size_t DocFeatureIndex::appendDocument( uint32_t docId, const barzer::Barz& barz, size_t numBeads, DocFeatureLink::Weight_t weight )
{
    ExtractedDocFeature::Vec_t featureVec;
    if( !getFeaturesFromBarz(featureVec, barz, internStems()) ) 
        return 0;
	
	for (auto& f : featureVec)
		f.docPos.weight = weight;
    
    return appendDocument( docId, featureVec, numBeads );
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
size_t DocFeatureIndex::appendDocument( uint32_t docId, const ExtractedDocFeature::Vec_t& features, size_t numBeads )
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
		
		++linkPos->count;
		
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

/// should be called after the last doc has been appended . 
void DocFeatureIndex::sortAll()
{
    for( InvertedIdx_t::iterator i = d_invertedIdx.begin(); i!= d_invertedIdx.end(); ++i ) 
        std::sort( i->second.begin(), i->second.end() ) ;
}

void DocFeatureIndex::findDocument( DocFeatureIndex::DocWithScoreVec_t& out, const char* query, const barzer::QuestionParm& qparm ) const 
{
}

void DocFeatureIndex::findDocument( DocFeatureIndex::DocWithScoreVec_t& out, const ExtractedDocFeature::Vec_t& fVec ) const
{
	std::map<uint32_t, double> doc2score;
	
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
		
		const auto& sources = invertedPos->second;
		
		const auto numSources = sources.size() * sources.size();
		
		for (const auto& link : sources)
		{
			// dumb safeguard, but I think we don't want to fuck up later in logarithm.
			if (link.count <= 0)
				continue;
			
			auto pos = doc2score.find(link.docId);
			if (pos == doc2score.end())
				pos = doc2score.insert({ link.docId, 0 }).first;
			
			pos->second += sizeBoost * classBoost * ((1 + link.weight) * (1 + std::log(link.count))) / numSources;
		}
	}
	
	out.clear();
	out.reserve(doc2score.size());
	std::copy(doc2score.begin(), doc2score.end(), std::back_inserter(out));
	
	std::sort(out.begin(), out.end(),
			[] (const DocWithScore_t& l, const DocWithScore_t& r)
				{ return l.second > r.second; });
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

DocFeatureLoader::~DocFeatureLoader() {}
DocFeatureLoader::DocFeatureLoader( DocFeatureIndex& index, const barzer::StoredUniverse& u ) : 
    d_universe(u),
    d_parser(u),
    d_index(index),
    m_curWeight(0),
    d_bufSz( DEFAULT_BUF_SZ ),
    d_xhtmlMode(ay::xhtml_parser_state::MODE_HTML),
    d_loadMode(LOAD_MODE_TEXT)
{
    d_barz.setUniverse( &d_universe );
}
void DocFeatureLoader::addPieceOfDoc( uint32_t docId, const char* str )
{
    d_parser.parse( d_barz, str, d_qparm );
}
namespace {
/// this callback is invoked for every phrase in a document 
struct DocAdderCB {
    uint32_t docId;
    DocFeatureLoader::DocStats stats;

    DocFeatureLoader& docLoader;
    const barzer::QuestionParm& qparm;
    
    DocAdderCB( uint32_t did , DocFeatureLoader& dl, const barzer::QuestionParm& qp ) : 
        docId(did),
        docLoader(dl), 
        qparm(qp) 
    {}
    void operator() ( BarzerTokenizerCB_data& dta, PhraseBreaker& phraser, barzer::Barz& /* same as docLoader.barz() */ ) {
        if( !(++stats.numPhrases % 10000) ) {
            std::cerr << ".";
        }
        docLoader.parseTokenized();

        stats.numFeatureBeads += docLoader.index().appendDocument( docId, docLoader.barz(), stats.numBeads, docLoader.getCurrentWeight() );
        stats.numBeads += docLoader.barz().getBeads().getList().size();
    }
};

struct DocAdderXhtmlCB {
    DocFeatureLoader& loader;
    BarzerTokenizerCB<DocAdderCB>& docAdderCB;

    DocAdderXhtmlCB( DocFeatureLoader& ldr, BarzerTokenizerCB<DocAdderCB>& dacb ) : loader(ldr), docAdderCB(dacb) {}

    void operator()( const ay::xhtml_parser_state& state, const char* s, size_t s_sz )
    {
        /// ATTENTION!!! here using state we can see whats in the tagStack  and boost confidence if needed etc
        /// we update loader.phraser().state with that

        if( state.isCallbackText() ) 
            loader.phraser().breakBuf( docAdderCB, s, s_sz );
    }
};

}

std::ostream& DocFeatureLoader::DocStats::print( std::ostream& fp ) const
{
    return (
        fp << "PH:" << numPhrases << ", BD:" << numBeads << ", FT:" << numFeatureBeads 
    );
}

size_t DocFeatureLoader::addDocFromStream( uint32_t docId, std::istream& fp, DocFeatureLoader::DocStats& stats )
{
    d_phraser.clear();
    DocAdderCB adderCb( docId, *this, qparm() );

    parser().tokenizer.setMax( MAX_QUERY_LEN, MAX_NUM_TOKENS );
    parser().lexer.setMaxCTokensPerQuery( MAX_NUM_TOKENS/2 );
	parser().lexer.setDontSpell(true);

    BarzerTokenizerCB<DocAdderCB> cb( adderCb, parser(), barz(), qparm() );

    if( d_loadMode== LOAD_MODE_TEXT )
        d_phraser.breakStream( cb, fp );
    else 
    if( d_loadMode== LOAD_MODE_XHTML ) {
        DocAdderXhtmlCB xhtmlCb( *this, cb );
        ay::xhtml_parser<DocAdderXhtmlCB> parser( fp , xhtmlCb );

        parser.setMode( d_xhtmlMode );
        parser.parse();
        
    } else if( d_loadMode== LOAD_MODE_AUTO ) {
        AYLOG(ERROR) << "LOAD_MODE_AUTO unimplemented" << std::endl;
    }
        
    stats += adderCb.stats;
	
	parser().lexer.setDontSpell(false);

    return stats.numBeads;
}

DocIndexLoaderNamedDocs::~DocIndexLoaderNamedDocs( ){}
DocIndexLoaderNamedDocs::DocIndexLoaderNamedDocs( DocFeatureIndex& index, const barzer::StoredUniverse& u ) :
    DocFeatureLoader(index,u)
{}
namespace fs = boost::filesystem;
void DocIndexLoaderNamedDocs::addAllFilesAtPath( const char* path )
{
    fs_iter_callback cb( *this );
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

		const auto ksize = boost::filesystem::file_size(di->path()) / 1024.0;
		const auto time = timer.calcTimeAsDouble();
        stats.print( std::cerr << "ADDED DOC[" << docId << "]|" << docName << "|\t" ) << "\t\t nt: " << ksize / time << " KiB/sec" << "\n";
    } else 
        std::cerr << "cant open " << di->path() << std::endl;
    return true;
}

void DocIndexAndLoader::init(const barzer::StoredUniverse& u)
{
    delete loader;
    delete index;

    index   = new DocFeatureIndex();
    loader  = new DocIndexLoaderNamedDocs(*index, u );
}

void BarzTokPrintCB::operator() ( BarzerTokenizerCB_data& dta, PhraseBreaker& phraser, barzer::Barz& barz )
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
