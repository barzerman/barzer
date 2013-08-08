#include <zurch_route.h>
#include <zurch_docidx.h>
#include <barzer_universe.h>
#include <barzer_server_request.h>
#include <vector>
#include <barzer_json_output.h>
#include <zurch_server.h>

namespace zurch {

namespace {

int getter_feature_docs( ZurchRoute& route, const char* q ) 
{

    return 0;
}
int getter_doc_byid( ZurchRoute& route, const char* query ) 
{
    std::vector< std::string > docIdStrVec;
    zurch::DocWithScoreVec_t docVec;
    ay::parse_separator( 
        [&] (size_t tok_num, const char* tok, const char* tok_end) -> bool {
            if( tok_end<= tok ) 
                return false;

            docIdStrVec.push_back( std::string(tok, tok_end-tok) );
            return false;
        },
        query, query+strlen(query) 

    );
    for( const auto& i: docIdStrVec ) {
        uint32_t docId  = route.d_ixl.getDocIdByName( i.c_str() );
        if( docId != 0xffffffff )
            docVec.push_back( std::pair<uint32_t,double>(docId, 1.0) );
    }

    barzer::QuestionParm qparm;
    route.d_rqp.getBarz().clear();

    zurch::DocFeatureIndex::TraceInfoMap_t barzTrace;
    std::map<uint32_t, zurch::DocFeatureIndex::PosInfos_t> positions;

    std::ostream& os = route.d_rqp.stream();
    if( route.d_rqp.ret == barzer::BarzerRequestParser::XML_TYPE ) {
        DocIdxSearchResponseXML response( qparm, route.d_ixl, route.d_rqp.getBarz() ); 
        response.print(os, docVec, positions);
    } else if ( route.d_rqp.ret == barzer::BarzerRequestParser::JSON_TYPE ) {
        DocIdxSearchResponseJSON response( qparm, route.d_ixl, route.d_rqp.getBarz() );
        response.print(os, docVec, positions, barzTrace);
    }
    return 0;
}
int getter_doc_features( ZurchRoute& route, const char* q ) 
{
    std::vector<DocFeatureIndex::FeaturesQueryResult> feat;
    const auto& idx = *(route.d_ixl.getIndex());
    uint32_t docId = route.d_ixl.getLoader()->getDocIdByName( q );
    idx.getUniqueFeatures( feat, docId );

    const barzer::StoredUniverse& universe = *(route.d_ixl.getUniverse());
    std::string entIdStr;
    std::ostream& os = route.d_rqp.stream();
    
    barzer::BarzerRequestParser::ReturnType ret = route.d_rqp.ret;
    bool isJson = ( ret == barzer::BarzerRequestParser::JSON_TYPE || ret == barzer::BarzerRequestParser::XML_TYPE );

    os << "{\n";
    bool wasFirst = true;


    size_t numPrinted = 0;
    os << "\"entity\" : [\n";
    for( const auto& featureInfo : feat ) {

        for( size_t j = 0; j< featureInfo.m_gram.size(); ++j ) {
            if( featureInfo.m_gram[j].featureClass == DocFeature::CLASS_ENTITY) {
                entIdStr.clear();
                if( numPrinted++ ) os << ",\n";
                os << "    { \"uniq\": " << featureInfo.m_uniqueness << ", ";
                barzer::BarzerEntity ent = idx.resolve_entity( entIdStr, featureInfo.m_gram[j].featureId, universe );
                barzer::BarzStreamerJSON::print_entity_fields( 
                    os,
                    ent, 
                    universe 
                ) << "}";
            }
        }
    }
    os << "],\n";
    numPrinted=0;
    os << "\"token\" : [\n";
    for( const auto& info : feat ) {
		const auto& ngram = info.m_gram;
        if( ngram[0].featureClass != DocFeature::CLASS_STEM ) 
            continue;
        if( numPrinted++ ) os << ",\n";
            
        os << "{ \"c\":" << info.m_uniqueness << ", \"t\": [";
        size_t ngramElement = 0;
        for( size_t j = 0, j_end = ngram.size(); j < j_end; ++j ) {

            const DocFeature& f = ngram[ j ];
            if( f.featureClass == DocFeature::CLASS_STEM ) {
                if( const char* t = idx.resolve_token(f.featureId) ) {
                    if( ngramElement++ ) os << ", ";
                    ay::jsonEscape( t, os, "\"" );
                }
            }
        }
        os << "]}";
    }
    os << "]\n";
    os << "}\n";
    return 0;
}

} // anonymous namespace 

int ZurchRoute::operator()( const char* q )
{
    if( d_rqp.isRoute("doc.features") ) 
        return getter_doc_features( *this, q );
    else
    if( d_rqp.isRoute("feature.docs") ) 
        return getter_doc_features( *this, q );
    else 
    if( d_rqp.isRoute("docid") ) 
        return getter_doc_byid( *this, q );

    return 0;
}

} // namespace zurch 
