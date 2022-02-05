#include "wss.h"

void exchange::fail(boost::beast::error_code ec, const char *what)
{
    std::cerr << what << ": " << " -- " << ec.what() << "\n" << std::flush;
}

exchange::session::session(boost::asio::ip::tcp::socket &&socket, boost::asio::io_context &io_cont)
    : ws_(std::move(socket)),ioc(io_cont), t(io_cont, boost::posix_time::seconds(0)), t_streamer(io_cont, boost::posix_time::seconds(0))
{
}

void exchange::session::run()
{
    // We need to be executing within a strand to perform async operations
    // on the I/O objects in this session. Although not strictly necessary
    // for single-threaded contexts, this example code is written to be
    // thread-safe by default.
    boost::asio::dispatch(ws_.get_executor(),
                          boost::beast::bind_front_handler(
                              &session::on_run,
                              shared_from_this()));
}

void exchange::session::on_run()
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
                    shared_from_this()));
}

void exchange::session::on_accept(boost::beast::error_code ec)
{
    if(ec)
        return fail(ec, "accept");

    // Read a message
    send_frame(boost::beast::error_code());
    do_read();
}

void exchange::session::do_read()
{
    // Read a message into our buffer
    ws_.async_read(
                buffer_,
                boost::beast::bind_front_handler(
                    &session::on_read,
                    shared_from_this()));
}

void exchange::session::stop_stream()
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

void exchange::session::keep_alive(const boost::system::error_code &ec)
{
    if(ec == boost::asio::error::operation_aborted)
        std::cout << "keep_alive() " << t.expires_from_now() << std::endl << std::flush;
    else
        stop_stream();
}

void exchange::session::send_frame(boost::beast::error_code ec)
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
                    shared_from_this()));
    std::cout << "send_frame_exit()" << std::endl << std::flush;
}

void exchange::session::on_read(boost::beast::error_code ec, std::size_t bytes_transferred)
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
                    shared_from_this()));
    std::cout << "on_read:exit() " << t.expires_from_now() << std::endl << std::flush;
}

void exchange::session::on_write_frame(boost::beast::error_code ec, std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);
    if(ec)
        return fail(ec, "write");
    //        buffer_.consume(buffer_.size());
    t_streamer.expires_from_now(boost::posix_time::seconds(1));
    t_streamer.async_wait(boost::bind(&session::send_frame, this, boost::asio::placeholders::error));
    //        count_pack++;
}

void exchange::session::on_write(boost::beast::error_code ec, std::size_t bytes_transferred)
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

exchange::session::~session() {
    std::cout << "destructor session \n" << std::endl << std::flush;
}

exchange::listener::listener(boost::asio::io_context &ioc, boost::asio::ip::tcp::endpoint endpoint)
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

void exchange::listener::run() {
    do_accept();
}

void exchange::listener::do_accept() {
    // The new connection gets its own strand
    //        if(counter_conection < MAX_CONNECTION)
    acceptor_.async_accept( boost::asio::make_strand(ioc_), boost::beast::bind_front_handler( &listener::on_accept, shared_from_this()));
}

void exchange::listener::on_accept(boost::beast::error_code ec, boost::asio::ip::tcp::socket socket) {
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

void exchange::version()
{
    std::cout << "Web socket server version " << CWSS_VERSION_MAJOR << "." << CWSS_VERSION_MINOR << std::endl;
}
