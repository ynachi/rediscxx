//
// Created by ynachi on 8/17/24.
//
#include <connection.hh>

namespace redis {
    seastar::future<> Connection::process_frames() {
        std::cout << "entered process frames" << "\n";
        // the caller and this method needs to make sure the connection
        // object is not out of scope, so use a shared reference from
        // the current connection.
        auto const self = this->get_ptr();
        while (true) {
            if (self->_input_stream.eof()) {
                self->_logger->debug("connection was closed by the user");
                break;
            }
            auto tmp_read_buf = co_await self->_input_stream.read();
            if (tmp_read_buf.empty()) {
                self->_logger->debug("connection was closed by the user");
                break;
            }
            this->_buffer.add_upstream_data(std::move(tmp_read_buf));
            auto data = this->_buffer.get_simple_string();
            std::cout << data.value() << "\n";
            if (data.error() == FrameDecodeError::Incomplete) {
                continue;
            }
            if (!data.has_value()) {
                self->_logger->debug("error decoding frame");
                continue;
            }
            const auto &ans = data.value();
            std::cout << data.value() << "\n";
            co_await self->_output_stream.write(data.value());
            co_await self->_output_stream.flush();
        }
        co_await self->_output_stream.close();
        co_return;
    }
} // namespace redis
