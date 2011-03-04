#include "barzer_server.h"

#include <cstdlib>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/asio.hpp>

namespace barzer {

    using boost::asio::ip::tcp;

    class SearchSession {

    private:
        tcp::socket socket_;
        enum { max_length = 1024 };
        char data_[max_length];

    public:
        SearchSession(boost::asio::io_service& io_service)
            : socket_(io_service)
        {}

        tcp::socket& socket() {
            return socket_;
        }

        void start() {
            socket_.async_read_some(boost::asio::buffer(data_, max_length),
                                    boost::bind(&SearchSession::handle_read, this,
                                                boost::asio::placeholders::error,
                                                boost::asio::placeholders::bytes_transferred));
        }

        void handle_read(const boost::system::error_code& error,
                         size_t bytes_transferred) {
            if (!error) {
                boost::asio::async_write(socket_,
                                         boost::asio::buffer(data_, bytes_transferred),
                                         boost::bind(&SearchSession::handle_write, this,
                                                     boost::asio::placeholders::error));
            } else {
                delete this;
            }
        }
    
        void handle_write(const boost::system::error_code& error) {
            if (!error) {
                socket_.async_read_some(boost::asio::buffer(data_, max_length),
                                        boost::bind(&SearchSession::handle_read, this,
                                                    boost::asio::placeholders::error,
                                                    boost::asio::placeholders::bytes_transferred));
            } else {
                delete this;
            }
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
