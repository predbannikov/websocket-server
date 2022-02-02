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

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

//------------------------------------------------------------------------------

// Report a failure
void fail(beast::error_code ec, char const* what)
{
    std::cerr << what << ": " << " -- " << ec.what() << "\n" << std::flush;
}

// Echoes back all received WebSocket messages
class session : public std::enable_shared_from_this<session>
{
    websocket::stream<beast::tcp_stream> ws_;
    beast::flat_buffer buffer_;
    net::io_context &ioc;
    boost::asio::deadline_timer t;
    boost::asio::deadline_timer t_streamer;

public:
    // Take ownership of the socket
    explicit session(tcp::socket&& socket, boost::asio::io_context &io_cont)
        : ws_(std::move(socket)),ioc(io_cont), t(io_cont, boost::posix_time::seconds(0)), t_streamer(io_cont, boost::posix_time::seconds(0))
    {
    }

    // Get on the correct executor
    void run()
    {
        // We need to be executing within a strand to perform async operations
        // on the I/O objects in this session. Although not strictly necessary
        // for single-threaded contexts, this example code is written to be
        // thread-safe by default.
        net::dispatch(ws_.get_executor(),
            beast::bind_front_handler(
                &session::on_run,
                shared_from_this()));
    }

    // Start the asynchronous operation
    void on_run()
    {
        // Set suggested timeout settings for the websocket
        ws_.set_option(
            websocket::stream_base::timeout::suggested(
                beast::role_type::server));

        // Set a decorator to change the Server of the handshake
        ws_.set_option(websocket::stream_base::decorator(
            [](websocket::response_type& res)
            {
                res.set(http::field::server,
                    std::string(BOOST_BEAST_VERSION_STRING) +
                        " websocket-server-async");
            }));
        // Accept the websocket handshake
        ws_.async_accept(
            beast::bind_front_handler(
                &session::on_accept,
                shared_from_this()));
    }

    void on_accept(beast::error_code ec)
    {
        if(ec)
            return fail(ec, "accept");

        // Read a message
        send_frame(beast::error_code());
        do_read();
    }

    void do_read()
    {
        // Read a message into our buffer
        ws_.async_read(
            buffer_,
            beast::bind_front_handler(
                &session::on_read,
                shared_from_this()));
    }

    void stop_stream()
    {
        try {
            std::cout << "stop_stream() " << t.expires_from_now() << std::endl << std::flush;
//            beast::websocket::close_reason cr;
//            ws_.close(cr);
            ws_.close(websocket::close_code::normal);
            std::cout << "wss close socket:" << std::endl << std::flush;

        }  catch (boost::system::error_code ec) {
            std::cerr << ec.message() << std::endl << ec.what() << std::endl << std::flush;
        }
    }

    void keep_alive(const boost::system::error_code &ec)
    {
        if(ec == boost::asio::error::operation_aborted)
            std::cout << "keep_alive() " << t.expires_from_now() << std::endl << std::flush;
        else
            stop_stream();
    }

    void send_frame( beast::error_code ec)
    {
        std::cout << "send_frame_start() " << t.expires_from_now() << std::endl << std::flush;
        if(ec) {
            return fail(ec, "read");

        }
        std::string str = std::to_string(rand()%1000000);
        ws_.text(true);
        ws_.async_write(
            boost::asio::buffer(str.data(), str.length()),
            beast::bind_front_handler(
                &session::on_write_frame,
                shared_from_this()));
        std::cout << "send_frame_exit()" << std::endl << std::flush;
    }

    void on_read( beast::error_code ec, std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);
        std::cout << "set timer " << beast::buffers_to_string(buffer_.data()) << t.expires_from_now() << std::endl << std::flush;
        if(ec) {
            return fail(ec, "read");
        }
        t.expires_from_now(boost::posix_time::seconds(5));
        t.async_wait(boost::bind(&session::keep_alive, this, boost::asio::placeholders::error));

        // Echo the message
        ws_.text(ws_.got_text());
        ws_.async_write(
            buffer_.data(),
            beast::bind_front_handler(
                &session::on_write,
                shared_from_this()));
        std::cout << "on_read:exit() " << t.expires_from_now() << std::endl << std::flush;
    }

    void on_write_frame(beast::error_code ec, std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);
        if(ec)
            return fail(ec, "write");
//        buffer_.consume(buffer_.size());
        t_streamer.expires_from_now(boost::posix_time::seconds(1));
        t_streamer.async_wait(boost::bind(&session::send_frame, this, boost::asio::placeholders::error));
//        count_pack++;
    }

    void on_write( beast::error_code ec, std::size_t bytes_transferred)
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

    ~session() {
        std::cout << "destructor session \n" << std::endl << std::flush;
    }
//    unsigned long count_pack = 0;
};

//------------------------------------------------------------------------------
//#define MAX_CONNECTION 2
// Accepts incoming connections and launches the sessions
class listener : public std::enable_shared_from_this<listener>
{
    net::io_context& ioc_;
    tcp::acceptor acceptor_;
    int counter_conection = 0;
public:
    listener(
        net::io_context& ioc,
        tcp::endpoint endpoint)
        : ioc_(ioc)
        , acceptor_(ioc)
    {
        beast::error_code ec;

        // Open the acceptor
        acceptor_.open(endpoint.protocol(), ec);
        if(ec)
        {
            fail(ec, "open");
            return;
        }

        // Allow address reuse
        acceptor_.set_option(net::socket_base::reuse_address(true), ec);
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
            net::socket_base::max_listen_connections, ec);
        if(ec)
        {
            fail(ec, "listen");
            return;
        }
    }

    // Start accepting incoming connections
    void run() {
        do_accept();
    }

private:
    void do_accept() {
        // The new connection gets its own strand
//        if(counter_conection < MAX_CONNECTION)
            acceptor_.async_accept( net::make_strand(ioc_), beast::bind_front_handler( &listener::on_accept, shared_from_this()));
    }

    void on_accept(beast::error_code ec, tcp::socket socket) {
        if(ec)
        {
            fail(ec, "accept");
        }
        else
        {
            // Create the session and run it
//            boost::asio::io_service io_service;
            std::make_shared<session>(std::move(socket), ioc_)->run();
        }

        // Accept another connection
        counter_conection++;
        do_accept();
    }
};

//------------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    // Check command line arguments.
    if (argc != 4)
    {
        std::cerr <<
            "Usage: websocket-server-async <address> <port> <threads>\n" <<
            "Example:\n" <<
            "    websocket-server-async 0.0.0.0 8080 1\n";
        return EXIT_FAILURE;
    }
    auto const address = net::ip::make_address(argv[1]);
    auto const port = static_cast<unsigned short>(std::atoi(argv[2]));
    auto const threads = std::max<int>(1, std::atoi(argv[3]));

    // The io_context is required for all I/O
    net::io_context ioc{threads};

    // Create and launch a listening port
    std::make_shared<listener>(ioc, tcp::endpoint{address, port})->run();

    // Run the I/O service on the requested number of threads
    std::vector<std::thread> v;
    v.reserve(threads - 1);
    for(auto i = threads - 1; i > 0; --i)
        v.emplace_back(
        [&ioc]
        {
            ioc.run();
        });
    ioc.run();

    return EXIT_SUCCESS;
}
