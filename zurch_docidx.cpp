#include <zurch_docidx.h>
#include <zurch_phrasebreaker.h>
#include <zurch_settings.h>
#include <barzer_universe.h>
#include <boost/filesystem.hpp>
#include <ay/ay_filesystem.h>
#include <boost/filesystem/fstream.hpp>

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
    feature.serialize(fp);
    docPos.serialize(fp << ",");
    return 0;
}
int ExtractedDocFeature::deserialize( std::istream& fp )
{
    feature.deserialize( fp );
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
: d_classBoosts{ 5, 1, 0.5 }
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
/// place external entity into the pool (add all relevant strings to pool as well)
uint32_t DocFeatureIndex::storeExternalEntity( const barzer::BarzerEntity& ent, const barzer::StoredUniverse& u )
{
    barzer::BarzerEntity newEnt;
    newEnt.eclass = ent.eclass;
    if( const char* s = u.getEntIdString(ent) ) 
        newEnt.tokId = d_stringPool.internIt(s);
    return d_entPool.produceIdByObj( newEnt );
}

uint32_t DocFeatureIndex::storeExternalString( const char* s, const barzer::StoredUniverse& u )
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

size_t DocFeatureIndex::appendDocument( uint32_t docId, const barzer::Barz& barz, size_t numBeads )
{
    const barzer::StoredUniverse* universe = barz.getUniverse() ;
    if( !universe ) 
        return 0;
    
    ExtractedDocFeature::Vec_t featureVec;
    size_t curOffset =0;
    bool neesToInternStems = internStems();

    for( auto i = barz.getBeadList().begin(); i!= barz.getBeadList().end(); ++i, ++curOffset ) {
        if( const barzer::BarzerLiteral* x = i->get<barzer::BarzerLiteral>() ) {
            uint32_t strId = storeExternalString( *x, *universe );
            featureVec.push_back( 
                ExtractedDocFeature( 
                    DocFeature( DocFeature::CLASS_TOKEN, strId ), 
                    FeatureDocPosition(curOffset)
                ) 
            );
        } else if( const barzer::BarzerEntity* x = i->getEntity() ) {
            uint32_t entId = storeExternalEntity( *x, *universe );
            featureVec.push_back( 
                ExtractedDocFeature( 
                    DocFeature( DocFeature::CLASS_ENTITY, entId ), 
                    FeatureDocPosition(curOffset)
                ) 
            );
        } else if( const barzer::BarzerEntityList* x = i->getEntityList() ) {
            for (barzer::BarzerEntityList::EList::const_iterator li = x->getList().begin();li != x->getList().end(); ++li) {
                uint32_t entId = storeExternalEntity( *li, *universe );
                featureVec.push_back( 
                    ExtractedDocFeature( 
                        DocFeature( DocFeature::CLASS_ENTITY, entId ), 
                        FeatureDocPosition(curOffset)
                    ) 
                );
            }
        } else { 
            const barzer::BarzerString* s = (neesToInternStems ? i->get<barzer::BarzerString>():0) ;
            if( s ) {
                const char* theString = (s->stemStr().length() ? s->stemStr().c_str(): s->getStr().c_str());
                uint32_t strId = storeExternalString( theString, *universe );
                featureVec.push_back( 
                    ExtractedDocFeature( 
                        DocFeature( DocFeature::CLASS_STEM, strId ), 
                        FeatureDocPosition(curOffset)
                    ) 
                );
            }
        }
    }
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
size_t DocFeatureIndex::appendDocument( uint32_t docId, const ExtractedDocFeature::Vec_t& v, size_t numBeads )
{
    for( auto i = v.begin(); i!= v.end(); ++i ) {
        const ExtractedDocFeature& f = *i;
        InvertedIdx_t::iterator fi = d_invertedIdx.find( f.feature );
        if( fi == d_invertedIdx.end() ) 
            fi = d_invertedIdx.insert({ f.feature, InvertedIdx_t::mapped_type() }).first;
		
		auto& vec = fi->second;

		const DocFeatureLink link(docId, f.weight());
		auto linkPos = std::find_if(vec.begin(), vec.end(),
				[&link] (const DocFeatureLink& other)
				{ return link.weight == other.weight && link.docId == other.docId; });
		if (linkPos == vec.end())
		{
			vec.push_back(link);
			linkPos = vec.end() - 1;
		}
		
		++linkPos->count;
    }
    return v.size();
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
	
	for (const auto& feature : fVec)
	{
		const auto invertedPos = d_invertedIdx.find(feature.feature);
		if (invertedPos == d_invertedIdx.end())
			continue;
		
		const double classBoost = d_classBoosts[feature.feature.featureClass];
		
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
			
			pos->second += classBoost * (link.weight * (1 + std::log(link.count))) / numSources;
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
        i->first.serialize( fp << "F " << std::dec << i->second.size() << " ");
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
    return 0;
}
int DocFeatureIndex::deserialize( std::istream& fp )
{
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
}

std::ostream& DocFeatureIndex::printStats( std::ostream& fp ) const 
{
    return fp << d_invertedIdx.size();
}

DocFeatureLoader::~DocFeatureLoader() {}
DocFeatureLoader::DocFeatureLoader( DocFeatureIndex& index, const barzer::StoredUniverse& u ) : 
    d_universe(u),
    d_parser(u),
    d_index(index),
    d_bufSz( DEFAULT_BUF_SZ )
{
    d_barz.setUniverse( &d_universe );
}
void DocFeatureLoader::addPieceOfDoc( uint32_t docId, const char* str )
{
    d_parser.parse( d_barz, str, d_qparm );
}
bool DocFeatureLoader::loadProperties( const boost::property_tree::ptree& pt )
{
    // AYLOG(ERROR) << "DocFeatureLoader::loadProperties unimplemented\n";
    return ZurchSettings()( *this, pt );
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
    void operator() ( barzer::Barz& /* same as docLoader.barz() */ ) {
        ++stats.numPhrases;
        docLoader.parseTokenized();

        stats.numFeatureBeads =docLoader.index().appendDocument( docId, docLoader.barz(), stats.numBeads );
        stats.numBeads+= docLoader.barz().getBeads().getList().size();
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

    BarzerTokenizerCB<DocAdderCB> cb( adderCb, parser(), barz(), qparm() );
    d_phraser.breakStream( cb, fp );

    stats = adderCb.stats;

    return stats.numBeads;
}

DocFeatureIndexFilesystem::~DocFeatureIndexFilesystem( ){}
DocFeatureIndexFilesystem::DocFeatureIndexFilesystem( DocFeatureIndex& index, const barzer::StoredUniverse& u ) :
    DocFeatureLoader(index,u)
{}
namespace fs = boost::filesystem;
void DocFeatureIndexFilesystem::addAllFilesAtPath( const char* path )
{
    fs_iter_callback cb( *this );
    ay::dir_regex_iterate( cb, path, d_loaderOpt.regex.c_str(), d_loaderOpt.d_bits.checkBit(LoaderOptions::BIT_DIR_RECURSE) );
}

bool DocFeatureIndexFilesystem::loadProperties( const boost::property_tree::ptree& pt )
{
    // AYLOG(ERROR) << "DocFeatureLoader::loadProperties unimplemented\n";
    return ZurchSettings()( *this, pt );
}

bool DocFeatureIndexFilesystem::fs_iter_callback::operator()( boost::filesystem::directory_iterator& di, size_t depth )
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
        size_t totalNumBeads = index.addDocFromStream( docId, fp, stats );

        stats.print( std::cerr << "ADDED DOC[" << docId << "]|" << docName << "|" ) << "\n";
    } else 
        std::cerr << "cant open " << di->path() << std::endl;
    return true;
}

void DocIndexOnFileSystem::init(const barzer::StoredUniverse& u)
{
    delete loader;
    delete index;

    index   = new DocFeatureIndex();
    loader  = new DocFeatureIndexFilesystem(*index, u );
}

} // namespace zurch
