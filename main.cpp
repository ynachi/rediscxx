#include <iostream>
#include <server.hh>
#include "photon/common/alog.h"
#include <glog/logging.h>


int main()
{
    log_output_level = ALOG_INFO;
    google::InitGoogleLogging("redisxx");
    LOG(INFO) << "Found " << 5 << " cookies";

    const auto config = redis::ServerConfig();
    auto server = redis::Server(config);
    server.run();
}
