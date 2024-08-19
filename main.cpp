#include <seastar/core/app-template.hh>
#include <server.hh>

int main(const int argc, char **argv) {
    seastar::app_template app;
    return app.run(argc, argv, []() -> seastar::future<> {
        auto server = redis::Server("127.0.0.2", 8092, true);
        std::cout << "server started listening to connections\n";
        co_await server.listen();
    });
}
