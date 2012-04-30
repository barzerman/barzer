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
    
    if( !rp->getUniverse() ) {
        if( 
            (name[0] == 'q' &&(!strcmp(name,"query") || !strcmp(name,"qblock"))) ||
            (name[0] == 'a' && !strcmp(name,"autoc")) 
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
    d_universe(0),
    os(s),
    d_aggressiveStem(false)
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
			CMDFUN(autoc)
			CMDFUN(qblock)
			CMDFUN(query)
			CMDFUN(nameval)
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
    if( d_aggressiveStem ) 
        qparm.setStemMode_Aggressive();

	//qparser.parse( barz, getTag().body.c_str(), qparm );
    if( !barz.topicInfo.getTopicMap().empty() ) {
        barz.topicInfo.computeTopTopics();
    }
	qparser.parse( barz, query, qparm );
	response.print(os);
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
    
    qparm.isAutoc = true;
    raw_autoc_parse( d_query.c_str(), qparm );
    d_query.clear();
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

void BarzerRequestParser::tag_query(RequestTag &tag) {
	AttrList &attrs = tag.attrs;


    AttrList::iterator it;
    if( !d_universe ) {
	    it = attrs.find("u");
		setUniverseId(it != attrs.end() ? atoi(it->second.c_str()) : 0);
    }

	it = attrs.find("as"); // aggressive stemming 
    if( it != attrs.end() ) 
        d_aggressiveStem = true;

    if( isParentTag("qblock") ) {
        d_query = tag.body.c_str();
    } else {
        raw_query_parse( tag.body.c_str() );
    }
}

} // namespace barzer
