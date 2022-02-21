#include <iostream>
#include "include/wss.hpp"

//#include "wsslibConfig.h"
//------------------------------------------------------------------------------



int main(int argc, char* argv[])
{
    std::cout << "argc=" << argc << " " << *argv << std::endl;
//    return 0;
    server_wss::version();
    // Check command line arguments.
    if (argc != 4)
    {
        std::cerr <<
//            "WSS Version " << wssserver_VERSION_MAJOR << "." << wssserver_VERSION_MINOR << std::endl <<
            "Usage: websocket-server-async <address> <port> <threads>\n" <<
            "Example:\n" <<
            "    websocket-server-async 0.0.0.0 8080 1\n";
        return EXIT_FAILURE;
    }
    auto const address = boost::asio::ip::make_address(argv[1]);
    auto const port = static_cast<unsigned short>(std::atoi(argv[2]));
    auto const threads = std::max<int>(1, std::atoi(argv[3]));

    // The io_context is required for all I/O
    boost::asio::io_context ioc{threads};

    // Create and launch a listening port
    std::make_shared<server_wss::listener>(ioc, boost::asio::ip::tcp::endpoint{address, port})->run();

    // Run the I/O service on the requested number of threads
    std::vector<std::thread> v;
    v.reserve(static_cast<size_t>(threads - 1));
    for(auto i = threads - 1; i > 0; --i)
        v.emplace_back(
        [&ioc]
        {
            ioc.run();
        });
    ioc.run();
    std::cout << "********" << std::endl;

    return EXIT_SUCCESS;
}
