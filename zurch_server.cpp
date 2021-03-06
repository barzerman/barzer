
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#include <zurch_server.h>
#include <ay_xml_util.h>
#include <barzer_server_response.h>
#include <barzer_json_output.h>
#include <boost/lexical_cast.hpp>

namespace zurch {

/// zurch (docidx) service interface 

std::ostream& DocIdxSearchResponseXML::printHTML( std::ostream& os, const DocWithScoreVec_t& docVec,
		const std::map<uint32_t, DocFeatureIndex::PosInfos_t>& positions ) const 
{
    // return
    ay::tag_raii tag_raii( os, "html");
    tag_raii.push( "body" );
    if( !d_ixl.getLoader() ) {
        AYLOG(ERROR) << "FATAL: loader is NULL";
        tag_raii.text( "Internal Error", "p" );
        return os;
    }
    tag_raii.text( "Question:", "tt" );
    tag_raii.text( d_barz.getOrigQuestion(), "b" );
    os << "<hr/>";
	
	std::vector<DocIndexLoaderNamedDocs::Chunk_t> chunks;
    const DocIndexLoaderNamedDocs& loader = *d_ixl.getLoader();
    const DocFeatureIndex& theIndex = *d_ixl.getIndex();
    bool hasChunks = loader.hasChunks() ;
    bool hasContent = loader.hasContent() ;
    for( auto i= docVec.begin(); i!= docVec.end() ; ++i ) {
        uint32_t docId = i->first;
        const char* docName = d_ixl.getDocName(docId);
        if( docName ) {
            os << "ID(";
            tag_raii.text(docName,"small");
            os << "), SCORE:(" << i->second << ")" << std::endl;
        }
        std::string title = loader.getDocTitle(docId);

        if( title.length() ) 
            tag_raii.text(title, "b");
        os<< "<br/>" << std::endl;

		
        if( hasContent && d_qparm.d_biflags.checkBit(barzer::QuestionParm::QPBIT_ZURCH_FULLTEXT) ) {
            std::string content;
            if (loader.getDocContents(docId, content))
                os<< content << std::endl;
	    }
        os<< "<hr/>" << std::endl;
	    
    }
    return os;
}
std::ostream& DocIdxSearchResponseXML::print( std::ostream& os, const DocWithScoreVec_t& docVec, const std::map<uint32_t, DocFeatureIndex::PosInfos_t>& positions ) const 
{
    if( d_qparm.d_biflags.checkBit(barzer::QuestionParm::QPBIT_ZURCH_HTML) ) {
        return printHTML(os,docVec,positions) ;
    }
    if( !d_ixl.getLoader() ) {
        AYLOG(ERROR) << "FATAL: loader is NULL";
        return os << "<error>Internal Error</error>";
    }
    ay::tag_raii zurchRaii( os, "zurch" );
	
	if (d_barz.getUniverse())
	{
		barzer::BarzStreamerXML streamer(d_barz, *d_barz.getUniverse(), d_qparm);
		streamer.print(os);
	}
	    
	std::vector<DocIndexLoaderNamedDocs::Chunk_t> chunks;
    const DocIndexLoaderNamedDocs& loader = *d_ixl.getLoader();
    const DocFeatureIndex& theIndex = *d_ixl.getIndex();
    bool hasChunks = loader.hasChunks() ;
    bool hasContent = loader.hasContent() ;
    for( auto i= docVec.begin(); i!= docVec.end() ; ++i ) {
        uint32_t docId = i->first;
        const char* docName = d_ixl.getDocName(docId);
        if( i==docVec.begin() )
            os << "\n";
		os << "<doc n=\"";
        if( docName ) 
            ay::XMLStream(os).escape(docName);
        std::string title = loader.getDocTitle(docId);

        os << "\"";
        if( title.length() ) {
            ay::XMLStream(os << " title=\"").escape(title.c_str());
            os << "\"";
        }
        os << " s=\"" << i->second << "\">";
		
        if( hasChunks && !d_qparm.d_biflags.checkBit( barzer::QuestionParm::QPBIT_ZURCH_NO_CHUNKS ) ) {
            os << "\n";
		    chunks.clear();
		    auto pos = positions.find(docId);
		    if (pos != positions.end())
			    loader.getBestChunks(docId, pos->second, 200, 5, chunks);
		    if (!chunks.empty())
		    {
			    os << "  <chunks>\n";
			    for (const auto& chunk : chunks)
				{
					os << "    <chunk>\n";
					for (const auto& item : chunk)
					{
						os << "      <item";
						if (item.m_isMatch)
							os << " m='1'";
						os << ">";
						
						ay::XMLStream(os).escape(item.m_contents) << "</item>\n";
					}
					os << "    </chunk>\n";
				}
			    os << "  </chunks>\n";
		    }
        }
		
        if( hasContent && d_qparm.d_biflags.checkBit(barzer::QuestionParm::QPBIT_ZURCH_FULLTEXT) ) {
            std::string content;
            if (loader.getDocContents(docId, content))
                ay::XMLStream(os << "  <content>\n").escape(content) << "  </content>\n";
	    }
	    
		os << "</doc>\n";
    }
    return os;
}
using ay::json_raii;
std::ostream& DocIdxSearchResponseJSON::print( 
    std::ostream& os, const DocWithScoreVec_t& docVec,
    const std::map<uint32_t, 
    DocFeatureIndex::PosInfos_t>& positions, const DocFeatureIndex::TraceInfoMap_t& traceMap) const 
{
    json_raii raii( os, false, 0 );
	
	if (d_barz.getUniverse() && !d_barz.getTtVec().empty())
	{
		os << "\n\"barz\": ";
		barzer::BarzStreamerJSON streamer(d_barz, *d_barz.getUniverse(),d_qparm);
		streamer.print(os) << ",";

		barzer::CToken::SpellCorrections corrs;

		const auto& bc = d_barz.getBeads();
		for (const auto& item : bc.getList())
		{
			for (const auto& v : item.getCTokens())
			{
				const auto& corr = v.first.getSpellCorrections(); 
				corrs.insert(corrs.end(), corr.begin(), corr.end());
			}
		}

		json_raii spellRaii(raii.startField("spell"), true, 1);
		for (const auto& corr : corrs)
		{
			spellRaii.startField("");
			json_raii itemRaii(os, false, 2);

			ay::jsonEscape(corr.first.c_str(), itemRaii.startField("before"), "\"");
			ay::jsonEscape(corr.second.c_str(), itemRaii.startField("after"), "\"");
		}
	}
	
    json_raii allDocsRaii( raii.startField("docs"), true, 1 );
	
	std::vector<DocIndexLoaderNamedDocs::Chunk_t> chunks;
    const DocIndexLoaderNamedDocs& loader = *d_ixl.getLoader();
    const DocFeatureIndex& theIndex = *d_ixl.getIndex();
    bool hasChunks = loader.hasChunks() ;
    bool hasContent = loader.hasContent() ;
    for( auto i= docVec.begin(); i!= docVec.end() ; ++i ) {
        uint32_t docId = i->first;
        if( d_tagIdxChecker && !d_tagIdxChecker->hasAllTags(docId) )
            continue;
        allDocsRaii.startField("");
        json_raii docRaii( os, false, 2 );
        const char* docName = d_ixl.getDocName(docId);
        if( !docName ) docName= "";
        ay::jsonEscape( docName , docRaii.startField( "n" ), "\"" );

        std::string title = loader.getDocTitle(docId);
        int docWeight = 0;
        if( const DocFeatureIndex::DocInfo * docInfo = theIndex.getDocInfo(docId) ) 
            docWeight = docInfo->extWeight;
        if( title.length()) 
            ay::jsonEscape( title.c_str() , docRaii.startField( "title" ), "\"" );

        docRaii.startField( "w" ) << i->second;
        docRaii.startField( "ew" ) << docWeight;

        if( d_qparm.d_biflags.checkBit(barzer::QuestionParm::QPBIT_ZURCH_TAGS) ) {
            std::vector< std::string > tagz;
            theIndex.d_docTags.visitAllTagsOfId( [&](const std::string& tag ) { tagz.push_back(tag); }, docId );
            if( !tagz.empty() ) {
                docRaii.startField( "tags" );
                json_raii itemRaii( os, true, 2 );
                for( const auto& t : tagz ) 
			        ay::jsonEscape(t.c_str(), itemRaii.startField("",true), "\"");
            }
        }
		
        if( hasChunks ) {
		    chunks.clear();
		    auto pos = positions.find(docId);
		    if (pos != positions.end())
			    loader.getBestChunks(docId, pos->second, 200, 5, chunks);
		    if (!chunks.empty() && !d_qparm.d_biflags.checkBit(barzer::QuestionParm::QPBIT_ZURCH_NO_CHUNKS) )
		    {
			    docRaii.startField("chunks");
			    json_raii chunksRaii(os, true, 2);
			    for (const auto& chunk : chunks)
				{
			        chunksRaii.startField("");
					json_raii singleRaii(os, true, 2);
					for (const auto& item : chunk) {
						if (item.m_isMatch) {
                            // json_raii hlraii(os, false, 2);
                            singleRaii.startField("");
					        json_raii hlraii(os, false, 2);
                            ay::jsonEscape(item.m_contents.c_str(), hlraii.startField("hilite"), "\"" );
                        } else {
						    ay::jsonEscape(item.m_contents.c_str(), singleRaii.startField(""), "\"");
                        }
                    }
				}
		    }
        }

		if (theIndex.isKeepDetailedPositionsEnabled() &&
				!d_qparm.d_biflags.checkBit(barzer::QuestionParm::QPBIT_ZURCH_NO_DETAILED))
		{
			const auto& detailed = theIndex.getDetailedFeaturesPositions(docId);
			if (detailed)
			{
				docRaii.startField("detailed");
				json_raii detailedRaii(os, true, 2);
				for (const auto& pair : *detailed)
				{
					const auto& ngram = pair.first;
					const auto& poslist = pair.second;

					detailedRaii.startField("");

					json_raii singleRaii(os, false, 2);

					{
						singleRaii.startField("gram");
						json_raii nameRaii(os, true, 2);
						for (size_t i = 0, size = ngram.size(); i < size; ++i)
							ay::jsonEscape(theIndex.resolveFeature(ngram[i]).c_str(), nameRaii.startField(""), "\"");
					}

					{
						singleRaii.startField("poslist");
						json_raii listRaii(os, true, 2);
						for (const auto& pair : poslist)
						{
							try
							{
								const auto& posStr = boost::lexical_cast<std::string>(pair.first);
								const auto& lengthStr = boost::lexical_cast<std::string>(pair.second);

								listRaii.startField("");

								json_raii pairRaii (os, false, 2);
								pairRaii.startField("pos") << posStr;
								pairRaii.startField("length") << lengthStr;
							}
							catch (...)
							{
								listRaii.startField("error") << "cannot write strings";
							}
						}
					}
				}
			}
		}

        if( hasContent && d_qparm.d_biflags.checkBit(barzer::QuestionParm::QPBIT_ZURCH_FULLTEXT) ) {
            std::string content;
            if (loader.getDocContents(docId, content))
                ay::jsonEscape(content.c_str(), docRaii.startField("content"), "\"");
        }

        if (d_qparm.d_biflags.checkBit(barzer::QuestionParm::QPBIT_ZURCH_TRACE))
		{
			const auto docTracePos = traceMap.find(docId);
			if (docTracePos != traceMap.end())
			{
				docRaii.startField("scoreTrace");
				json_raii scoreTraceRaii(os, true, 2);
				for (const auto& feature : docTracePos->second)
				{
					scoreTraceRaii.startField("");
					json_raii singleRaii(os, false, 2);
					ay::jsonEscape(feature.resolvedFeature.c_str(), singleRaii.startField("f"), "\"");
					singleRaii.startField("add") << feature.scoreAdd;
					singleRaii.startField("linkWeight") << feature.linkWeight;
					singleRaii.startField("count") << feature.linkCount;
					singleRaii.startField("numSrc") << feature.numSources;
				}
			}
		}
    }
    return os;
}

} // namespace zurch
