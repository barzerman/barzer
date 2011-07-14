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
#include <barzer_barz.h>
#include <barzer_parse.h>
#include <barzer_server_response.h>
#include <barzer_settings.h>


extern "C" {
#include <expat.h>
};

namespace barzer {
typedef std::pair<std::string, std::string> AttrPair;


class BarzerRequestParser;


struct TrieId {
	User::Id userId;
	TriePath path;
	TrieId(User::Id uid, const std::string &cl, const std::string &tid)
		: userId(uid), path(cl, tid) {}
};

struct UserId {
	User::Id id;
	UserId(User::Id i) : id(i) {}
};


typedef boost::variant<
		Rulefile,
		TrieId,
		UserId
	> CmdArg;
typedef std::vector<CmdArg> CmdArgList;

class BarzerRequestParser {
public:
	typedef std::map<const std::string, const std::string> AttrList;
	struct RequestTag {
		std::string tagName;
		AttrList attrs;
		std::string body;
	};
	enum Cmd {ADD,DELETE};
private:

	std::vector<RequestTag> tagStack;

	CmdArgList arglist;

	Barz barz;

	GlobalPools &gpools;
	BarzerSettings &settings;
	uint32_t userId;

	std::ostream &os;
public:
	XML_Parser parser;

	BarzerRequestParser(GlobalPools&, std::ostream &s, uint32_t uid );
	~BarzerRequestParser();

	int parse(const char *buf, const size_t len) {
		return XML_Parse(parser, buf, len, true);
	}

	RequestTag& getTag();

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
	/// query is already stripped of the 
	void raw_query_parse( const char* query );

	void tag_query(RequestTag&);
	void tag_cmd(RequestTag&);
	void tag_rulefile(RequestTag&);
	void tag_user(RequestTag&);
	void tag_trie(RequestTag&);

	BarzerSettings& getSettings() { return settings; }
	std::ostream& stream() { return os; }
};

inline BarzerRequestParser::RequestTag& BarzerRequestParser::getTag()
	{ return tagStack.back(); }

}

#endif /* BARZER_SERVER_REQUEST_H_ */
