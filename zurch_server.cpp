#include <zurch_server.h>
#include <ay_xml_util.h>

namespace zurch {

/// zurch (docidx) service interface 

std::ostream& DocIdxSearchResponseXML::print( std::ostream& os, const DocFeatureIndex::DocWithScoreVec_t& docVec ) const 
{
    ay::tag_raii( os, "zurch" );
    for( auto i= docVec.begin(); i!= docVec.end() ; ++i ) {
        std::stringstream sstr;
        const char* docName = d_ixl.getDocName(i->first);
        sstr << "n=\"";
        if( docName ) 
            ay::XMLStream(sstr).escape(docName);
        sstr << "\"";
        sstr << "s=\"" << i->second << "\"";
        ay::tag_raii( os, "doc", sstr.str().c_str() );
    }
    return os;
}
using ay::json_raii;
std::ostream& DocIdxSearchResponseJSON::print( std::ostream& os, const DocFeatureIndex::DocWithScoreVec_t& docVec ) const 
{
    json_raii raii( os, false, 0 );
    json_raii allDocsRaii( raii.startField("docs"), true, 1 );
    for( auto i= docVec.begin(); i!= docVec.end() ; ++i ) {
        allDocsRaii.startField("");
        json_raii docRaii( os, false, 2 );
        const char* docName = d_ixl.getDocName(i->first);
        if( !docName ) docName= "";
        ay::jsonEscape( docName , docRaii.startField( "n" ), "\"" );
        docRaii.startField( "w" ) << i->second;
    }
    return os;
}
} // namespace zurch
