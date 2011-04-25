#ifndef BARZER_SERVER_H
#define BARZER_SERVER_H

#include <cstdlib>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>

#include <ay/ay_headers.h>
#include <ay/ay_logger.h>

#include <barzer_universe.h>
#include <barzer_barz.h>
#include <barzer_parse.h>
#include <barzer_server_request.h>

namespace barzer {
int run_server(int port);

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
    StoredUniverse universe;

public:
    AsyncServer(boost::asio::io_service& io_service, short port);

    void handle_accept(SearchSession *new_session,
					   const boost::system::error_code& error);

    void query(const char*, const size_t, std::ostream&);
};

}

#endif

