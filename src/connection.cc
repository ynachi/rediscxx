//
// Created by ynachi on 8/17/24.
//
#include <connection.hh>

namespace redis {
    seastar::future<> Connection::process_frames() {
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
            co_await self->_output_stream.write(std::move(tmp_read_buf));
            co_await self->_output_stream.flush();
        }
        co_await self->_output_stream.close();
        co_return;
    }
}

