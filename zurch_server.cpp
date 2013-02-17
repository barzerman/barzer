
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#include <zurch_server.h>
#include <ay_xml_util.h>

namespace zurch {

/// zurch (docidx) service interface 

std::ostream& DocIdxSearchResponseXML::print( std::ostream& os, const DocFeatureIndex::DocWithScoreVec_t& docVec,
		const std::map<uint32_t, std::vector<uint32_t>>& positions ) const 
{
    if( !d_ixl.getLoader() ) {
        AYLOG(ERROR) << "FATAL: loader is NULL";
        return os << "<error>Internal Error</error>";
    }
    ay::tag_raii zurchRaii( os, "zurch" );
	std::vector<std::string> chunks;
    const DocIndexLoaderNamedDocs& loader = *d_ixl.getLoader();
    bool hasChunks = loader.hasChunks() ;
    bool hasContent = loader.hasContent() ;
    for( auto i= docVec.begin(); i!= docVec.end() ; ++i ) {
        uint32_t docId = i->first;
        const char* docName = d_ixl.getDocName(docId);
		os << "<doc n=\"";
        if( docName ) 
            ay::XMLStream(os).escape(docName);
        std::string title = loader.getDocTitle(docId);

        os << "\"";
        if( title.length() ) {
            ay::XMLStream(os << " title=\"").escape(title.c_str());
            os << "\"";
        }
        os << " s=\"" << i->second << "\">\n";
		
        if( hasChunks ) {
		    chunks.clear();
		    auto pos = positions.find(docId);
		    if (pos != positions.end())
			    loader.getBestChunks(docId, pos->second, 200, 5, chunks);
		    if (!chunks.empty())
		    {
			    os << "\t<chunks>\n";
			    for (const auto& chunk : chunks)
				    ay::XMLStream(os << "\t\t<chunk>").escape(chunk) << "</chunk>\n";
			    os << "\t</chunks>\n";
		    }
        }
		
        if( hasContent) {
            std::string content;
            if (loader.getDocContents(docId, content))
                ay::XMLStream(os << "\t<content>\n").escape(content) << "\t</content>\n";
	    }	
		os << "</doc>\n";
    }
    return os;
}
using ay::json_raii;
std::ostream& DocIdxSearchResponseJSON::print( std::ostream& os, const DocFeatureIndex::DocWithScoreVec_t& docVec,
		const std::map<uint32_t, std::vector<uint32_t>>& positions ) const 
{
    json_raii raii( os, false, 0 );
    json_raii allDocsRaii( raii.startField("docs"), true, 1 );
	std::vector<std::string> chunks;
    const DocIndexLoaderNamedDocs& loader = *d_ixl.getLoader();
    bool hasChunks = loader.hasChunks() ;
    bool hasContent = loader.hasContent() ;
    for( auto i= docVec.begin(); i!= docVec.end() ; ++i ) {
        allDocsRaii.startField("");
        json_raii docRaii( os, false, 2 );
        uint32_t docId = i->first;
        const char* docName = d_ixl.getDocName(docId);
        if( !docName ) docName= "";
        ay::jsonEscape( docName , docRaii.startField( "n" ), "\"" );

        std::string title = loader.getDocTitle(docId);
        if( title.length()) 
            ay::jsonEscape( title.c_str() , docRaii.startField( "title" ), "\"" );

        docRaii.startField( "w" ) << i->second;
		
        if( hasChunks ) {
		    chunks.clear();
		    auto pos = positions.find(docId);
		    if (pos != positions.end())
			    loader.getBestChunks(docId, pos->second, 200, 5, chunks);
		    if (!chunks.empty())
		    {
			    docRaii.startField("chunks");
			    json_raii chunksRaii(os, true, 2);
			    for (const auto& str : chunks)
				    ay::jsonEscape(str.c_str(), chunksRaii.startField(""), "\"");
		    }
        }
        if( hasContent ) {
            std::string content;
            if (loader.getDocContents(docId, content))
                ay::jsonEscape(content.c_str(), docRaii.startField("content"), "\"");
        }
    }
    return os;
}
} // namespace zurch
