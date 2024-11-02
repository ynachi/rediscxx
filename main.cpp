#include <iostream>
#include <seastar/core/app-template.hh>
#include <server.hh>

int main(const int argc, char **argv)
{
    seastar::app_template app;
    return app.run(argc, argv,
                   []() -> seastar::future<>
                   {
                       auto config = redis::ServerConfig();
                       auto server = redis::Server(std::move(config));
                       std::cout << "server started listening to connections\n";
                       return  server.listen();
                   });
}
