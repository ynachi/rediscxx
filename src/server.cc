//
// Created by ynachi on 8/17/24.
//
#include <seastar/core/seastar.hh>
#include <server.hh>
#include <connection.hh>

namespace redis {
    Server::Server(const std::string &ip_addr, const u_int16_t port, const bool reuse_addr)
        : _logger(make_lw_shared(seastar::logger("server"))) {
        seastar::listen_options listen_options;
        listen_options.reuse_address = reuse_addr;
        listen_options.lba = seastar::server_socket::load_balancing_algorithm::default_;

        try {
            auto const listen_address = seastar::make_ipv4_address({ip_addr, port});
            _listener = seastar::listen(listen_address, listen_options);
            _logger->info("server instance created at {}:{}", ip_addr, port);
        } catch (const std::exception &e) {
            _logger->error("invalid listening address: {}:{}, error: {}", ip_addr, port, e.what());
            seastar::engine_exit();
        }
    }

    Server::Server(Server &&other) noexcept: _logger(std::move(other._logger)), _listener(std::move(other._listener)) {
    }

    seastar::future<> Server::listen() {
        while (true) {
            try {
                auto accept_result = co_await this->_listener.accept();
                // create a connection object here
                auto const conn = Connection::create(std::move(accept_result), _logger);
                // we use void because we do not want to wait for the future, because connections
                // should be processed as soon as we get them.
                (void) seastar::futurize_invoke([conn] {
                    return conn->process_frames();
                });
            } catch (std::exception &e) {
                this->_logger->error("failed to accept a new connection to the server", e.what());
            }
        }
    }
}
