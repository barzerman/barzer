
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
/*
 * barzer_server_request.h
 *
 *  Created on: Apr 21, 2011
 *      Author: polter
 */
#pragma once
#include <map>
#include <vector>
#include <ay/ay_logger.h>
#include <boost/function.hpp>
#include <barzer_barz.h>
#include <barzer_parse.h>
#include <barzer_settings.h>
#include <barzer_server_request_filter.h>
#include <barzer_question_parm.h>

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
    ReqFilterCascade filterCascade;

    std::vector<std::string> docTags;

	struct RequestTag {
		std::string tagName;
		AttrList attrs;
		std::string body;
	};
#ifdef DELETE
#undef DELETE
#endif
	enum Cmd {ADD,DELETE};
	enum ReturnType { XML_TYPE, JSON_TYPE };
private:

	bool m_internStrings;
	std::vector<RequestTag> tagStack;

	CmdArgList arglist;

	Barz barz;

	const GlobalPools &gpools;
	const BarzerSettings &settings;
	uint32_t userId;
    /// potentially changes with every query 
    const StoredUniverse* d_universe;

	std::ostream &os;
    std::string d_query; // for query block 
    /// request subtype
    /// zurch: 
    ///   doc.features - features for a given doc
    ///   feature.docs - docs linked to a specific feature
    std::string d_route; // subtype of request . 
    std::string d_extra; // extra parameters (can be used by route)
    std::map<std::string, std::string> d_extraMap;	// more extra parameters that got unparsed by the rest of the stuff
    bool d_aggressiveStem;
    // barzer mode (BENI)

    int16_t d_beniMode;  // QuestionParm::parseBeniFlag should be used to fill

    size_t  d_tagCount; // number of tags parsed  
    // when the tag barz is encountered the mode is set to on
    bool d_xmlIsInvalid; /// when this is true no tags/cdata will be processed
    BarzXMLParser* d_barzXMLParser;
    /// unique query id 
    uint64_t       d_queryId;
    
    bool           d_simplified;
    
public:
    // instead of parsing input XML initializes everything from a URI string
    typedef enum : int {
        ERR_INIT_OK=0,
        ERR_INIT_BAD_USER,
        ERR_INIT_BAD_USERID,
        ERR_INIT_BAD_USER_NAME,
        ERR_INIT_BAD_USER_KEY,

        ERR_PROC_INTERNAL
    } ErrInit;
    static const char* getErrInitText( ErrInit );

private:
    auto autoc_nameval_process( QuestionParm& qparm, const std::string& n, const std::string& v ) -> ErrInit;
    
    void processGhettodbFields( const std::string& );
public:
    std::string    d_queryFlags;
    size_t d_maxResults; // max number of docs to return (32 default)

    std::ostream& printError( const char* err );

    auto initAutocFromUri( QuestionParm& qparm, const ay::uri_parse& uri ) -> ErrInit;

    auto initFromUri( QuestionParm& qparm, const char* uri, size_t uri_len, const char* query, size_t query_len ) -> ErrInit;
    // should be used together with initFromUri - query will be taken from d_query
    int parse(QuestionParm& qparm) ; 

	ReturnType ret;
    
    const char* httpContentTypeString() const
    {
        switch( ret ) {
        case XML_TYPE:
            return(
                strchr( d_queryFlags.c_str(), 'H') ? 
                    "text/html; charset=utf-8" : "text/xml; charset=utf-8"
            );
            break;
        case JSON_TYPE:
            return "application/json; charset=utf-8";
            break;
        default:
            return "text/plain; charset=utf-8";
        }
    }

    uint32_t d_zurchDocIdxId; /// which zurch index to use - this  id is used to get a specific ZurchIUndex from global Pools
    enum {
        ZURCH_MODE_STANDARD,
        ZURCH_MODE_BYID,
        ZURCH_MODE_ALL // no search is performed docs retrieved by tags only
    };
    int     d_zurchMode; /// which zurch index to use - this  id is used to get a specific ZurchIUndex from global Pools

    /// zurch doc indices uniquely correspond to the pair (userId,zurchDocIdxId)
    enum class QType {
        BARZER, /// default
        ZURCH,
        AUTOCOMPLETE
    } d_queryType; /// 

    bool isQueryTypeBarzer() const { return (d_queryType == QType::BARZER); }
    bool isQueryTypeZurch() const { return (d_queryType == QType::ZURCH); }
    bool isQueryTypeAutocomplete() const { return (d_queryType == QType::AUTOCOMPLETE); }

    void     setQueryType( QType qt )
        { d_queryType = qt; }
    QType getQueryType( ) const { return d_queryType; }

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

    BarzerRequestParser(const GlobalPools &gp, std::ostream &s );
	BarzerRequestParser(const GlobalPools&, std::ostream &s, uint32_t uid );
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

	const GlobalPools& getGlobalPools() const { return  gpools; }
	GlobalPools& getGlobalPools() { return  *(const_cast<GlobalPools*>(&gpools)); }

	void process(const char *name);
	/// query is already stripped of the 
	void raw_query_parse( const char* query);
	void raw_query_parse_zurch( const char* query, const StoredUniverse& );
	void raw_autoc_parse( const char* query, QuestionParm& qparm );

	void tag_autoc(RequestTag&);
	void tag_findents(RequestTag&);

	void tag_qblock(RequestTag&);

	void tag_prop(RequestTag&); // parent must be qblock or autoc
	void tag_pf(RequestTag&); // parent must be prop 

	void tag_query(RequestTag&);
	void tag_cmd(RequestTag&);

	void tag_rulefile(RequestTag&);
    /// <nameval n="propertyname"/>
	void tag_nameval(RequestTag&);
	void tag_var(RequestTag&);
	void tag_user(RequestTag&);
	void tag_topic(RequestTag&);
	void tag_trie(RequestTag&);

	const BarzerSettings& getSettings() const { return settings; }
	BarzerSettings& getSettings() { return *(const_cast<BarzerSettings*>(&settings)); }
	std::ostream& stream() { return os; }
    const char* getParentTag() const 
        { return( (tagStack.size() > 1) ?  tagStack[ tagStack.size() -2 ].tagName.c_str() : 0); }
    
    bool isParentTag( const char* t ) const
        { 
            const char* s = getParentTag();
            return (s && !strcmp(s,t));
        }
    const std::string& getExtra() const { return d_extra;} 
    const std::map<std::string, std::string>& getExtraMap() const { return d_extraMap; }

    const std::string& getRoute() const { return d_route;} 
    bool isRoute( const std::string& r ) const { return ( r == d_route ); }
    bool isRoute( const char* r  ) const { return ( d_route==r ); }
};

inline BarzerRequestParser::RequestTag& BarzerRequestParser::getTag()
	{ return tagStack.back(); }

} // namespace barzer
