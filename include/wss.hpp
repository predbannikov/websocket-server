#ifndef WSS_H
#define WSS_H
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/thread.hpp>
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "wssserverConfig.h"

namespace server_wss {


//namespace beast = boost::beast;         // from <boost/beast.hpp>
//namespace http = beast::http;           // from <boost/beast/http.hpp>
//namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
//namespace net = boost::asio;            // from <boost/asio.hpp>
//using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

//------------------------------------------------------------------------------

// Report a failure
void version();
void fail(boost::beast::error_code ec, char const* what);

// Echoes back all received WebSocket messages
template<class T>
class session : public std::enable_shared_from_this<session<T>>
{
  boost::beast::websocket::stream<boost::beast::tcp_stream> ws_;
  boost::beast::flat_buffer buffer_;
  boost::asio::io_context &ioc;
  boost::asio::deadline_timer t;
  boost::asio::deadline_timer t_streamer;

 public:
  // Take ownership of the socket
  explicit session(boost::asio::ip::tcp::socket&& socket,
                   boost::asio::io_context &io_cont);

      // Get on the correct executor
  void run();

      // Start the asynchronous operation
  void on_run();

  void on_accept(boost::beast::error_code ec);

  void do_read();

  void stop_stream();

  void keep_alive(const boost::system::error_code &ec);

  void send_frame( boost::beast::error_code ec);

  void on_read( boost::beast::error_code ec,
               std::size_t bytes_transferred);

  void on_write_frame(boost::beast::error_code ec,
                      std::size_t bytes_transferred);

  void on_write( boost::beast::error_code ec,
                std::size_t bytes_transferred);

  ~session();
  //    unsigned long count_pack = 0;
};

//------------------------------------------------------------------------------
//#define MAX_CONNECTION 2
// Accepts incoming connections and launches the sessions
class listener : public std::enable_shared_from_this<listener>
{
  boost::asio::io_context& ioc_;
  boost::asio::ip::tcp::acceptor acceptor_;
  int counter_conection = 0;
 public:
  listener(
      boost::asio::io_context& ioc,
      boost::asio::ip::tcp::endpoint endpoint);

      // Start accepting incoming connections
  void run();

 private:
  void do_accept();

  void on_accept(boost::beast::error_code ec,
                 boost::asio::ip::tcp::socket socket);
};

}


void server_wss::fail(boost::beast::error_code ec, const char *what)
{
  std::cerr << what << ": " << " -- " << ec.what() << "\n" << std::flush;
}

template<class T>
server_wss::session<T>::session(boost::asio::ip::tcp::socket &&socket, boost::asio::io_context &io_cont)
    : ws_(std::move(socket)),ioc(io_cont), t(io_cont, boost::posix_time::seconds(0)), t_streamer(io_cont, boost::posix_time::seconds(0))
{
}

template<class T>
void server_wss::session<T>::run()
{
  // We need to be executing within a strand to perform async operations
  // on the I/O objects in this session. Although not strictly necessary
  // for single-threaded contexts, this example code is written to be
  // thread-safe by default.
  boost::asio::dispatch(ws_.get_executor(),
                        boost::beast::bind_front_handler(
                            &session::on_run,
                            session::shared_from_this()));
}

template <class T>
void server_wss::session<T>::on_run()
{
  // Set suggested timeout settings for the websocket
  ws_.set_option(
      boost::beast::websocket::stream_base::timeout::suggested(
          boost::beast::role_type::server));

      // Set a decorator to change the Server of the handshake
  ws_.set_option(boost::beast::websocket::stream_base::decorator(
      [](boost::beast::websocket::response_type& res)
      {
        res.set(boost::beast::http::field::server,
                std::string(BOOST_BEAST_VERSION_STRING) +
                    " websocket-server-async");
      }));
  // Accept the websocket handshake
  ws_.async_accept(
      boost::beast::bind_front_handler(
          &session::on_accept,
          session::shared_from_this()));

}

template<class T>
void server_wss::session<T>::on_accept(boost::beast::error_code ec)
{
  if(ec)
    return fail(ec, "accept");

      // Read a message
  send_frame(boost::beast::error_code());
  do_read();
}

template <class T>
void server_wss::session<T>::do_read()
{
  // Read a message into our buffer
  ws_.async_read(
      buffer_,
      boost::beast::bind_front_handler(
          &session::on_read,
          session::shared_from_this()));
}

template<class T>
void server_wss::session<T>::stop_stream()
{
  try {
    std::cout << "stop_stream() " << t.expires_from_now() << std::endl << std::flush;
    //            beast::websocket::close_reason cr;
    //            ws_.close(cr);
    ws_.close(boost::beast::websocket::close_code::normal);
    std::cout << "wss close socket:" << std::endl << std::flush;

  }  catch (boost::system::error_code ec) {
    std::cerr << ec.message() << std::endl << ec.what() << std::endl << std::flush;
  }
}

template<class T>
void server_wss::session<T>::keep_alive(const boost::system::error_code &ec)
{
  if(ec == boost::asio::error::operation_aborted)
    std::cout << "keep_alive() " << t.expires_from_now() << std::endl << std::flush;
  else
    stop_stream();
}

template<class T>
void server_wss::session<T>::send_frame(boost::beast::error_code ec)
{
  std::cout << "send_frame_start() " << t.expires_from_now() << std::endl << std::flush;
  if(ec) {
    return fail(ec, "read");

  }
  std::string str = std::to_string(rand()%1000000);
  ws_.text(true);
  ws_.async_write(
      boost::asio::buffer(str.data(), str.length()),
      boost::beast::bind_front_handler(
          &session::on_write_frame,
          session<T>::shared_from_this()));
  std::cout << "send_frame_exit()" << std::endl << std::flush;
}

template<class T>
void server_wss::session<T>::on_read(boost::beast::error_code ec, std::size_t bytes_transferred)
{
  boost::ignore_unused(bytes_transferred);
  std::cout << "set timer " << boost::beast::buffers_to_string(buffer_.data()) << t.expires_from_now() << std::endl << std::flush;
  if(ec) {
    return fail(ec, "read");
  }
  t.expires_from_now(boost::posix_time::seconds(5));
  t.async_wait(boost::bind(&session::keep_alive, this, boost::asio::placeholders::error));

      // Echo the message
  ws_.text(ws_.got_text());
  ws_.async_write(
      buffer_.data(),
      boost::beast::bind_front_handler(
          &session::on_write,
          session<T>::shared_from_this()));
  std::cout << "on_read:exit() " << t.expires_from_now() << std::endl << std::flush;
}

template<class T>
void server_wss::session<T>::on_write_frame(boost::beast::error_code ec, std::size_t bytes_transferred)
{
  boost::ignore_unused(bytes_transferred);
  if(ec)
    return fail(ec, "write");
  //        buffer_.consume(buffer_.size());
  t_streamer.expires_from_now(boost::posix_time::seconds(1));
  t_streamer.async_wait(boost::bind(&session::send_frame, this, boost::asio::placeholders::error));
                                                                                                     //        count_pack++;
}

template<class T>
void server_wss::session<T>::on_write(boost::beast::error_code ec, std::size_t bytes_transferred)
{
  boost::ignore_unused(bytes_transferred);
  if(ec)
    return fail(ec, "write");
  // Clear the buffer
  buffer_.consume(buffer_.size());
  do_read();
              //        if(count_pack == 0) {
  //            t_streamer.expires_from_now(boost::posix_time::seconds(1));
  //            t_streamer.async_wait(boost::bind(&session::send_frame, this, boost::asio::placeholders::error));
  //        }
  //        count_pack++;
}

template<class T>
server_wss::session<T>::~session()
{
  std::cout << "destructor session \n" << std::endl << std::flush;
}

server_wss::listener::listener(boost::asio::io_context &ioc, boost::asio::ip::tcp::endpoint endpoint)
    : ioc_(ioc)
      , acceptor_(ioc)
{
  boost::beast::error_code ec;

      // Open the acceptor
  acceptor_.open(endpoint.protocol(), ec);
  if(ec)
  {
    fail(ec, "open");
    return;
  }

      // Allow address reuse
  acceptor_.set_option(boost::asio::socket_base::reuse_address(true), ec);
  if(ec)
  {
    fail(ec, "set_option");
    return;
  }

      // Bind to the server address
  acceptor_.bind(endpoint, ec);
  if(ec)
  {
    fail(ec, "bind");
    return;
  }

      // Start listening for connections
  acceptor_.listen(
      boost::asio::socket_base::max_listen_connections, ec);
  if(ec)
  {
    fail(ec, "listen");
    return;
  }
}

void server_wss::listener::run() {
  do_accept();
}

void server_wss::listener::do_accept() {
  // The new connection gets its own strand
  //        if(counter_conection < MAX_CONNECTION)
  acceptor_.async_accept( boost::asio::make_strand(ioc_), boost::beast::bind_front_handler( &listener::on_accept, shared_from_this()));
}

void server_wss::listener::on_accept(boost::beast::error_code ec, boost::asio::ip::tcp::socket socket) {
  if(ec)
  {
    fail(ec, "accept");
  }
  else
  {
    // Create the session and run it
    //            boost::asio::io_service io_service;
    std::make_shared<session<int>>(std::move(socket), ioc_)->run();
  }

      // Accept another connection
  counter_conection++;
  do_accept();
}

void server_wss::version()
{
//  std::cout << "Web socket server version " << CWSS_VERSION_MAJOR << "." << CWSS_VERSION_MINOR << std::endl;
}

#endif // WSS_H
