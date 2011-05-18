/*
 * barzer_server_request.h
 *
 *  Created on: Apr 21, 2011
 *      Author: polter
 */

#ifndef BARZER_SERVER_REQUEST_H_
#define BARZER_SERVER_REQUEST_H_

#include <map>
#include <vector>
#include <ay/ay_logger.h>
#include <boost/function.hpp>
#include <barzer_universe.h>
#include <barzer_barz.h>
#include <barzer_parse.h>
#include <barzer_server_response.h>


extern "C" {
#include <expat.h>
};

namespace barzer {
typedef std::pair<std::string, std::string> AttrPair;


class BarzerRequestParser;

typedef boost::function<void(BarzerRequestParser*)> ReqCmdFunc;
typedef std::map<std::string,ReqCmdFunc> CmdFunMap;

class BarzerRequestParser {
	struct RequestTag {
		std::string tagName;
		std::map<std::string,std::string> attrs;
		std::string body;
	};
	std::vector<RequestTag> tagStack;

	Barz barz;

	GlobalPools &gpools;
	uint32_t userId;

	/*
	Barz barz;
	QParser qparser;
	BarzStreamerXML response;
	*/

	std::ostream &os;
public:
	XML_Parser parser;

	BarzerRequestParser(GlobalPools&, std::ostream &s);
	~BarzerRequestParser();

	int parse(const char *buf, const size_t len) {
		return XML_Parse(parser, buf, len, true);
	}

	RequestTag& getTag() {
		return tagStack.back();
	}

	void addTag(const char *name)
	{
		tagStack.resize(tagStack.size() + 1);
		tagStack.back().tagName = name;
	}

	void setAttr(const char *name, const char *value) {
		tagStack.back().attrs.insert(AttrPair(name, value));
	}

	void setBody(const std::string &s);

	void process(const char *name);
	void command_query();


};


}

#endif /* BARZER_SERVER_REQUEST_H_ */
