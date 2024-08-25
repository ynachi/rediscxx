//
// Created by ynachi on 8/17/24.
//

#ifndef CONNECTION_HH
#define CONNECTION_HH

#include <seastar/net/api.hh>
#include <seastar/util/log.hh>
#include "protocol.hh"

namespace redis {
    class Connection : public std::enable_shared_from_this<Connection> {
        struct Private {
            explicit Private() = default;
        };

    public:
        seastar::future<> process_frames();

        std::shared_ptr<Connection> get_ptr() { return shared_from_this(); }

        static std::shared_ptr<Connection> create(seastar::accept_result accept_result,
                                                  const seastar::lw_shared_ptr<seastar::logger> &logger) {
            return std::make_shared<Connection>(Private(), std::move(accept_result), logger);
        }

        explicit Connection(Private, seastar::accept_result accept_result,
                            const seastar::lw_shared_ptr<seastar::logger> &logger) noexcept :
            _input_stream(accept_result.connection.input()), _output_stream(accept_result.connection.output()),
            _remote_address(accept_result.remote_address), _logger(logger), _buffer() {
            this->_logger->info("server accepted a new connection {}", this->_remote_address);
        }

        // Delete all other constructors to enforce factory method usage
        Connection(const Connection &) = delete;

        Connection(Connection &&) = delete;

        Connection &operator=(const Connection &) = delete;

        Connection &operator=(Connection &&) = delete;

    private:
        seastar::input_stream<char> _input_stream;
        seastar::output_stream<char> _output_stream;
        seastar::socket_address _remote_address;
        seastar::lw_shared_ptr<seastar::logger> _logger;
        BufferManager _buffer;
    };
} // namespace redis


#endif // CONNECTION_HH
