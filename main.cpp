#include <iostream>
#include <server.hh>

int main()
{
    using boost::asio::awaitable;
    using boost::asio::co_spawn;
    using boost::asio::detached;
    using boost::asio::socket_base;
    using boost::asio::use_awaitable;
    using boost::asio::ip::tcp;

    try
    {
        boost::asio::io_context ctx(8);

        const tcp::endpoint endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 9292);

        const auto server = redis::Server::create(ctx, endpoint, true, 8);
        server->start();
    }
    catch (std::exception &e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }
}
