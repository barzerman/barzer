#include <barzer_server.h>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>
#include <barzer_el_xml.h>
#include <barzer_universe.h>


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
			AYLOG(DEBUG) << len << " written into the socket";
		}
		delete this;
}

void SearchSession::handle_read(const boost::system::error_code &ec, size_t bytes_transferred) {

	if (!ec) {
		char *chunk = new char[bytes_transferred];
		std::istream is(&data_);
		is.read(chunk, bytes_transferred);

		std::ostream os(&outbuf);
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
    BELTrie* trie = gp.getTrie( trieClass.c_str(), trieId.c_str() );
    if( trie ) {
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
        uni->clearTrieList();
        uni->clearSpelling();
    }
    return 0;
}

namespace {

const char*  read_pipe_sep( std::ostream& os, std::string& dest, const char * buf, size_t maxLen = 32 ) 
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
        reader.setCurrentUniverse( uni );
	    std::stringstream is( str );
	   
	    reader.initParser(BELReader::INPUT_FMT_XML);
        reader.loadFromStream( is );
    } else {
        reqEnv.outStream << "<error> Wrong format for ADD_STMSET</error>\n";
    }

    return 0;
}
int proc_EMIT( RequestEnvironment& reqEnv, const GlobalPools& realGlobalPools, const char* str )
{
	GlobalPools gp;
	if( realGlobalPools.parseSettings().stemByDefault() ) 
		gp.parseSettings().set_stemByDefault( );

	BELTrie* trie  = gp.mkNewTrie();
	std::ostream &os = reqEnv.outStream;
	BELReaderXMLEmit reader(trie, os);
	reader.initParser(BELReader::INPUT_FMT_XML);
	std::stringstream is( str );
	reader.setSilentMode();
    os << "<patternset>\n";
	reader.loadFromStream( is );
	os << "</patternset>\n";
	delete trie;
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
        char * newLine = strchr( buf, '\r' );
        if( newLine ) 
            *newLine = 0;
		IFHEADER_ROUTE(EMIT) 
		IFHEADER_ROUTE(ADD_STMSET)
		IFHEADER_ROUTE(CLEAR_TRIE)
		IFHEADER_ROUTE(CLEAR_USER)
		IFHEADER_ROUTE(LOAD_CONFIG)
		IFHEADER_ROUTE(LOAD_USRCFG)
		IFHEADER_ROUTE(RUN_SCRIPT)

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
    for (std::size_t i = 0; i < gp.settings.getNumThreads(); ++i)
	    threads.create_thread(boost::bind(&boost::asio::io_service::run, &io_service));

	AsyncServer s(gp, io_service, port);
	s.init();
	// io_service.run();
	threads.join_all();

	return 0;
}

int run_server(GlobalPools &gp, uint16_t port) {
	if( gp.settings.getNumThreads() ) {
		
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

}
