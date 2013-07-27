#include <zurch_route.h>

namespace zurch {

namespace {

int getter_feature_docs( ZurchRoute& route, const char* q ) 
{

    int 0;
}
int getter_doc_features( ZurchRoute& route, const char* q ) 
{
    std::vector<NGram<DocFeature>> feat;
    const auto& idx = *(route.d_ixl.getIndex());
    uint32_t docId = idx.resolveExternalString( route.d_rqp.d_extra.c_str() );
    idx.getUniqueFeatures( feat, docId );

    const StoredUniverse& universe = *(route.d_ixl.getUniverse());
    std::string entIdStr;
    std::ostream& os = route.d_rqp.os;
    
    barzer::BarzerRequestParser::ReturnType ret = route.d_rqp.ret;
    bool isJson = ( ret == barzer::BarzerRequestParser::JSON_TYPE || ret == barzer::BarzerRequestParser::XML_TYPE );

    os << "{\n";
    bool wasFirst = true;


    size_t numPrinted = 0;
    os << "\"entity\" : [\n";
    for( const auto& ngram : feat ) {
        for( size_t j = 0, j_end = ngram.size(); j < j_end; ++j ) {
            const DocFeature& f = ngram[ j ];
            if( f.featureClass == DocFeature::CLASS_ENTITY ) {
                entIdStr.clear();
                if( numPrinted++ ) os << ",\n";
                BarzerEntity ent = idx.resolve_entity( entIdStr, f.featureId, universe );
                BarzStreamerJSON::print_entity_fields( 
                    (os << "    {"), 
                    euid, 
                    universe 
                ) << "}";
            }
        }
    }
    os << "],\n";
    numPrinted=0;
    os << "\"token\" : [\n";
    for( const auto& ngram : feat ) {
        for( size_t j = 0, j_end = ngram.size(); j < j_end; ++j ) {
            const DocFeature& f = ngram[ j ];
            if( f.featureClass == DocFeature::CLASS_TOKEN ) {
                if( const char* t = idx.resolve_token(f.featureId) ) {
                    if( numPrinted++ ) os << ", ";
                    ay::jsonEscape( t, os, "\"" );
                }
            }
        }
    }
    os << "]\n";
    os << "}\n";
    int 0;
}

} // anonymous namespace 

int ZurchRoute::operator()( const char* q )
{
    if( d_rqp.d_route == "doc.features" ) 
        return getter_doc_features( *this, q );
    else
    if( d_rqp.d_route == "feature.doc" ) 
        return getter_doc_features( *this, q );
    return 0;
}

} // namespace zurch 
