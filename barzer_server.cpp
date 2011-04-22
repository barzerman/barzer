#include <barzer_server.h>

namespace barzer {

using boost::asio::ip::tcp;

void SearchSession::start() {
	boost::asio::write(socket_, boost::asio::buffer("-ok- welcome to barzer\n"));

	boost::asio::async_read_until(socket_, data_, "\r\n",
								  boost::bind(&SearchSession::handle_read, this,
											  boost::asio::placeholders::error,
											  boost::asio::placeholders::bytes_transferred));
}


void SearchSession::handle_read(const boost::system::error_code ec, size_t bytes_transferred) {

	if (!ec) {
		std::string response;
		char *chunk = new char[bytes_transferred];
		std::istream is(&data_);

		is.readsome(chunk, bytes_transferred);
		std::string request(chunk);

		response = process_input(request);

		boost::asio::write(socket_,  boost::asio::buffer("-ok- "));
		boost::asio::write(socket_,  boost::asio::buffer(response, bytes_transferred));
	}

	delete this;
}


AsyncServer::AsyncServer(boost::asio::io_service& io_service, short port)
	: io_service_(io_service),
	  acceptor_(io_service, tcp::endpoint(tcp::v4(), port)),
	  parser(universe)

{
    SearchSession *new_session = new SearchSession(io_service_, this);
    acceptor_.async_accept(new_session->socket(),
                           boost::bind(&AsyncServer::handle_accept, this, new_session,
                                       boost::asio::placeholders::error));
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


int run_server(int port) {
	boost::asio::io_service io_service;
	std::cerr << "Running barzer search server on port " << port << "..." << std::endl;
	AsyncServer s(io_service, port);
	io_service.run();

	return 0;
}
}
