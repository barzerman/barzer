
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#include <barzer_server.h>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>
#include <barzer_el_xml.h>
#include <barzer_emitter.h>
#include <barzer_universe.h>
#include <autotester/barzer_at_comparators.h>
#include <ay_translit_ru.h>
#include <barzer_relbits.h>
#include <barzer_el_trie_ruleidx.h>
#include <barzer_shellsrv_shared.h>


extern "C" {
#include <unistd.h>
}

namespace barzer {

using boost::asio::ip::tcp;

class AsyncServer;

class SearchSession {
	tcp::socket socket_;
	boost::asio::streambuf data_;
	boost::asio::streambuf outbuf;

	AsyncServer *server;

public:
	SearchSession(boost::asio::io_service& io_service, AsyncServer *s)
		: socket_(io_service), server(s) {}

	tcp::socket& socket() { return socket_;	}

	std::string process_input(std::string input) {
		return boost::algorithm::to_upper_copy(input);
	}
	void start();

	void handle_write(const boost::system::error_code&, size_t);
	void handle_read(const boost::system::error_code&, size_t);

};


class AsyncServer {
    boost::asio::io_service& io_service_;
    tcp::acceptor acceptor_;

    // should probably change this to be initialized in main() or something
    GlobalPools &gpools;

public:
    AsyncServer(GlobalPools &gp, boost::asio::io_service& io_service, short port)
    	: io_service_(io_service),
    	  acceptor_(io_service, tcp::endpoint(tcp::v4(), port)), gpools(gp)
    {
        SearchSession *new_session = new SearchSession(io_service_, this);
        acceptor_.async_accept(new_session->socket(),
                               boost::bind(&AsyncServer::handle_accept, this, new_session,
                                           boost::asio::placeholders::error));
    }


    void handle_accept(SearchSession *new_session,
					   const boost::system::error_code& error);



    void query_router(char*, const size_t, std::ostream&);

    void init() {
    	//
    }
};



void SearchSession::start() {
	boost::asio::async_read_until(socket_, data_, "\r\n.\r\n",
								  boost::bind(&SearchSession::handle_read, this,
											  boost::asio::placeholders::error,
											  boost::asio::placeholders::bytes_transferred));
}


void SearchSession::handle_write(const boost::system::error_code &e, std::size_t len)
{
		if (e) {
			AYLOG(ERROR) << "Error writing into buffer: " << e;
		} else {
			// AYLOG(DEBUG) << len << " written into the socket";
		}
		delete this;
}

namespace {

void sanitize_russian_ya( char* s, size_t& s_len )
{
    const unsigned char* s_end = ((unsigned char*)s)+s_len;
    unsigned char* j=(unsigned char*)(s); 
    for( unsigned char* i=(unsigned char*)(s); i< s_end && j< s_end; ++i,++j ) {
        // workaround the YA character
        if( j[0] == 0xd1 && j[1] == 0xff ) {
            if( j[2] == 0xf5 && j[3] == 0xff && j[4] == 0xfd && j[5] == 0x06  ) {
                j[5] = 0x8f;
                j+= 4;
            } 
        } else 
            *i = *j;
    }
    s_len= j-(unsigned char*)s;
}

int url_route( const std::string& uri, const std::string& url, RequestEnvironment& reqEnv, const GlobalPools& realGlobalPools, const char* str )
{
    if( uri.empty() ) 
        return 0;
    switch( uri[0] ) {
    case 'k': 
        if( uri == "keyword" ) {
            Barz barz;
            BarzerShellSrvShared srvShared( realGlobalPools );
            int rc = srvShared.batch_parse( barz, url, std::string(), false );
            if( rc ) {
                reqEnv.outStream << "<error> batch parse returned " << rc << "</error>" << std::endl;
            } else {
                reqEnv.outStream << "<info> batch parse completed successfully </info>" << std::endl;
            }
        }
        break;
    }
    return 0;
}

}

void SearchSession::handle_read(const boost::system::error_code &ec, size_t bytes_transferred) {

	if (!ec) {
		char *chunk = new char[bytes_transferred];
		std::istream is(&data_);
		is.read(chunk, bytes_transferred);

		std::ostream os(&outbuf);
        sanitize_russian_ya( chunk, bytes_transferred );

		server->query_router(chunk, bytes_transferred, os);
		delete[] chunk;

		boost::asio::async_write(socket_, outbuf,
				boost::bind(&SearchSession::handle_write, this,
							boost::asio::placeholders::error,
							boost::asio::placeholders::bytes_transferred));
	}
}



void AsyncServer::handle_accept(SearchSession *new_session,
				   const boost::system::error_code& error)
{
    if (!error) {
        new_session->start();
        new_session = new SearchSession(io_service_, this);
        acceptor_.async_accept(new_session->socket(),
                               boost::bind(&AsyncServer::handle_accept, this, new_session,
                                           boost::asio::placeholders::error));
    } else {
        delete new_session;
    }
}

namespace request {
int barze( GlobalPools& gp, RequestEnvironment& reqEnv )
{
	BarzerRequestParser rp(gp, reqEnv.outStream, 0 );

    rp.getBarz().setServerReqEnv( &reqEnv );
	rp.parse(reqEnv.buf, reqEnv.len);

	return 0;
}

int proc_LOAD_USRCFG( RequestEnvironment& reqEnv, GlobalPools& gp, const char* cfgfile  )
{
    BELReader reader(gp,&(reqEnv.outStream));
    return gp.getSettings().loadUserConfig( reader, cfgfile );
    return 0;
}

int proc_LOAD_CONFIG( RequestEnvironment& reqEnv, GlobalPools& gp, const char* cfgfile  )
{
    BELReader reader(gp,&(reqEnv.outStream));
    gp.getSettings().load( reader, cfgfile );
    return 0;
}
int proc_CLEAR_TRIE( RequestEnvironment& reqEnv, GlobalPools& gp, const char*  str)
{
    const char* pipe = strchr( str, '|' );
    std::string trieClass, trieId;
    if( pipe ) {
        trieClass.assign( str, pipe-str );
        trieId.assign( pipe+1 );
    }
    if( BELTrie* trie = gp.getTrie( trieClass.c_str(), trieId.c_str() ) )  {
        BELTrie::WriteLock trie_lock( trie->getThreadLock() );
        trie->clear();
    } else{
        reqEnv.outStream << "<error>No valid trie (" << trieClass << "|" << trieId << ")</error>\n";
    }
    return 0;
}
int proc_CLEAR_USER( RequestEnvironment& reqEnv, GlobalPools& gp, const char* str )
{

    int userId = atoi( str );
    if( userId<= 0  ) {
        std::cerr << "Cannot clear id:" << userId << std::endl;
        return 0;
    }
    StoredUniverse* uni = gp.getUniverse( userId ) ;
    if( !uni ) {
        reqEnv.outStream << "<error>no valid universe found for id " << userId << "</error>\n";
        return 0;
    } else {
        reqEnv.outStream << "<info>Cleared universe for userid " <<  userId << "</info>";
        StoredUniverse::WriteLock universe_lock( uni->getMutex() );
        uni->clear();
    }
    return 0;
}

namespace {

const char*  read_pipe_sep( std::ostream& os, std::string& dest, const char * buf, size_t maxLen = 256 ) 
{
    const char* pipe = strchr( buf , '|' );
    if( pipe ) {
        size_t len = pipe - buf;
        if( len > maxLen ) {
            os << "<error>Name too long</error>\n";
            return 0;
        } else 
            return ( dest.assign(buf,len), buf+ len+1 );
    } 
    return 0;
}

}
// adds trie to user
int proc_ADD_TRIE( RequestEnvironment& reqEnv, GlobalPools& gp, const char*  str )
{
    std::string useridStr, trieClass, trieId;
    if(     
        (str= read_pipe_sep(reqEnv.outStream, useridStr,str)) !=0  &&
        (str= read_pipe_sep(reqEnv.outStream, trieClass,str)) !=0  &&
        (str= read_pipe_sep(reqEnv.outStream, trieId,str))    !=0 
    ) {
        uint32_t userId = (uint32_t)( atoi(useridStr.c_str() ) );

        StoredUniverse* uni = gp.getUniverse( userId );
        if( !uni ) {
            reqEnv.outStream << "<error>Valid universe for user id " << userId << " doesnt exist</error>\n";
            return 0;
        }
        BELTrie*  trie = gp.getTrie( trieClass.c_str(), trieId.c_str() );
        if( !trie ) {
            reqEnv.outStream << "<error>Cannot produce Trie (" << trieClass << '|' << trieId << ")</error>\n";
            return 0;
        }

        uni->appendTriePtr( trie, 0 );
    }
    return 0;
}

int proc_STMT_REMOVE( RequestEnvironment& reqEnv, GlobalPools& gp, const char*  str )
{
    enum {
        TOK_userid,
        TOK_trieClass,
        TOK_trieId,
        TOK_source,
        TOK_statementId
    };
    std::vector< std::string > sv;
    auto str_len = strlen(str);
    ay::parse_separator( 
        [&] (size_t tok_num, const char* tok, const char* tok_end) -> bool {
            if( tok_end> tok ) 
                sv.push_back( std::string(tok, tok_end-tok) );
            return false;
        },
        str, str+str_len
    );
    if( sv.size() >= TOK_statementId ) {
        auto userId = static_cast<uint32_t>( atoi(sv[TOK_userid].c_str()) );
        if( StoredUniverse* uni = gp.getUniverse( userId ) ) {
            StoredUniverse::WriteLock universe_lock( uni->getMutex() );
            for( auto i = sv.begin()+ TOK_statementId; i< sv.end(); ++i ) {
                if( !i->empty() ) {
                    if( !uni->ruleIdx().removeNode( 
                        sv[ TOK_trieClass ].c_str(),
                        sv[ TOK_trieId ].c_str(),
                        sv[ TOK_source ].c_str(),
                        atoi(i->c_str())
                    ) ) {
                        xmlEscape( str, reqEnv.outStream << "<error> failed to delete the rule" ) << "</error>\n";
                    }
                }
            }
        } else  
            reqEnv.outStream << "<error>no valid universe for user id " << userId << " doesnt exist</error>\n";
    }
    return 0;
}

/// format is !!ADD_STMSET:userid|trieClass|trieId|<stmset> ... </stmset>
int proc_ADD_STMSET( RequestEnvironment& reqEnv, GlobalPools& gp, const char*  str )
{
    /// decoing trie class / trie id
    /// str points past the first ':' 
    std::string useridStr, trieClass, trieId;
    if(     
        (str= read_pipe_sep(reqEnv.outStream,useridStr,str)) !=0  &&
        (str= read_pipe_sep(reqEnv.outStream,trieClass,str)) !=0  &&
        (str= read_pipe_sep(reqEnv.outStream,trieId,str))    !=0 
    ) {
        uint32_t userId = (uint32_t)( atoi(useridStr.c_str() ) );

        StoredUniverse* uni = gp.getUniverse( userId );
        if( !uni ) {
            reqEnv.outStream << "<error>no valid universe for user id " << userId << " doesnt exist</error>\n";
            return 0;
        }
        StoredUniverse::WriteLock universe_lock( uni->getMutex() );

        // BELTrie* trie = &(uni->produceTrie( trieClass, trieId ));

        BELTrie* trie = gp.getTrie( trieClass.c_str(), trieId.c_str() );
        if( !trie ) { 
            uint32_t trieClass_strId = gp.internString_internal( trieClass.c_str() );
            uint32_t trieId_strId = gp.internString_internal( trieId.c_str() );
            trie = gp.produceTrie( trieClass_strId, trieId_strId );
        }

        BELTrie::WriteLock trie_lock(trie->getThreadLock());

        BELReader  reader( trie, gp, &(reqEnv.outStream)  );
        reader.setCurrentUniverse( uni, gp.getSettings().getUser(uni->getUserId()) );
        reader.setLiveCommandMode();
	    std::stringstream is( str );
	   
	    reader.initParser(BELReader::INPUT_FMT_XML);
        reader.loadFromStream( is );
    } else {
        reqEnv.outStream << "<error> Wrong format for ADD_STMSET</error>\n";
    }

    return 0;
}
int proc_EN2RU( RequestEnvironment& reqEnv, const GlobalPools& realGlobalPools, const char* q )
{
	std::ostream &os = reqEnv.outStream;
	const char *prevStart = q;
	while (true)
	{
		if (!isspace(*q) && *q)
		{
			++q;
			continue;
		}

		std::string tmp;
		tmp.assign(prevStart, q);
		for (auto i = tmp.begin(), end = tmp.end(); i != end; ++i)
			*i = tolower(*i);
		std::string result;

		ay::tl::en2ru(tmp.c_str(), tmp.size(), result);
		xmlEscape( result.c_str(), os ) << " ";
		if (*q)
		{
            // xmlEscape( q, os );
			// os << *q;
			prevStart = ++q;
		}
		else
		{
			os << std::endl;
			break;
		}
	}
    return 0;
}
int proc_URL( RequestEnvironment& reqEnv, const GlobalPools& realGlobalPools, const char* str )
{
    std::string uri, url;
    ay::get_uri_url( uri, url, ( *str == '/' ? str+1: str ) );
    return url_route( uri, url, reqEnv, realGlobalPools, str );
}
int proc_EMIT( RequestEnvironment& reqEnv, const GlobalPools& realGlobalPools, const char* str )
{
	GlobalPools gp(false);
	if( realGlobalPools.parseSettings().stemByDefault() ) 
		gp.parseSettings().set_stemByDefault( );


	BELTrie* trie  = gp.mkNewTrie();
	std::ostream &os = reqEnv.outStream;
	BELReaderXMLEmit reader(trie, os);
	reader.initParser(BELReader::INPUT_FMT_XML);
	std::stringstream is( str );
	reader.setSilentMode();
    os << "<patternset>\n";
    reader.initCurrentUniverseToZero();

	reader.loadFromStream( is );
	os << "</patternset>\n";
	delete trie;
	return 0;
}

int proc_ENTRULES(RequestEnvironment& reqEnv, const GlobalPools& gp, const char *str)
{
	const auto strLen = std::strlen(str);
	const auto strEnd = str + strLen;
	const auto firstComma = std::find(str, strEnd, ',');
	const auto secondComma = std::find(firstComma + 1, strEnd, ',');
	const auto thirdComma = std::find(secondComma + 1, strEnd, ',');
	uint32_t uid = atoi(std::string(str, firstComma).c_str()),
			c = atoi(std::string(firstComma + 1, secondComma).c_str()),
			sc = atoi(std::string(secondComma + 1, thirdComma).c_str());
	const std::string entIdStr(thirdComma + 1, strEnd);
	
	const auto uni = gp.getUniverse(uid);
	if (!uni)
	{
		reqEnv.outStream << "<error>\nNo universe for user " << reqEnv.userId << "</error>\n";
		return 0;
	}
	
	const auto entIdStrId = gp.internalString_getId(entIdStr.c_str());
	
	std::vector<BarzelTranslationTraceInfo> out;
	uni->getEntRevLookup().lookup(StoredEntityUniqId(entIdStrId, c, sc), out);
	reqEnv.outStream << "<results count='" << out.size() << "'>";
	for (const auto& info : out)
	{
		reqEnv.outStream << "\n\t<match file=\"";
		xmlEscape(gp.internalString_resolve_safe(info.source), reqEnv.outStream) << "\" stmt=\"" << info.statementNum << "\"/>";
	}
	reqEnv.outStream << "\n</results>\n";
	return 0;
}

int proc_CHKBIT( RequestEnvironment& reqEnv, const GlobalPools& realGlobalPools, const char* str )
{
    size_t b = ( str ? atoi(str) : 0 );
    reqEnv.outStream << "<bits>\n";
    reqEnv.outStream << "  <b n=\"" << b << "\" v=\"" << RelBitsMgr::inst().check( b ) << "\"/>\n";
    reqEnv.outStream << "</bits>\n";
    return 0;
}
int proc_COUNT_EMIT( RequestEnvironment& reqEnv, const GlobalPools& realGlobalPools, const char* str )
{
	GlobalPools gp(false);
	if( realGlobalPools.parseSettings().stemByDefault() ) 
		gp.parseSettings().set_stemByDefault( );

	BELTrie* trie  = gp.mkNewTrie();
	std::ostream &os = reqEnv.outStream;
	BELReaderXMLEmitCounter reader(trie, os);
	reader.initParser(BELReader::INPUT_FMT_XML);
	std::stringstream is( str );
	reader.setSilentMode();
	reader.loadFromStream( is );
    os << reader.getCounter() ;
    delete trie;
	return 0;  
}

int proc_MATCH_XML(RequestEnvironment& reqEnv, GlobalPools& gp, const char *str)
{
	const char *firstEnd = strstr(str, "<<");
	const char *secondEnd = 0;
	if (firstEnd)
		secondEnd = strstr(firstEnd + 2, "<<");
	if (!firstEnd || !secondEnd)
	{
		reqEnv.outStream << "<error>Wrong format for </error>\n";
		return 0;
	}

	const uint16_t score = autotester::matches(str, firstEnd - str,
			firstEnd + 2, secondEnd - firstEnd - 2,
			autotester::ParseContext(gp, reqEnv.userId));
	reqEnv.outStream << "<score>" << score << "</score>\n";
	//autotester::matches();
    return 0;
}

int proc_RUN_SCRIPT( RequestEnvironment& reqEnv, GlobalPools& gp, const char* cfgfile  );

int route( GlobalPools& gpools, char* buf, const size_t len, std::ostream& os )
{
    #define IFHEADER_ROUTE(x) if( !strncmp(buf+2, #x":", sizeof( #x) ) ) {\
            RequestEnvironment reqEnv(os,buf+ sizeof(#x)+2 , len - (sizeof(#x)+2) );\
            os << "<cmdoutput>\n";\
			request::proc_##x( reqEnv,gpools, buf+ sizeof(#x)+2 );\
            os << "</cmdoutput>\n";\
            return ROUTE_ERROR_OK;\
    }
    /// command interface !!CMD:
	if( buf[0] == '!' && buf[1] == '!' && strchr( buf+2, ':') ) {
        char* cut = strstr(buf, "\r\n.\r\n");
        if (cut) *cut = 0;
		IFHEADER_ROUTE(COUNT_EMIT)
		IFHEADER_ROUTE(EMIT)
		IFHEADER_ROUTE(URL)
		IFHEADER_ROUTE(ENTRULES)
		IFHEADER_ROUTE(CHKBIT)
		IFHEADER_ROUTE(EN2RU)
		IFHEADER_ROUTE(ADD_STMSET)
		IFHEADER_ROUTE(STMT_REMOVE)
		IFHEADER_ROUTE(CLEAR_TRIE)
		IFHEADER_ROUTE(CLEAR_USER)
		IFHEADER_ROUTE(LOAD_CONFIG)
		IFHEADER_ROUTE(LOAD_USRCFG)
		IFHEADER_ROUTE(RUN_SCRIPT)
		IFHEADER_ROUTE(MATCH_XML)

		AYLOG(ERROR) << "UNKNOWN header: " << std::string( buf, (len>6 ? 6: len) ) << std::endl;
        return ROUTE_ERROR_UNKNOWN_COMMAND;
	} else {
		RequestEnvironment reqEnv(os,buf,len);
		request::barze( gpools, reqEnv );
	}
	return ROUTE_ERROR_OK;
}

int proc_RUN_SCRIPT( RequestEnvironment& reqEnv, GlobalPools& gp, const char* cfgfile  )
{
    char buf[ 512] ;
    FILE* fp = fopen( cfgfile, "r" );
    if( !fp ) {
	    reqEnv.outStream << "<error>cant open script file</error>\n";
        return ROUTE_ERROR_OK;
    }
    while( fgets( buf, sizeof(buf)-1, fp ) ) {
        size_t len = strlen(buf);
        if( len <=1 ) 
            continue;
        if( isspace(buf[ len -1 ]) ) {
            --len;
            buf[ len ]= 0;
        }
	    route( gp, buf, len, reqEnv.outStream );
    }
	return ROUTE_ERROR_OK;
}

} // request namespace ends

/*
	in order for the server to process special transactions they must begin with a header
	if the first 2 characters in buf are '!' followed by command (e.g. EMIT) request will be routed 
	accordingly
*/
void AsyncServer::query_router(char* buf, const size_t len, std::ostream& os)
{
	request::route( gpools, buf, len, os );
}

int run_server_mt(GlobalPools &gp, uint16_t port) {
	ay::Logger::getLogger()->setFile("barzer_server.log");
	std::cerr << "Running barzer search server(mt) on port " << port << "..." << std::endl;

	boost::asio::io_service io_service;

	boost::asio::io_service::work work(io_service);

  	boost::thread_group threads;
    for (std::size_t i = 0; i < gp.getSettings().getNumThreads(); ++i)
	{
		boost::thread *thread = threads.create_thread(boost::bind(&boost::asio::io_service::run, &io_service));
		ay::StemThreadPool::inst().createThreadStemmer(thread->native_handle());
	}

	AsyncServer s(gp, io_service, port);
	s.init();
	// io_service.run();
	threads.join_all();

	return 0;
}
namespace {
int try_svc_run(GlobalPools &gp, uint16_t port) {
    if( gp.getSettings().getNumThreads() ) {
            
        return run_server_mt(gp,port);
    }
    ay::Logger::getLogger()->setFile("barzer_server.log");
    std::cerr << "Running barzer search server on port " << port << "..." << std::endl;

    boost::asio::io_service io_service;

    AsyncServer s(gp, io_service, port);
    s.init();
    io_service.run();
    return 0;
}

} // anon namespace

int run_server(GlobalPools &gp, uint16_t port) {
    
    while( true ) {
        try {
            return try_svc_run( gp, port );
        } catch(std::exception& e ) {
            std::cerr <<"Port busy ..";
            sleep(1);
            std::cerr <<" retrying...\n";
        }
    }
	return 0;
}
std::ostream& RequestVariableMap::print( std::ostream& fp ) const
{
    auto m = getAllVars();
    for( auto i = m.begin(); i!= m.end(); ++i ) {
        fp << i->first << "=" << i->second << std::endl;
    }
    return fp;
}

int RequestEnvironment::setNow( const std::string& str )
{
    std::stringstream sstr(str);
    d_now.deserialize( sstr );
    return 0;
}
}
