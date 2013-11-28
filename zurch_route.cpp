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

    std::ostream& os = route.d_rqp.stream();

	try {
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
		os << "{ \"error\": \"invalid options passed\" }\n";
		return 0;
	}


    std::vector<DocFeatureIndex::FeaturesQueryResult> feat;
    const auto& idx = *(route.d_ixl.getIndex());

	const bool empty = !std::strlen(q) || (q[0] == ' ' && q[1] == 0);
	const uint32_t docId = route.d_ixl.getLoader()->getDocIdByName(q);
	if (docId == static_cast<uint32_t>(-1) && !empty)
	{
		os << "{ \"error\": \"unknown document ID is passed\", \"id\": \"" << q << "\" }\n";
		return 0;
	}

    idx.getUniqueFeatures( feat, docId, maxGramSize, uniqueness );

    const auto& universe = *(route.d_ixl.getUniverse());
    std::string entIdStr;
    
    barzer::BarzerRequestParser::ReturnType ret = route.d_rqp.ret;
    bool isJson = ( ret == barzer::BarzerRequestParser::JSON_TYPE || ret == barzer::BarzerRequestParser::XML_TYPE );

    os << "{\n";

	decltype(feat) linked, ents, entless;
	for (const auto& info : feat)
	{
		const auto gsize = info.m_gram.size();
		if (gsize < minGramSize)
			continue;

		if (gsize == 1 &&
				docId != static_cast<uint32_t>(-1) &&
				info.m_gram[0].featureClass == DocFeature::CLASS_ENTITY)
        {
	        const auto& entLinks = route.d_ixl.getLoader()->d_entDocLinkIdx;
			if (auto vec = entLinks.getLinkedEnts(docId)) {
                bool foundId = false;
                const auto theId = info.m_gram[0].featureId;
                for( const auto& i : *vec ) {
                    if( i.first == theId ) {
                        foundId = true;
                        linked.push_back(info);
                        break;
                    }
                }
                if( foundId ) 
                    continue;
            }
        }
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

	const auto positions = idx.getDetailedFeaturesPositions(docId);

	auto printFeatList = [&](const std::vector<DocFeatureIndex::FeaturesQueryResult>& feats) -> void
	{
		bool isFirst = true;
		for (const auto& info : feats)
		{
			const auto gramSize = info.m_gram.size();
			if (!isFirst)
				os << ",";
			isFirst = false;

			os << "    {\n";
			os << "        \"size\": " << gramSize << ",\n";
			os << "        \"uniq\": " << info.m_uniqueness << ",\n";
			os << "        \"grams\": [ {";

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

			if (positions)
			{
				const auto& thisFeaturePos = positions->find(info.m_gram);
				if (thisFeaturePos != positions->end())
				{
					const auto& vec = thisFeaturePos->second;
					bool isFirstPos = true;
					os << "        ,\"poslist\": [ {";
					for (const auto& pair : vec)
					{
						if (!isFirstPos)
							os << ", {";
						isFirstPos = false;

						os << " \"pos\": " << pair.first << ", \"len\": " << pair.second << " }";
					}
					os << " ]\n";
				}
			}

			os << "    }\n";
		}
	};

	os << "\"linked\" : [\n";
	printFeatList(linked);
	os << "],\n" << "\"ents\" : [\n";
	printFeatList(ents);
	os << "],\n" << "\"entless\" : [\n";
	printFeatList(entless);
	os << "]\n";
	os << "}\n";
	return 0;
}

int list_funcs(ZurchRoute& route, const char *q)
{
	auto& os = route.d_rqp.stream();

	os << "[\n";

	const struct
	{
		const char *name;
		const char *desc;
		const char *params;
	} infos[] =
	{
		{ "doc.features", "Returns the features of the given document.", nullptr },
		{ "feature.docs", "Returns the list of the documents matching the given feature.", "uniq,maxgramsize,mingramsize,allents" },
		{ "docid", "Retrieves a document by its ID.", nullptr }
	};
	bool isFirst = true;
	for (auto info : infos)
	{
		if (!isFirst)
			os << ",";
		isFirst = false;
		os << "    {\n";
		os << "        \"name\": \"" << info.name << "\",\n";
		os << "        \"desc\": \"" << info.desc << "\"\n";
		if (info.params)
			os << "        ,\"params\": \"" << info.params << "\"\n";
		os << "    }\n";
	}
	os << "]\n";

	return 0;
}

int defhandler(ZurchRoute& route, const char *q)
{
	auto& os = route.d_rqp.stream();

	os << "{ \"error\": \"unknown route is passed, try `listfuncs` for the list of the routes\" }\n";

	return 0;
}

} // anonymous namespace 

int ZurchRoute::operator()( const char* q )
{
    const char* r = d_rqp.getRoute().c_str();
    switch( r[0] ) {
    case 'd':
        if( d_rqp.isRoute("doc.features") ) 
            return getter_doc_features( *this, q );
        else if( d_rqp.isRoute("docid") )
            return getter_doc_byid( *this, q );
        break;
    case 'f':
        if( d_rqp.isRoute("feature.docs") )
            return getter_doc_features( *this, q );
        break;
    case 'h':
	    if (d_rqp.isRoute("help.zurchroutes"))
            return list_funcs(*this, q);
        break;
    }
    return defhandler(*this, q);

    return 0;
}

} // namespace zurch 
