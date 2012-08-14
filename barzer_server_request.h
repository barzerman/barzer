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
#include <barzer_settings.h>

extern "C" {
#include <expat.h>
};

#include <arch/barzer_arch.h>
namespace barzer {
typedef std::pair<std::string, std::string> AttrPair;


class BarzerRequestParser;
struct BarzXMLParser;

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
#ifdef DELETE
#undef DELETE
#endif
	enum Cmd {ADD,DELETE};
private:
	bool m_internStrings;
	std::vector<RequestTag> tagStack;

	CmdArgList arglist;

	Barz barz;

	GlobalPools &gpools;
	BarzerSettings &settings;
	uint32_t userId;
    /// potentially changes with every query 
    const StoredUniverse* d_universe;

	std::ostream &os;
    std::string d_query; // for query block 
    bool d_aggressiveStem;
    size_t  d_tagCount; // number of tags parsed  
    // when the tag barz is encountered the mode is set to on
    bool d_xmlIsInvalid; /// when this is true no tags/cdata will be processed
    BarzXMLParser* d_barzXMLParser;
    /// unique query id 
    uint64_t       d_queryId;
public:
    bool isQueryIdValid() const { return std::numeric_limits<uint64_t>::max() != d_queryId; }

    bool isXmlInvalid() const { return d_xmlIsInvalid; }
    void setXmlInvalid() { d_xmlIsInvalid=true; }

    size_t getTagCount() const { return d_tagCount; }
    size_t incrementTagCount() { return ( ++d_tagCount); }
    const BarzXMLParser* getBarzXMLParserPtr() const { return d_barzXMLParser; }
          BarzXMLParser* getBarzXMLParserPtr()       { return d_barzXMLParser; }

    void setBarzXMLParserPtr( BarzXMLParser* p )
        { d_barzXMLParser = p; }

    BarzXMLParser& getBarzXMLParser() { return *d_barzXMLParser; }
    const BarzXMLParser& getBarzXMLParser() const { return *d_barzXMLParser; }

	Barz& getBarz() { return barz; }
	const Barz& getBarz() const { return barz; }
	XML_Parser parser;

	BarzerRequestParser(GlobalPools&, std::ostream &s, uint32_t uid );
	~BarzerRequestParser();

	void setInternStrings(bool);
	bool shouldInternStrings() const;

	int parse(const char *buf, const size_t len) {
		return XML_Parse(parser, buf, len, true);
	}

	RequestTag& getTag();

	void addTag(const char *name)
	{
		tagStack.resize(tagStack.size() + 1);
		tagStack.back().tagName = name;
	}

    const StoredUniverse*                    setUniverseId( int id );
    void                    setUniverse( const StoredUniverse* u );
    const StoredUniverse*   getUniverse() const { return d_universe; }
	void setAttr(const char *name, const char *value) {
		tagStack.back().attrs.insert(AttrPair(name, value));
	}

	void setBody(const std::string &s);

	GlobalPools& getGlobalPools() { return  gpools; }
	const GlobalPools& getGlobalPools() const { return  gpools; }

	void process(const char *name);
	/// query is already stripped of the 
	void raw_query_parse( const char* query );
	void raw_autoc_parse( const char* query, QuestionParm& qparm );

	void tag_autoc(RequestTag&);

	void tag_qblock(RequestTag&);
	void tag_query(RequestTag&);
	void tag_cmd(RequestTag&);

	void tag_rulefile(RequestTag&);
    /// <nameval n="propertyname"/>
	void tag_nameval(RequestTag&);
	void tag_user(RequestTag&);
	void tag_topic(RequestTag&);
	void tag_trie(RequestTag&);

	BarzerSettings& getSettings() { return settings; }
	std::ostream& stream() { return os; }
    const char* getParentTag() const 
        { return( (tagStack.size() > 1) ?  tagStack[ tagStack.size() -2 ].tagName.c_str() : 0); }
    
    bool isParentTag( const char* t ) const
        { 
            const char* s = getParentTag();
            return (s && !strcmp(s,t));
        }
};

inline BarzerRequestParser::RequestTag& BarzerRequestParser::getTag()
	{ return tagStack.back(); }

}

#endif /* BARZER_SERVER_REQUEST_H_ */
