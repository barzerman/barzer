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
extern "C" {
// cast to XML_StartElementHandler
static void startElement(void* ud, const XML_Char *n, const XML_Char **a)
{
	const char* name = (const char*)n;
	const char** atts = (const char**)a;
	barzer::BarzerRequestParser *rp = (barzer::BarzerRequestParser *)ud;
	int attrCount = XML_GetSpecifiedAttributeCount( rp->parser );
	if( attrCount <=0 || (attrCount & 1) ) attrCount = 0; // odd number of attributes is invalid
	rp->addTag(name);
    
	for (int i = 0; i < attrCount; i += 2) {
		rp->setAttr(atts[i], atts[i+1]);
	}
}

// cast to XML_EndElementHandler
static void endElement(void *ud, const XML_Char *n)
{
	const char *name = (const char*) n;
	barzer::BarzerRequestParser *rp = (barzer::BarzerRequestParser *)ud;
	rp->process(name);
}

// cast to XML_CharacterDataHandler
static void charDataHandle( void * ud, const XML_Char *str, int len)
{
	//const char* s = (const char*)str;
	barzer::BarzerRequestParser *rp = (barzer::BarzerRequestParser *)ud;
	std::string s((char*) str, len);
	rp->setBody(s);
}
} // extern "C" ends

namespace barzer {

BarzerRequestParser::BarzerRequestParser(GlobalPools &gp, std::ostream &s, uint32_t uid ) : 
    gpools(gp), 
    settings(gp.getSettings()), 
    userId(uid)/* qparser(u), response(barz, u) */, 
    os(s)
{
		parser = XML_ParserCreate(NULL);
		XML_SetUserData(parser, this);
		XML_SetElementHandler(parser, &startElement, &endElement);
		XML_SetCharacterDataHandler(parser, &charDataHandle);
}

BarzerRequestParser::~BarzerRequestParser()
{
	XML_ParserFree(parser);
}

typedef boost::function<void(BarzerRequestParser*, BarzerRequestParser::RequestTag&)> ReqTagFunc;
typedef std::map<std::string,ReqTagFunc> TagFunMap;


#define CMDFUN(n) (#n, boost::mem_fn(&BarzerRequestParser::tag_##n))
static const ReqTagFunc* getCmdFunc(std::string &name) {
	static TagFunMap funmap = boost::assign::map_list_of
			CMDFUN(qblock)
			CMDFUN(query)
			CMDFUN(cmd)
			CMDFUN(rulefile)
			CMDFUN(topic)
			CMDFUN(trie)
			CMDFUN(user)
			;
			//("query", boost::mem_fn(&BarzerRequestParser::command_query));

	TagFunMap::const_iterator fi = funmap.find(name);
	if (fi == funmap.end()) return 0;
	return &(fi->second);
}
#undef CMDFUN


void BarzerRequestParser::setBody(const std::string &s) {
	getTag().body.append(s);
}

void BarzerRequestParser::process(const char *name) {
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
        std::cerr << "invalid add\n";
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
			parser.stream() << "<error>invalid userid " << uid.id << "</error>";
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

void BarzerRequestParser::raw_query_parse( const char* query ) 
{
	const GlobalPools& gp = gpools;
	const StoredUniverse * up = gp.getUniverse(userId);
	if( !up ) {
		os << "<error>invalid user id " << userId << "</error>\n";
		return;
	}
	const StoredUniverse &u = *up;

	QParser qparser(u);
	BarzStreamerXML response(barz, u);

	QuestionParm qparm;
	//qparser.parse( barz, getTag().body.c_str(), qparm );
    if( !barz.topicInfo.getTopicMap().empty() ) {
        std::cerr << "SHIT SHIT\n";
        barz.topicInfo.computeTopTopics();
    }
	qparser.parse( barz, query, qparm );
	response.print(os);
    /// doing this just in case barz is reused 
    barz.clearWithTraceAndTopics();
}

void BarzerRequestParser::tag_topic(RequestTag &tag) {
    if( !isParentTag("qblock") ) 
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

void BarzerRequestParser::tag_query(RequestTag &tag) {
	AttrList &attrs = tag.attrs;
	AttrList::iterator it = attrs.find("u");
	if( it != attrs.end() ) {
		userId = atoi(it->second.c_str());
	}
    if( isParentTag("qblock") ) {
        d_query = tag.body.c_str();
    } else {
        raw_query_parse( tag.body.c_str() );
    }
}

} // namespace barzer
