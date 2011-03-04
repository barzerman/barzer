#include "barzer_server.h"

#include <cstdlib>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>

namespace barzer {

    using boost::asio::ip::tcp;

    class SearchSession {

    private:
        tcp::socket socket_;
        boost::asio::streambuf data_;

    public:
        SearchSession(boost::asio::io_service& io_service)
            : socket_(io_service)
        {}

        tcp::socket& socket() {
            return socket_;
        }
        
        void start() {
            boost::asio::write(socket_, boost::asio::buffer("-ok- welcome to barzer\n"));

            boost::asio::async_read_until(socket_, data_, "\r\n",
                                          boost::bind(&SearchSession::handle_read, this,
                                                      boost::asio::placeholders::error,
                                                      boost::asio::placeholders::bytes_transferred));
        }

        std::string process_input(std::string input) {
            return boost::algorithm::to_upper_copy(input);
        }

        void handle_read(const boost::system::error_code ec, size_t bytes_transferred) {

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
        
    };
  
    class AsyncServer {

    private:
        boost::asio::io_service& io_service_;
        tcp::acceptor acceptor_;

    public:
        AsyncServer(boost::asio::io_service& io_service, short port)
            : io_service_(io_service),
              acceptor_(io_service, tcp::endpoint(tcp::v4(), port)) {
            
            SearchSession* new_session = new SearchSession(io_service_);
            acceptor_.async_accept(new_session->socket(),
                                   boost::bind(&AsyncServer::handle_accept, this, new_session,
                                               boost::asio::placeholders::error));
        }
    
        void handle_accept(SearchSession* new_session,
                           const boost::system::error_code& error) {
            if (!error) {
                new_session->start();
                new_session = new SearchSession(io_service_);
                acceptor_.async_accept(new_session->socket(),
                                       boost::bind(&AsyncServer::handle_accept, this, new_session,
                                                   boost::asio::placeholders::error));
            } else {
                delete new_session;
            }
        }

    };
  
    int run_server(int port) {
        boost::asio::io_service io_service;
        std::cerr << "Running barzer search server on port " << port << "..." << std::endl;
        AsyncServer s(io_service, port);
        io_service.run();
    
        return 0;
    }
}