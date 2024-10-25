#include <seastar/core/app-template.hh>
#include <seastar/net/api.hh>
#include <seastar/testing/test_case.hh>
#include <seastar/testing/thread_test_case.hh>
#include <seastar/core/sleep.hh>
#include <seastar/testing/test_runner.hh>
#include "framer/handler.h"

using namespace seastar;
using namespace seastar::testing;

SEASTAR_THREAD_TEST_CASE(test_parser_simple)
{
    auto server_socket = net::make_server_socket(net::ipv4_addr::make_loopback(), 12345);
}