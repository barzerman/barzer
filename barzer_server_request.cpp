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

BarzerRequestParser::BarzerRequestParser(GlobalPools &gp, std::ostream &s)
	: gpools(gp), settings(gp.getSettings()), userId(0)/* qparser(u), response(barz, u) */, os(s)
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
			CMDFUN(query)
			CMDFUN(cmd)
			CMDFUN(rulefile)
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
	void operator()(Rulefile &rf) {
		parser.getSettings().addRulefile(rf);
		parser.stream() << "<rulefile>!!";
	}
	void operator()(TrieId &tid) {
		BarzerSettings &s = parser.getSettings();
		User &u = s.createUser(tid.userId);
		u.addTrie(tid.path);
		parser.stream() << "<trie>!!";
	}
	void operator()(UserId &uid) {
		parser.getSettings().createUser(uid.id);
		parser.stream() << "<userid>!!";
	}
};

void BarzerRequestParser::tag_cmd(RequestTag &tag) {
	AttrList::iterator it = tag.attrs.find("name");
	if (it != tag.attrs.end() && it->second == "add") {
		CmdAdd cp(*this);
		cp.process(arglist);
	} else {
		os << "<error>unknown cmd name</error>";
	}
	arglist.clear();
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
	if( !up ) return;

	const StoredUniverse &u = *up;

	QParser qparser(u);
	BarzStreamerXML response(barz, u);

	QuestionParm qparm;
	//qparser.parse( barz, getTag().body.c_str(), qparm );
	qparser.parse( barz, query, qparm );
	response.print(os);
}

void BarzerRequestParser::tag_query(RequestTag &tag) {
	if( tag.tagName == "query" ) {
		return raw_query_parse( tag.body.c_str() );
	}
	StoredUniverse &u = gpools.produceUniverse(userId);

	QParser qparser(u);
	BarzStreamerXML response(barz, u);

	QuestionParm qparm;
	//qparser.parse( barz, getTag().body.c_str(), qparm );
	qparser.parse( barz, tag.body.c_str(), qparm );
	response.print(os);
//
}

} // namespace barzer
