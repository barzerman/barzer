/*
 * barzer_server_request.cpp
 *
 *  Created on: Apr 21, 2011
 *      Author: polter
 */

#include <barzer_server_request.h>
#include <boost/assign.hpp>
#include <boost/mem_fn.hpp>

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
	: gpools(gp), userId(0)/* qparser(u), response(barz, u) */, os(s)
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

#define CMDFUN(n) (#n, boost::mem_fn(&BarzerRequestParser::command_##n))
static const ReqCmdFunc* getCmdFunc(std::string &name) {
	static CmdFunMap funmap = boost::assign::map_list_of
			CMDFUN(query);
			//("query", boost::mem_fn(&BarzerRequestParser::command_query));

	CmdFunMap::const_iterator fi = funmap.find(name);
	if (fi == funmap.end()) return 0;
	return &(fi->second);
}
#undef CMDFUN


void BarzerRequestParser::setBody(const std::string &s) {
	tagStack.back().body += s;
}

void BarzerRequestParser::process(const char *name) {
	RequestTag &t = tagStack.back();
	if (t.tagName == name) {
		const ReqCmdFunc *fun = getCmdFunc(t.tagName);
		if (fun) {
			(*fun)(this);
		}
	} else {
		AYLOG(ERROR) << "Tag mismatch. expected: '" << t.tagName
		<< "'; got:'" << name << "'.";
	}
}


void BarzerRequestParser::command_query() {
	StoredUniverse &u = gpools.produceUniverse(userId);

	QParser qparser(u);
	BarzStreamerXML response(barz, u);

	QuestionParm qparm;
	qparser.parse( barz, getTag().body.c_str(), qparm );
	response.print(os);
//
}

} // namespace barzer
