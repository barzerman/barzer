#include <zurch_server.h>
#include <ay_xml_util.h>

namespace zurch {

/// zurch (docidx) service interface 

std::ostream& DocIdxSearchResponseXML::print( std::ostream& os, const DocFeatureIndex::DocWithScoreVec_t& docVec ) const 
{
    ay::tag_raii zurchRaii( os, "zurch" );
    for( auto i= docVec.begin(); i!= docVec.end() ; ++i ) {
        uint32_t docId = i->first;
        const char* docName = d_ixl.getDocName(docId);
		os << "<doc n=\"";
        if( docName ) 
            ay::XMLStream(os).escape(docName);
        std::string title = d_ixl.getLoader()->getDocTitle(docId);
        os << "\"";
        if( title.length() ) {
            ay::XMLStream(os << " title=\"").escape(title.c_str());
            os << "\"";
        }
        os << " s=\"" << i->second << "\"/>\n";
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
        uint32_t docId = i->first;
        const char* docName = d_ixl.getDocName(docId);
        if( !docName ) docName= "";
        ay::jsonEscape( docName , docRaii.startField( "n" ), "\"" );

        std::string title = d_ixl.getLoader()->getDocTitle(docId);
        if( title.length()) 
            ay::jsonEscape( title.c_str() , docRaii.startField( "title" ), "\"" );

        docRaii.startField( "w" ) << i->second;
    }
    return os;
}
} // namespace zurch
