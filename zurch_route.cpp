#include <limits>
#include <zurch_route.h>
#include <zurch_docidx.h>
#include <barzer_universe.h>
#include <barzer_server_request.h>
#include <vector>
#include <barzer_json_output.h>
#include <zurch_server.h>
#include <boost/lexical_cast.hpp>

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
	size_t uniqueness = std::numeric_limits<size_t>::max();
	size_t minGramSize = 1;
	size_t maxGramSize = std::numeric_limits<size_t>::max();
	bool allEnts = false;

	try
	{
		const auto& extraMap = route.d_rqp.getExtraMap();

		{
			const auto uPos = extraMap.find("uniq");
			if (uPos != extraMap.end())
				uniqueness = boost::lexical_cast<size_t>(uPos->second);
		}

		{
			const auto gsPos = extraMap.find("maxgramsize");
			if (gsPos != extraMap.end())
				maxGramSize = boost::lexical_cast<size_t>(gsPos->second);
		}

		{
			const auto gsPos = extraMap.find("mingramsize");
			if (gsPos != extraMap.end())
				minGramSize = boost::lexical_cast<size_t>(gsPos->second);
		}

		{
			const auto ePos = extraMap.find("allents");
			if (ePos != extraMap.end())
			{
				const auto& str = ePos->second;
				allEnts = str == "yes" || str == "true" || str == "1";
			}
		}
	}
	catch (...)
	{
	}

    std::vector<DocFeatureIndex::FeaturesQueryResult> feat;
    const auto& idx = *(route.d_ixl.getIndex());
    uint32_t docId = route.d_ixl.getLoader()->getDocIdByName( q );
    idx.getUniqueFeatures( feat, docId, maxGramSize, uniqueness );

    const auto& universe = *(route.d_ixl.getUniverse());
    std::string entIdStr;
    std::ostream& os = route.d_rqp.stream();
    
    barzer::BarzerRequestParser::ReturnType ret = route.d_rqp.ret;
    bool isJson = ( ret == barzer::BarzerRequestParser::JSON_TYPE || ret == barzer::BarzerRequestParser::XML_TYPE );

    os << "{\n";

	decltype(feat) ents, entless;
	for (const auto& info : feat)
	{
		if (info.m_gram.size() < minGramSize)
			continue;

		bool hasEnts = false;
		if (allEnts)
		{
			hasEnts = true;
			for (size_t i = 0, size = info.m_gram.size(); i < size; ++i)
				if (info.m_gram[i].featureClass != DocFeature::CLASS_ENTITY)
				{
					hasEnts = false;
					break;
				}
		}
		else
			for (size_t i = 0, size = info.m_gram.size(); i < size; ++i)
				if (info.m_gram[i].featureClass == DocFeature::CLASS_ENTITY)
				{
					hasEnts = true;
					break;
				}

		if (hasEnts)
			ents.push_back(info);
		else
			entless.push_back(info);
	}

	auto printFeatList = [&](const std::vector<DocFeatureIndex::FeaturesQueryResult>& feats) -> void
	{
		bool isFirst = true;
		for (const auto& info : feats)
		{
			const auto gramSize = info.m_gram.size();
			if (!isFirst)
				os << ",";
			isFirst = false;

			os << "\t{\n";
			os << "\t\t\"size\": " << gramSize << ",\n";
			os << "\t\t\"uniq\": " << info.m_uniqueness << ",\n";
			os << "\t\t\"grams\": [ {";

			bool isFirstUGram = true;
			for (size_t j = 0; j < gramSize; ++j)
			{
				if (!isFirstUGram)
					os << ", {";
				isFirstUGram = false;

				const auto& ugram = info.m_gram[j];
				if (ugram.featureClass == DocFeature::CLASS_ENTITY)
				{
					os << " ";

					entIdStr.clear();
					const auto& ent = idx.resolve_entity(entIdStr, ugram.featureId, universe);
					barzer::BarzStreamerJSON::print_entity_fields(os, ent, universe);
				}
				else
					ay::jsonEscape (idx.resolveFeature(ugram).c_str(), os << "\"text\": ", "\"");

				os << " }";
			}

			os << " ]\n";
			os << "\t}\n";
		}
	};

	os << "\"ents\" : [\n";
	printFeatList(ents);
	os << "],\n" << "\"entless\" : [\n";
	printFeatList(entless);
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
