
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
/*
 * barzer_server_request.cpp
 *
 *  Created on: Apr 21, 2011
 *      Author: polter
 */

#include <barzer_server_request.h>
#include <boost/assign.hpp>
#include <boost/mem_fn.hpp>
#include <cstdlib>
#include <barzer_universe.h>
#include <barzer_autocomplete.h>
#include <barzer_ghettodb.h>
#include <ay/ay_parse.h>
#include <barzer_barzxml.h>
#include <barzer_server_response.h>
#include <barzer_json_output.h>
#include <barzer_geoindex.h>
#include <barzer_el_cast.h>
#include <boost/lexical_cast.hpp>
#include <barzer_server.h>
#include <zurch_docidx.h>
#include <zurch_server.h>

extern "C" {

namespace {

inline const char* getAttr( const char* n, const char** attr, size_t attr_sz )
{
    const char** attr_end = attr + attr_sz;
    for( const char**s = attr; s< attr_end; s+=2 ) {
        if( n[0] == s[0][0] && !strcmp(n,s[0]) ) {
            return s[1];
        }
    }
    return 0;
}

}

// #define STREQ4(x,s) ( 0[x] == 0[s] && 1[x] == 1[s] && 2[x] == 2[s] && 3[x] == 3[s] && !4[x] && !4[s])
#define AY_STREQ4(x,s) ( *((uint32_t*)(x)) == *(uint32_t*)(s)) 
// cast to XML_StartElementHandler
static void startElement(void* ud, const XML_Char *n, const XML_Char **a)
{
	barzer::BarzerRequestParser *rp = (barzer::BarzerRequestParser *)ud;
    if( rp->isXmlInvalid() )
        return;

	const char* name = (const char*)n;
	const char** atts = (const char**)a;
	int attrCount = XML_GetSpecifiedAttributeCount( rp->parser );
	if( attrCount <=0 || (attrCount & 1) ) attrCount = 0; // odd number of attributes is invalid
    
    uint32_t attr_sz = static_cast<size_t>(attrCount);

    if( rp->getBarzXMLParserPtr() ) {
        rp->getBarzXMLParser().takeTag( name, atts , attr_sz, true );
        return;
    } else if( !rp->getTagCount() && AY_STREQ4(name,"barz") ) { // this is an opening barz tag
        const char* universeIdStr= getAttr( "u", atts, attr_sz );
        // uint32_t universeId = 0;
        const barzer::GlobalPools& gpools =rp->getGlobalPools();

        const barzer::StoredUniverse* u = 0;
        if( universeIdStr ) {
            uint32_t universeId = atoi( universeIdStr );
            u = gpools.getUniverse(universeId);
        } 
        if( !u ) {
            rp->setXmlInvalid();
            if( universeIdStr ) 
                rp->stream() << "BarzXML Error: INVALID universe id: " << universeIdStr << std::endl;
            else 
                rp->stream() << "BarzXML Error: Must supply universe in u attribute" << std::endl;
        } else { // valid universe extracted 
            barzer::BarzXMLParser *parser = new barzer::BarzXMLParser( rp->getBarz(), rp->stream(), rp->getGlobalPools(), *u );
            parser->setInternStrings(rp->shouldInternStrings());
            rp->setBarzXMLParserPtr(parser);
            rp->getBarzXMLParser().takeTag( name, atts , attr_sz, true );
        }

        return;
    }
    rp->addTag(name);
    rp->incrementTagCount();

    if( !rp->getUniverse() ) {
        if( 
            (name[0] == 'q' &&(!strcmp(name,"query") || !strcmp(name,"qblock"))) ||
            ((name[0] == 'a' && !strcmp(name,"autoc")) ||
			((name[0] == 'f' && !strcmp(name,"findents")))) 
        ) 
        { /// processing query/qblock - resolving universe and userId early on
            if( !strcmp(name,"query") || !strcmp(name,"qblock") ) {
                for (int i = 0; i < attrCount; i += 2) {
                    const char * attrName = atts[i];
                    const char * attrVal = atts[i+1];
                    if( attrName[0] == 'u' && !attrName[1] ) {
                        int userId = atoi(attrVal);
                        if( !rp->setUniverseId( userId ) ) {
							AYLOG(WARNING) << attrVal << " is not a valid user number\n";
                        }
                    }
                    rp->setAttr(attrName, attrVal );
                }
            }
        } else {
        }
    } else {
    }
    for (int i = 0; i < attrCount; i += 2) {
        rp->setAttr(atts[i], atts[i+1]);
    }
}

// cast to XML_EndElementHandler
static void endElement(void *ud, const XML_Char *n)
{
    barzer::BarzerRequestParser *rp = (barzer::BarzerRequestParser *)ud;
    if( rp->isXmlInvalid() )
        return;
	const char *name = (const char*) n;
    if(  rp->getBarzXMLParserPtr() ) { // barzXML input
        rp->getBarzXMLParser().takeTag( name, 0, 0, false );
        return;
    } else { // standard input
        rp->process(name);
    }
}

// cast to XML_CharacterDataHandler
static void charDataHandle( void * ud, const XML_Char *str, int len)
{
	barzer::BarzerRequestParser *rp = (barzer::BarzerRequestParser *)ud;
    if( rp->isXmlInvalid() )
        return;
    if( rp->getBarzXMLParserPtr() ) {
        rp->getBarzXMLParser().takeCData( str, len );
    } else {
        //const char* s = (const char*)str;
        std::string s((char*) str, len);
        rp->setBody(s);
    }
}
} // extern "C" ends

namespace barzer {

BarzerRequestParser::BarzerRequestParser(GlobalPools &gp, std::ostream &s ) :
    m_internStrings(false),
    gpools(gp), 
    settings(gp.getSettings()), 
    userId(0xffffffff),
    d_universe(0),
    os(s),
    d_aggressiveStem(false),
    d_tagCount(0),
    d_xmlIsInvalid(false),
    d_barzXMLParser(0),
    d_queryId( std::numeric_limits<uint64_t>::max() ),
    d_simplified(false),
    ret(XML_TYPE),
    d_zurchDocIdxId(0xffffffff),
    d_queryType(QType::BARZER)
{parser = 0;}

int BarzerRequestParser::initFromUri( const char* u, size_t u_len, const char* query, size_t query_len ) 
{
    ay::uri_parse uri;
    uri.parse( query, query_len );

    if( !strncmp( u, "/zurch", u_len ) ) {
        setQueryType( QType::ZURCH );
        d_zurchDocIdxId = 0;
        d_zurchSearchById = false;
    } else
        setQueryType(QType::BARZER);

    d_query.clear();
    d_queryFlags.clear();
    ret = ( d_queryType == QType::BARZER ? XML_TYPE : JSON_TYPE );

    for( auto i = uri.theVec.begin(); i!= uri.theVec.end(); ++i )  {
        if( !i->first.length() ) 
            continue;
        switch( i->first[0] ) {
        case 'b':
            if( i->first == "byid" ) {
                d_zurchSearchById = ( i->second != "no" );
            }
            break;
        case 'd':
            if( i->first == "docidx" )  { // zurch docid 
                d_zurchDocIdxId= atoi( i->second.c_str() );  
            }
            break;
        case 'q':
            if( i->first == "q" ) 
                d_query = i->second;
            break;
        case 'f': /// flag
            if( i->first == "flag" ) {
                d_queryFlags = i->second;
            } else if( i->first == "flt" ) {
                fitlerCascade.parse( i->second.c_str(), i->second.c_str()+i->second.length() );
            }
            break;
        case 'r':
            if( i->first == "ret" )  {
                if( i->second == "json" ) {
                    d_simplified = false;
                    ret = JSON_TYPE;
                } else 
                if( i->second == "sjson" ) {
                    d_simplified = true;
                    ret = JSON_TYPE;
                } else
                if( i->second == "xml" ) {
                    ret = XML_TYPE;
                }
            }
            break;
        case 'u': 
            if( i->first =="u" ) {
                userId = static_cast<uint32_t>( atoi(i->second.c_str() ) );
	            setUniverse(gpools.getUniverse(userId));
                if( !d_universe ) 
                    return 1;
            }
            break;
        }
    }
    return 0;
}
int BarzerRequestParser::parse()
{
    if( !d_universe ) {
        printError( "User not found" );
        return 0;
    }
    switch( d_queryType ) {
    case QType::BARZER:
        raw_query_parse( d_query.c_str() );
        break;
    case QType::ZURCH:
        raw_query_parse_zurch( d_query.c_str(), *d_universe );
        break;
    }
    return 0;
}

BarzerRequestParser::BarzerRequestParser(GlobalPools &gp, std::ostream &s, uint32_t uid ) : 
    m_internStrings(false),
    gpools(gp), 
    settings(gp.getSettings()), 
    userId(uid)/* qparser(u), response(barz, u) */, 
    d_universe(0),
    os(s),
    d_aggressiveStem(false),
    d_tagCount(0),
    d_xmlIsInvalid(false),
    d_barzXMLParser(0),
    d_queryId( std::numeric_limits<uint64_t>::max() ),
    d_simplified(false),
    ret(XML_TYPE),
    d_zurchDocIdxId(0xffffffff),
    d_zurchSearchById(false),
    d_queryType(QType::BARZER)
{
    parser = XML_ParserCreate(NULL);
    XML_SetUserData(parser, this);
    XML_SetElementHandler(parser, &startElement, &endElement);
    XML_SetCharacterDataHandler(parser, &charDataHandle);
}

BarzerRequestParser::~BarzerRequestParser()
{
    if( parser )
	    XML_ParserFree(parser);
    if( d_barzXMLParser )
        delete d_barzXMLParser;
}

typedef boost::function<void(BarzerRequestParser*, BarzerRequestParser::RequestTag&)> ReqTagFunc;
typedef std::map<std::string,ReqTagFunc> TagFunMap;


#define CMDFUN(n) (#n, boost::mem_fn(&BarzerRequestParser::tag_##n))
static const ReqTagFunc* getCmdFunc(std::string &name) {
	static TagFunMap funmap = boost::assign::map_list_of
			CMDFUN(autoc) // autocomplete query (can be contained in qblock)
			CMDFUN(findents) // find nearest entities query
			CMDFUN(prop) // property filter block
			CMDFUN(pf) // property filter child of prop
			CMDFUN(qblock) // can envelop various tags (query/autoc/var/nameval) 
			CMDFUN(query)  // traditional barer query (semantic search)
			CMDFUN(nameval) // request for a ghettodb value
			CMDFUN(cmd)     
			CMDFUN(rulefile)
			CMDFUN(topic)
			CMDFUN(trie)
			CMDFUN(user)
			CMDFUN(var)
			;
			//("query", boost::mem_fn(&BarzerRequestParser::command_query));

	TagFunMap::const_iterator fi = funmap.find(name);
	if (fi == funmap.end()) return 0;
	return &(fi->second);
}
#undef CMDFUN

void BarzerRequestParser::setInternStrings (bool should)
{
	m_internStrings = should;
}

bool BarzerRequestParser::shouldInternStrings() const
{
	return m_internStrings;
}

const StoredUniverse*  BarzerRequestParser::setUniverseId( int id )
{
    userId = id;
	setUniverse(gpools.getUniverse(id));
    return d_universe;
}

void BarzerRequestParser::setUniverse (const StoredUniverse* u)
{
	d_universe = u;
	barz.setUniverse(d_universe);
}

void BarzerRequestParser::setBody(const std::string &s) 
{
    if( tagStack.empty() ) {
    } else {
	    // getTag().body.append(s);
	    tagStack.back().body.append(s);
    }
}

void BarzerRequestParser::process(const char *name) {
    if( tagStack.empty() ) 
        return;

	RequestTag &t = getTag();
	if (t.tagName == name) {
		const ReqTagFunc *fun = getCmdFunc(t.tagName);
		if (fun) {
			(*fun)(this, t);
		}
	} else {
		AYLOG(ERROR) << "Tag mismatch. expected: '" << t.tagName
		<< "'; got:'" << name << "'.";
	}
	tagStack.pop_back();
}

template<class T>struct CmdProc : public boost::static_visitor<> {
	BarzerRequestParser &parser;
	CmdProc(BarzerRequestParser &p) : parser(p) {}
	void process(CmdArgList &list) {
		for (CmdArgList::iterator it = list.begin(); it != list.end(); ++it) {
			boost::apply_visitor(*static_cast<T*>(this), *it);
		}
	}
};

struct CmdAdd : public CmdProc<CmdAdd> {
	CmdAdd(BarzerRequestParser &p) : CmdProc<CmdAdd>(p) {}
	void operator()(const Rulefile &rf) {
        BELReader reader( parser.getGlobalPools(), &(parser.stream()) );
		parser.getSettings().addRulefile(reader,rf );
		//parser.stream() << "<rulefile>!!";
	}
    template <typename T>
    void operator()( const T& t )
    {
        parser.stream() << "invalid add\n";
    }
    /*
	void operator()(const TrieId &tid) {
        
		BarzerSettings &s = parser.getSettings();
		User &u = s.createUser(tid.userId);
		u.addTrie(tid.path,false,0);
		
	}
    */
	void operator()(const UserId &uid) {
		parser.getSettings().createUser(uid.id);
		//parser.stream() << "<userid>!!";
	}
};
struct CmdClear : public CmdProc<CmdClear> {
	CmdClear(BarzerRequestParser &p) : CmdProc<CmdClear>(p) {}
	template <typename T>
	void operator() ( const T& ) { }
	void operator()(const UserId &uid) {
		StoredUniverse * up = parser.getGlobalPools().getUniverse(uid.id);
		if( up ) 
			up->clear();
		else {
			parser.stream() << "<error>invalid user id " << uid.id << "</error>";
		}
	}
	void operator()(const TrieId &tid) {
		BELTrie* trie = parser.getGlobalPools().getTrie( tid.path.first.c_str(), tid.path.second.c_str() );
		if( trie ) {
			BELTrie::WriteLock w_lock(trie->getThreadLock());

			if( trie->getNumUsers() ) 
				trie->clear();
			else {
				parser.stream() << "<error>trie " << tid.path.first << ':' << tid.path.second << " used by multiple users. Deregister them first.</error>";
			}
		} else {
			parser.stream() << "<error>invalid trie id " << tid.path.first << ':' << tid.path.second << "</error>";
		}
	}

};

void BarzerRequestParser::tag_cmd(RequestTag &tag) {
    os << "<cmdoutput>\n";
	AttrList::iterator it = tag.attrs.find("name");
	if (it == tag.attrs.end() ) {
		os << "<error>unknown cmd name</error>";
		arglist.clear();
		os << "</cmdoutput>\n";
		return;
	}
	if(it->second == "add") {
		CmdAdd cp(*this);
		cp.process(arglist);
	} else if( it->second == "clear" ) {
		CmdClear cp(*this);
		cp.process(arglist);
	}
	arglist.clear();
	os << "</cmdoutput>\n";
}

void BarzerRequestParser::tag_user(RequestTag &tag) {
	//RequestTag &tag = getTag();
	AttrList &attrs = tag.attrs;
	AttrList::iterator it = attrs.find("id");
	if (it == attrs.end()) {
		os << "<error>no user id found</error>";
	} else {
		arglist.push_back(UserId(atoi( it->second.c_str() )));
	}
	//tagStack.pop();
}

void BarzerRequestParser::tag_rulefile(RequestTag &tag) {
	AttrList &attrs = tag.attrs;
	const char* fname = tag.body.c_str();
	AttrList::iterator cl = attrs.find("class"),
					   id = attrs.find("id");
	if (cl == attrs.end() || id == attrs.end())	{
		arglist.push_back(Rulefile(fname));
	} else {
		arglist.push_back(Rulefile(fname, cl->second, id->second));
	}
}

void BarzerRequestParser::tag_trie(RequestTag &tag) {

}

void BarzerRequestParser::raw_autoc_parse( const char* query, QuestionParm& qparm ) 
{
    if(!d_universe ) 
        return ;
	const StoredUniverse &u = *d_universe;
	//qparser.parse( barz, getTag().body.c_str(), qparm );
    if( !barz.topicInfo.getTopicMap().empty() ) {
        barz.topicInfo.computeTopTopics();
    }
    BarzerAutocomplete autoc( barz, u, qparm, os );
	autoc.parse(query);
    /// doing this just in case barz is reused 
    barz.clearWithTraceAndTopics();
}

std::ostream& BarzerRequestParser::printError( const char* err )
{
    if( ret == XML_TYPE ) {
		os << "<error>";
        xmlEscape(err, os);
		os << "</error>";
    } else if( ret == JSON_TYPE ) {
		ay::jsonEscape( err, os<< "{ error: ", "\"" );
    } else {
        os << err ;
    }
    return os;
}

void BarzerRequestParser::raw_query_parse_zurch( const char* query, const StoredUniverse& u )
{
    const zurch::DocIndexAndLoader* ixl = u.getZurchIndex( d_zurchDocIdxId );
    if( !ixl || !ixl->getIndex() ) {
        if( ret == XML_TYPE )
		os << "<error>invalid zurch index id " << d_zurchDocIdxId << " for universe " << userId << "</error>\n";
        else if( ret == JSON_TYPE ) 
            os<< "{ \"error\": \"invalid zurch index id " << d_zurchDocIdxId << "\"}" ;

		return;
    }
    const zurch::DocFeatureIndex* index = ixl->getIndex();
    barz.clear();

	QuestionParm qparm;
    QParser qparser(u);

    zurch::ExtractedDocFeature::Vec_t featureVec;
    zurch::DocFeatureIndex::DocWithScoreVec_t docVec;  
	std::map<uint32_t, zurch::DocFeatureIndex::PosInfos_t> positions;
	// std::cout << "handling '" << query << "'" << std::endl;
    qparm.turnSplitCorrectionOff();

    if( !d_queryFlags.empty() )
        qparm.setZurchFlags( d_queryFlags.c_str() );
    if( d_zurchSearchById ) { /// in this case query must contain document name
        uint32_t docId  = ixl->getDocIdByName( query );
        if( docId != 0xffffffff )
            docVec.push_back( std::pair<uint32_t,double>(docId, 1.0) );
    } else {
	    qparser.tokenize_only( barz, query, qparm );
	    qparser.lex_only( barz, qparm );
	    qparser.semanticize_only( barz, qparm );
     
    	
        if( index->fillFeatureVecFromQueryBarz( featureVec, barz ) )  {
            zurch::DocFeatureIndex::SearchParm parm( 16, (fitlerCascade.empty()? 0: &fitlerCascade), &positions );
            index->findDocument( docVec, featureVec, parm );
        }
    }
    if( ret == XML_TYPE ) {
        zurch::DocIdxSearchResponseXML response( qparm, *ixl, barz ); 
        response.print(os, docVec, positions);
    } else if ( ret == JSON_TYPE ) {
        zurch::DocIdxSearchResponseJSON response( qparm, *ixl, barz ); 
        response.print(os, docVec, positions);
    }
}

void BarzerRequestParser::raw_query_parse( const char* query)
{
	const GlobalPools& gp = gpools;
	const StoredUniverse * up = gp.getUniverse(userId);
	if( !up ) {
        if( ret == XML_TYPE ) 
		    os << "<error>invalid user id " << userId << "</error>\n";
        else  if( ret == JSON_TYPE ) 
            os<< "{ \"error\": \"invalid user id " << userId << "\"}" ;
            
		return;
	}
	const StoredUniverse &u = *up;
    if( d_queryType == QType::ZURCH ) {
        raw_query_parse_zurch(query,u);
        return;
    }

	QParser qparser(u);

	QuestionParm qparm;
    if( d_aggressiveStem ) 
        qparm.setStemMode_Aggressive();

	//qparser.parse( barz, getTag().body.c_str(), qparm );
    if( !barz.topicInfo.getTopicMap().empty() ) {
        barz.topicInfo.computeTopTopics();
    }
	qparser.parse( barz, query, qparm );

    if( barz.hasReqVarNotEqualTo("geo::enableFilter","false") )
		    proximityFilter(barz, *up);

    switch(ret) {
    case XML_TYPE: {
    	BarzStreamerXML response( barz, u );
    	response.print(os);
    }
    break;
    case JSON_TYPE: {
    	BarzStreamerJSON response( barz, u );
        response.setSimplified(d_simplified);
    	response.print(os);
    }
    break;
    default:
    	AYLOG(ERROR) << "unknown return type";
    	break;
    }

    /// doing this just in case barz is reused 
    barz.clearWithTraceAndTopics();
}

void BarzerRequestParser::tag_topic(RequestTag &tag) {
    if( !isParentTag("qblock") && !isParentTag("autoc")) 
        return;

	AttrList &attrs = tag.attrs;
    const char* topicIdStr = 0;
    uint32_t entClass = 0, entSubclass = 0;
    for( AttrList::const_iterator a = attrs.begin(); a!= attrs.end(); ++a ) {
        const char* n = a->first.c_str();
        const char* v = a->second.c_str();
        switch( n[0] ) {
        case 'c': // class 
            entClass = (uint32_t)(atoi(v));
            break;
        case 's': // sub class 
            entSubclass = (uint32_t)(atoi(v));
            break;
        case 'i': // id 
            topicIdStr = v;
            break;
        }
    }
    StoredTokenId tokId = ( topicIdStr ? gpools.dtaIdx.getTokIdByString(topicIdStr) : 0xffffffff );      
    BarzerEntity topicEnt( tokId, entClass, entSubclass );
    barz.topicInfo.addTopic( topicEnt );
    barz.topicInfo.setTopicFilterMode_Strict(); 
}

namespace {
struct AutocTopicParseCB {
    BarzerRequestParser& brp;
    AutocTopicParseCB( BarzerRequestParser& p ) : brp(p) {}
    
    int operator()( size_t num, const char* t, const char* t_end )
    {
        ay::string_tokenizer_iter ti( t, t_end, '.' );
        StoredEntityUniqId euid;
        std::string str; 
        if( ti ) {
            ti.get_string( str );
            euid.eclass.ec = atoi( str.c_str() );
            ++ti;
        } 
        if( ti ) {
            ti.get_string( str );
            euid.eclass.subclass = atoi( str.c_str() );
            ++ti;
        } 
        if( ti ) {
            ti.get_string( str );
            ++ti;
            euid.tokId = brp.getGlobalPools().dtaIdx.getTokIdByString(str.c_str());
        } 
        
        if( euid.eclass.isValid() && euid.tokId != 0xffffffff ) {
            brp.getBarz().topicInfo.addTopic( euid );
            brp.getBarz().topicInfo.setTopicFilterMode_Strict(); 
        }

        return 0;
    }
};
}

void BarzerRequestParser::tag_autoc(RequestTag &tag) 
{
	AttrList &attrs = tag.attrs;
	const GlobalPools& gp = gpools;

	QuestionParm qparm;
    for( AttrList::const_iterator a = attrs.begin(); a!= attrs.end(); ++a ) {
        const std::string& v = a->second;
        switch( a->first[0] ) {
        case 'u': 
            if( !d_universe ) {
                setUniverseId(atoi(a->second.c_str()));
            }
            if( !d_universe ) {
                os << "<error>invalid user id " << userId << "</error>\n";
                return;
            }
            break;
        case 't':
            switch( a->first[1] ) {
            case 'c': // tc trie class (optional)
                qparm.autoc.trieClass = gp.internalString_getId( a->second.c_str());
                break;
            case 'i': // ti trie id (optional)
                qparm.autoc.trieId = gp.internalString_getId( a->second.c_str());
                break;
            case 'o': // to - topic 
                {
                    AutocTopicParseCB cb(*this);
                    ay::parse_separator( cb, v.c_str(), v.c_str()+v.length(), '|' );
                }
                break;
            }
            break;
        case 'e':
            qparm.autoc.parseEntClassList(a->second.c_str());
            break;
        }
    }
    d_query = tag.body.c_str();
	
    if( barz.hasReqVar("numResults") ) {
	    const auto numResVar = barz.getReqVarValue("numResults");
	    if (numResVar) {
		    BarzerNumber num;
		    if (BarzerAtomicCast(d_universe).convert(num, *numResVar) == BarzerAtomicCast::CASTERR_OK)
			    qparm.autoc.numResults = num.getInt();
	    }
    }
    
    qparm.isAutoc = true;
    raw_autoc_parse( d_query.c_str(), qparm );
    d_query.clear();
}

void BarzerRequestParser::tag_findents (RequestTag& tag)
{
	const auto& attrs = tag.attrs;
	double lon = -1, lat = -1, dist = -1;
	int64_t ec = -1, esc = -1;
	std::string distUnitStr;
	for (auto attr = attrs.begin(), end = attrs.end(); attr != end; ++attr)
	{
		const auto& name = attr->first;
		const auto& val = attr->second;
		switch (name[0])
		{
		case 'u':
			if (!d_universe)
				setUniverseId(atoi(val.c_str()));
			break;
		case 'l':
			switch (name[1])
			{
			case 'o': // longitude
				try
				{
					lon = boost::lexical_cast<double>(val);
				}
				catch (...)
				{
					os << "<error>invalid longitude " << val << "</error>\n";
					return;
				}
				break;
			case 'a': // latitude
				try
				{
					lat = boost::lexical_cast<double>(val);
				}
				catch (...)
				{
					os << "<error>invalid latitude " << val << "</error>\n";
					return;
				}
				break;
			}
			break;
		case 'd':
			switch (name[1])
			{
			case 'i':
				try
				{
					dist = boost::lexical_cast<double>(val);
				}
				catch (...)
				{
					os << "<error>invalid distance " << val << "</error>\n";
					return;
				}
				break;
			case 'u':
				distUnitStr = attr->second;
				break;
			}
			break;
		case 'e':
			switch (name[1])
			{
			case 'c': // entity class
				try
				{
					ec = boost::lexical_cast<int64_t>(val);
				}
				catch (...)
				{
					os << "<error>invalid entity class " << val << "</error>\n";
					return;
				}
				break;
			case 's': // entity class
				
				break;
			}
			break;
		}
	}
	
	if (lon < 0 || lat < 0 || dist < 0)
	{
		os << "<error>invalid parameters lon/lat/dist: " << lon << " " << " " << lat << " " << dist << "</error>\n";
		return;
	}
	
	auto unit = distUnitStr.empty() ? ay::geo::Unit::Metre : ay::geo::unitFromString(distUnitStr);
	if (unit >= ay::geo::Unit::MAX)
	{
		os << "<error>invalid unit: " << distUnitStr << "</error>\n";
		return;
	}
	
	if (!d_universe)
	{
		os << "<error>invalid user id " << userId << "</error>\n";
		return;
	}
	
	dist = ay::geo::convertUnit(dist, unit, ay::geo::Unit::Degree);
	
	const auto& geo = d_universe->getGeo();
	
	std::vector<uint32_t> ents;
	if (ec >= 0 && esc >= 0)
		geo->findEntities(ents, BarzerGeo::Point_t(lon, lat), EntClassPred(ec, esc, *d_universe), dist);
	else
		geo->findEntities(ents, BarzerGeo::Point_t(lon, lat), DumbPred(), dist);
	
	os << "<entities count='" << ents.size() << "'>\n";
	const auto& gp = d_universe->getGlobalPools();
	for(size_t i = 0; i < ents.size(); ++i)
	{
		auto ent = gp.getDtaIdx().getEntById(ents[i]);
		if (!ent)
		{
			os << "<ent invalid='true'/>";
			continue;
		}
		
		os << "\t<ent id='" << ents[i] << "' class='" << ent->getClass() << "' subclass='" << ent->getSubclass() << "'/>\n";
	}
	os << "</entities>\n";
}

// <qblock u="23"><topic c="20" s="1" i="abc"/><query>zoo</query></qblock>
void BarzerRequestParser::tag_qblock(RequestTag &tag) {
	AttrList &attrs = tag.attrs;
    for( AttrList::const_iterator a = attrs.begin(); a!= attrs.end(); ++a ) {
        if( a->first == "u" ) {
            userId = atoi(a->second.c_str());
        } 
    }
    raw_query_parse( d_query.c_str() );
    barz.topicInfo.setTopicFilterMode_Light();
    d_query.clear();
}

void BarzerRequestParser::tag_var(RequestTag &tag) {
    if( RequestEnvironment* env = barz.getServerReqEnv() ) {
        const char* n=0, *v=0 /*, *t=0*/;
        for( auto i = tag.attrs.begin(); i!= tag.attrs.end(); ++i ) {
            if( i->first =="n" ) {
                n = i->second.c_str();
            } else if( i->first == "v" ) {
                v = i->second.c_str();
            } else if( i->first == "t" ) { // type NOT SUPPORTED YET
                // t = i->second.c_str();
            }
        }
        if( n && v ) {
            barz.setReqVarValue( n, BarzelBeadAtomic_var(BarzerString(v)) );
        } else {
	        stream() << "<error>var:";
            if( !n ) stream() << "n attribute must be set. ";
            if( !v ) stream() << "v attribute must be set. ";
	        stream() << "</error>\n";
        }
    } else 
	    stream() << "<error>var: request environment not set</error>\n";
}

void BarzerRequestParser::tag_nameval(RequestTag &tag) {
	AttrList &attrs = tag.attrs;
	AttrList::iterator it = attrs.find("n");
    if( it != attrs.end() ) {
        if( d_universe ) {
            // const Ghettodb& ghettoDb = d_universe->getGhettodb();
            const char* propName = it->second.c_str();
            uint32_t n = d_universe->getGhettodb().getStringId(propName);
            if( n!= 0xffffffff ) {
                barz.topicInfo.addPropName( n );
            } else {
                /// implementing fall through into universe 0
                if( d_universe->getUserId() ) {
                    const StoredUniverse* zeroUniverse = gpools.getUniverse(0);
                    n = ( zeroUniverse ? zeroUniverse->getGhettodb().getStringId(propName) : 0xffffffff );

                    if( n!= 0xffffffff ) {
                        barz.topicInfo.addPropNameZeroUniverse( n );
                        return;
                    }
                }
                stream() << "<error>field " << propName << " doesnt exist</error>\n";
            }
        } else {
            stream() << "<error>no valid universe</error>\n";
        }
    }
}
void BarzerRequestParser::tag_pf(RequestTag &tag) 
{
    if( !isParentTag("prop") )
        return;
    for( const auto & i : tag.attrs ) {
        if( i.first == "v" ) 
            fitlerCascade.parse( i.second.c_str(), i.second.c_str()+i.second.length() );
    }
}
void BarzerRequestParser::tag_prop(RequestTag &tag) 
{
    if( !isParentTag("qblock") && !isParentTag("autoc")) 
        return;
}

void BarzerRequestParser::tag_query(RequestTag &tag) 
{
	AttrList &attrs = tag.attrs;
    AttrList::iterator it;
    if( !d_universe ) {
	    it = attrs.find("u");
		setUniverseId(it != attrs.end() ? atoi(it->second.c_str()) : 0);
    }
    d_simplified = false;
    d_queryFlags.clear();
    //ReturnType t = XML_TYPE;
    for( auto i = attrs.begin(); i!= attrs.end(); ++i ) {
        if( i->first == "qid" ) 
            barz.setQueryId( atoi( i->second.c_str() ) );
        else if( i->first == "as" ) 
            d_aggressiveStem = true;
        else if (i->first == "ret" ) { 
            if( i->second == "json") {
        	    ret = JSON_TYPE;
            } else if( i->second == "sjson" ) {
                d_simplified = true;
        	    ret = JSON_TYPE;
            }
        } else if (i->first == "now" ) {
            if( RequestEnvironment* env = barz.getServerReqEnv() )
                env->setNow( i->second );
        } else if( i->first == "zurch" ) {
            /// value of zurch attributes QuestionParm::setZurchFlags 
            /// (see the code for values - this is a string of single character flags)
            setQueryType(QType::ZURCH);
            d_zurchDocIdxId = atoi(i->second.c_str());
        } else if( i->first =="flag" ) {
            d_queryFlags = i->second;
        } else if( i->first.length() > 2 && i->first[0] == 'v' ) {  /// value filter 
            /// value filter vi.
        }
    }

    if( isParentTag("qblock") ) {
        d_query = tag.body.c_str();
    } else {
        raw_query_parse( tag.body.c_str());
    }
}

} // namespace barzer
