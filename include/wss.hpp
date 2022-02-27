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

/**
 * @namespace server_wss
 * @brief Общее пространство имён
 */
namespace server_wss {


//namespace beast = boost::beast;         // from <boost/beast.hpp>
//namespace http = beast::http;           // from <boost/beast/http.hpp>
//namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
//namespace net = boost::asio;            // from <boost/asio.hpp>
//using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

/**
 * @brief   Печать версии библиотеки в stdout.
 */
void version() {
  std::cout << "Web socket server version " <<
      wssserver_VERSION_MAJOR << "." <<
      wssserver_VERSION_MINOR << std::endl;
}

/**
 * @brief Печать ошибки.
 * @param[in] ec - Содержит тип ошибки boost::beast::error_code.
 * @param[in] what - Передать доп. описание вызвавшего.
 */
void fail(boost::beast::error_code ec, char const* what) {
  std::cerr << what << ": " << " -- " << ec.what() << "\n" << std::flush;
}


/*******************************************************************************
 *                              SESSION IMPL
 ******************************************************************************/

/**
 *  @brief    Класс обрабатывает сессию сервера с клиентом.
 *            Операции чтения/записи с сокетом выполняются асинхронно.
 */
template<class T>
class session : public std::enable_shared_from_this<session<T>>
{
  /**
   * @brief   Сокет соединения.
   */
  boost::beast::websocket::stream<boost::beast::tcp_stream> ws_;

  /**
   * @brief   Буфер для приёма сообщений.
   *          Используется в ping-pong мехонизме.
   */
  boost::beast::flat_buffer buffer_;

  /**
   * @brief   Объект исполнитель запросов ввода/вывода.
   */
  boost::asio::io_context &ioc;

  /**
   * @brief   Таймер механизма ping-pong.
   */
  boost::asio::deadline_timer t;

  /**
   * @brief   Таймер, по истечению которого вызывается отправка сообщения
   *          клиенту.
   */
  boost::asio::deadline_timer t_streamer;

 public:
  // Take ownership of the socket
  explicit session(boost::asio::ip::tcp::socket&& socket,
                   boost::asio::io_context &io_cont)
      : ws_(std::move(socket)),ioc(io_cont),
      t(io_cont, boost::posix_time::seconds(0)),
      t_streamer(io_cont, boost::posix_time::seconds(0)) {}

      // Get on the correct executor
  void run() {
    // We need to be executing within a strand to perform async operations
    // on the I/O objects in this session. Although not strictly necessary
    // for single-threaded contexts, this example code is written to be
    // thread-safe by default.S
    boost::asio::dispatch(ws_.get_executor(),
                          boost::beast::bind_front_handler(
                              &session::on_run,
                              session::shared_from_this()));
  }

      // Start the asynchronous operation
  void on_run() {
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

  void on_accept(boost::beast::error_code ec) {
    if (ec)
      return fail(ec, "accept");

    send_frame(boost::beast::error_code());

    // Read a message
    do_read();
  }

  void do_read() {
    // Read a message into our buffer
    ws_.async_read(
        buffer_,
        boost::beast::bind_front_handler(
            &session::on_read,
            session::shared_from_this()));
  }

/**
 * @brief Закрывает сокет
 */
  void stop_stream() {
    try {
      std::cout << "stop_stream() " << t.expires_from_now() <<
          std::endl << std::flush;
      //            beast::websocket::close_reason cr;
      //            ws_.close(cr);
      ws_.close(boost::beast::websocket::close_code::normal);
      std::cout << "wss close socket:" << std::endl << std::flush;

    } catch (boost::system::error_code ec) {
      std::cerr << ec.message() << std::endl <<
          ec.what() << std::endl << std::flush;
    }
  }

  void keep_alive(const boost::system::error_code &ec) {
    if (ec == boost::asio::error::operation_aborted)
      std::cout << "keep_alive() " <<
          t.expires_from_now() << std::endl << std::flush;
    else
      stop_stream();
  }

  void send_frame( boost::beast::error_code ec) {
    std::cout << "send_frame_start() " <<
        t.expires_from_now() << std::endl << std::flush;
    if (ec) {
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

  void on_read( boost::beast::error_code ec,
               std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);
    std::cout << "set timer " <<
        boost::beast::buffers_to_string(buffer_.data()) <<
        t.expires_from_now() <<
        std::endl << std::flush;
    if (ec) {
      return fail(ec, "read");
    }
    t.expires_from_now(boost::posix_time::seconds(5));
    t.async_wait(boost::bind(&session::keep_alive,
                 this,
                 boost::asio::placeholders::error));

        // Echo the message
    ws_.text(ws_.got_text());
    ws_.async_write(
        buffer_.data(),
        boost::beast::bind_front_handler(
            &session::on_write,
            session<T>::shared_from_this()));
    std::cout << "on_read:exit() " <<
        t.expires_from_now() <<
        std::endl << std::flush;
  }

  void on_write_frame(boost::beast::error_code ec,
                      std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);
    if (ec)
      return fail(ec, "write");
    //        buffer_.consume(buffer_.size());
    t_streamer.expires_from_now(boost::posix_time::seconds(1));
    t_streamer.async_wait(boost::bind(&session::send_frame,
                          this,
                          boost::asio::placeholders::error));
  }

  void on_write(boost::beast::error_code ec,
                std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);
    if (ec)
      return fail(ec, "write");
    // Clear the buffer
    buffer_.consume(buffer_.size());
    do_read();
  }

  ~session() {
    std::cout << "destructor session \n" << std::endl << std::flush;
  }
  //    unsigned long count_pack = 0;
};


/*******************************************************************************
 *                                LISTENER IMPL
 ******************************************************************************/

class listener : public std::enable_shared_from_this<listener>
{
  boost::asio::io_context& ioc_;
  boost::asio::ip::tcp::acceptor acceptor_;
  int counter_conection = 0;
 public:
  listener(
      boost::asio::io_context& ioc,
      boost::asio::ip::tcp::endpoint endpoint)
          : ioc_(ioc),
            acceptor_(ioc) {
    boost::beast::error_code ec;

        // Open the acceptor
    acceptor_.open(endpoint.protocol(), ec);
    if (ec) {
      fail(ec, "open");
      return;
    }

        // Allow address reuse
    acceptor_.set_option(boost::asio::socket_base::reuse_address(true), ec);
    if (ec) {
      fail(ec, "set_option");
      return;
    }

        // Bind to the server address
    acceptor_.bind(endpoint, ec);
    if (ec) {
      fail(ec, "bind");
      return;
    }

        // Start listening for connections
    acceptor_.listen(
        boost::asio::socket_base::max_listen_connections, ec);
    if (ec) {
      fail(ec, "listen");
      return;
    }
  }

  /// Start accepting incoming connections
  void run() {
    do_accept();
  }

 private:
  void do_accept() {
    // The new connection gets its own strand
    //        if(counter_conection < MAX_CONNECTION)
    acceptor_.async_accept(boost::asio::make_strand(ioc_),
        boost::beast::bind_front_handler(&listener::on_accept,
                                         shared_from_this()));
  }

  void on_accept(boost::beast::error_code ec,
                 boost::asio::ip::tcp::socket socket) {
    if (ec) {
      fail(ec, "accept");
    } else {
      // Create the session and run it
      //            boost::asio::io_service io_service;
      std::make_shared<session<int>>(std::move(socket), ioc_)->run();
    }

        // Accept another connection
    counter_conection++;
    do_accept();
  }
};

}


#endif // WSS_H
