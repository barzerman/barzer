#include <barzer_server.h>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>


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
    StoredUniverse &universe;

public:
    //AsyncServer(StoredUniverse &u, boost::asio::io_service& io_service, short port)
    AsyncServer(StoredUniverse &u, boost::asio::io_service& io_service, short port)
    	: io_service_(io_service),
    	  acceptor_(io_service, tcp::endpoint(tcp::v4(), port)), universe(u)
    {
        SearchSession *new_session = new SearchSession(io_service_, this);
        acceptor_.async_accept(new_session->socket(),
                               boost::bind(&AsyncServer::handle_accept, this, new_session,
                                           boost::asio::placeholders::error));
    }


    void handle_accept(SearchSession *new_session,
					   const boost::system::error_code& error);

    void query(const char*, const size_t, std::ostream&);

    void init() {
/*    	BELTrie &trie = universe.getBarzelTrie();
    	BELReader reader(&trie, universe);
    	reader.initParser(BELReader::INPUT_FMT_XML);
    	char fname[] = "barzel_rules.xml";
    	int numsts = reader.loadFromFile(fname);
    	*/
/*    	BarzerSettings &set = universe.getSettings();
    	set.load();
    	*/
    	//AYLOG(DEBUG) << numsts << " statements read from `" << fname << "'";
    }
};



void SearchSession::start() {
	//boost::asio::write(socket_, boost::asio::buffer("-ok- welcome to barzer\n"));

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
		std::string response;
		char *chunk = new char[bytes_transferred];
		std::istream is(&data_);
		is.read(chunk, bytes_transferred);

		//boost::asio::streambuf buf;
		std::ostream os(&outbuf);
		server->query(chunk, bytes_transferred, os);
		delete chunk;

		boost::asio::async_write(socket_, outbuf,
				boost::bind(&SearchSession::handle_write, this,
							boost::asio::placeholders::error,
							boost::asio::placeholders::bytes_transferred));

//				&handle_write);


		/*
		std::string request(chunk);
		response = process_input(request);

		boost::asio::write(socket_,  boost::asio::buffer("-ok- "));
		boost::asio::write(socket_,  boost::asio::buffer(response, bytes_transferred));
		*/

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

void AsyncServer::query(const char* buf, const size_t len, std::ostream& os) {
	BarzerRequestParser rp(universe, os);
	rp.parse(buf, len);
}



int run_server(StoredUniverse &u, uint16_t port) {
	ay::Logger::getLogger()->setFile("barzer_server.log");
	boost::asio::io_service io_service;
	std::cerr << "Running barzer search server on port " << port << "..." << std::endl;
	AsyncServer s(u, io_service, port);
	s.init();
	io_service.run();

	return 0;
}

}
