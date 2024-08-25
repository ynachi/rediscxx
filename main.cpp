#include <iostream>
#include <server.hh>

int main(const int argc, char **argv)
{
    using boost::asio::awaitable;
    using boost::asio::co_spawn;
    using boost::asio::detached;
    using boost::asio::socket_base;
    using boost::asio::use_awaitable;
    using boost::asio::ip::tcp;

    try
    {
        boost::asio::io_context ctx;

        boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 9292);

        auto server = redis::Server(endpoint, true);
        server.run();
    }
    catch (std::exception &e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }
}
