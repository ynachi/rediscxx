#include <iostream>
#include <server.hh>

int main()
{
    const auto config = redis::ServerConfig();
    auto server = redis::Server(config);
    server.start_accept_loop();
}
