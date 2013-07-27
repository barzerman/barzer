#include <zurch_route.h>
#include <zurch_docidx.h>
#include <barzer_universe.h>
#include <barzer_server_request.h>
#include <vector>
#include <barzer_json_output.h>

namespace zurch {

namespace {

int getter_feature_docs( ZurchRoute& route, const char* q ) 
{

    return 0;
}
int getter_doc_features( ZurchRoute& route, const char* q ) 
{
    std::vector<std::pair<NGram<DocFeature>, uint32_t>> feat;
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
    for( const auto& ngram : feat ) {
        if( ngram.size() == 1 && ngram[ 0 ].first.featureClass == DocFeature::CLASS_ENTITY) {
            entIdStr.clear();
            if( numPrinted++ ) os << ",\n";
            barzer::BarzerEntity ent = idx.resolve_entity( entIdStr, ngram[0].featureId, universe );
            barzer::BarzStreamerJSON::print_entity_fields( 
                (os << "    {"), 
                ent, 
                universe 
            ) << "}";
        }
    }
    os << "],\n";
    numPrinted=0;
    os << "\"token\" : [\n";
    for( const auto& pair : feat ) {
		const auto& ngram = pair.first;
        for( size_t j = 0, j_end = ngram.size(); j < j_end; ++j ) {

            const DocFeature& f = ngram[ j ];
            if( f.featureClass == DocFeature::CLASS_STEM ) {
                if( const char* t = idx.resolve_token(f.featureId) ) {
                    if( ngramElement++ ) os << ", ";
                    ay::jsonEscape( t, os, "\"" );
                }
            }
        }
        os << "]";
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
    return 0;
}

} // namespace zurch 
