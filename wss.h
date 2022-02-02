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


namespace exchange {


//namespace beast = boost::beast;         // from <boost/beast.hpp>
//namespace http = beast::http;           // from <boost/beast/http.hpp>
//namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
//namespace net = boost::asio;            // from <boost/asio.hpp>
//using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

//------------------------------------------------------------------------------

// Report a failure
static void fail(boost::beast::error_code ec, char const* what);

// Echoes back all received WebSocket messages
class session : public std::enable_shared_from_this<session>
{
    boost::beast::websocket::stream<boost::beast::tcp_stream> ws_;
    boost::beast::flat_buffer buffer_;
    boost::asio::io_context &ioc;
    boost::asio::deadline_timer t;
    boost::asio::deadline_timer t_streamer;

public:
    // Take ownership of the socket
    explicit session(boost::asio::ip::tcp::socket&& socket, boost::asio::io_context &io_cont);

    // Get on the correct executor
    void run();

    // Start the asynchronous operation
    void on_run();

    void on_accept(boost::beast::error_code ec);

    void do_read();

    void stop_stream();

    void keep_alive(const boost::system::error_code &ec);

    void send_frame( boost::beast::error_code ec);

    void on_read( boost::beast::error_code ec, std::size_t bytes_transferred);

    void on_write_frame(boost::beast::error_code ec, std::size_t bytes_transferred);

    void on_write( boost::beast::error_code ec, std::size_t bytes_transferred);

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

    void on_accept(boost::beast::error_code ec, boost::asio::ip::tcp::socket socket);
};

}


#endif // WSS_H
