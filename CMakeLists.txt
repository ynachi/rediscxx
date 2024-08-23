cmake_minimum_required(VERSION 3.29)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # For clang-tidy.
set(BUILD_SHARED_LIBS OFF) # We expect external libraries to be linked statically.
project(redis LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)


# #####################################################################################################################
# DEPENDENCIES
# #####################################################################################################################

# Gtest
include(cmake/FindGtest.cmake)

# seastar
find_package(Seastar REQUIRED)

# project include dirs
include_directories(${CMAKE_SOURCE_DIR}/include)

# #####################################################################################################################
# TARGETS
# #####################################################################################################################

# resp3
set(RESP3_HEADERS include/frame_id.hh include/frame.hh)
set(RESP3_SOURCES src/frame_id.cc src/frame.cc)
add_library(resp3_lib ${RESP3_SOURCES} ${RESP3_HEADERS})

# server
set(SERVER_HEADERS include/server.hh include/connection.hh include/protocol.hh)
set(SERVER_SOURCES src/server.cc src/connection.cc src/protocol.cc)
add_library(server_lib ${SERVER_SOURCES} ${SERVER_HEADERS})
target_link_libraries(server_lib PRIVATE Seastar::seastar)

add_executable(redis main.cpp)
target_link_libraries(redis PRIVATE Seastar::seastar server_lib)

# #####################################################################################################################
# TEST TARGETS
# #####################################################################################################################
add_executable(decoder_test tests/protocol_test.cc)
target_link_libraries(decoder_test GTest::gtest_main server_lib)
add_test(NAME DecoderTest COMMAND decoder_test)

include(GNUInstallDirs)
install(TARGETS redis
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
