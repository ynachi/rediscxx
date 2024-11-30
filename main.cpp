#include <iostream>
#include <server.hh>

#include "cmake-build-debug-coverage/_deps/photon-src/common/alog.h"

int main()
{
    log_output_level = ALOG_DEBUG;
    const auto config = redis::ServerConfig();
    auto server = redis::Server(config);
    server.run();
}
