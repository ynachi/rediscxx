cmake_minimum_required(VERSION 3.28)
if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.30.0")
    cmake_policy(SET CMP0167 OLD)
endif ()
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # For clang-tidy.
set(BUILD_SHARED_LIBS OFF) # We expect external libraries to be linked statically.
project(redis LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)

#add_compile_options(-fsanitize=address -g)
#add_link_options(-fsanitize=address)

set(REDISCXX_BUILD_SUPPORT_DIR "${CMAKE_SOURCE_DIR}/build_support")

# #####################################################################################################################
# DEPENDENCIES
# #####################################################################################################################

# attempt to find the binary if user did not specify
find_program(CLANG_FORMAT_BIN NAMES clang-format clang-format-18)
if ("${CLANG_FORMAT_BIN}" STREQUAL "CLANG_FORMAT_BIN-NOTFOUND")
    message(WARNING "redis/main couldn't find clang-format.")
else ()
    message(STATUS "redis/main found clang-format at ${CLANG_FORMAT_BIN}")
endif ()

# Photon
include(cmake/FindPhoton.cmake)

# Gtest
include(cmake/FindGtest.cmake)

# project include dirs
include_directories(${CMAKE_SOURCE_DIR}/include)

# #####################################################################################################################
# TARGETS
# #####################################################################################################################

# frame handler
set(FRAME_HANDLER_HEADERS include/framer/handler.h include/framer/frame.h include/framer/frame_id.h)
set(FRAME_HANDLER_SOURCES src/framer/handler.cpp src/framer/frame.cpp src/framer/frame_id.cpp)
add_library(framer_lib ${FRAME_HANDLER_SOURCES} ${FRAME_HANDLER_HEADERS})
target_link_libraries(framer_lib PRIVATE photon_static)

# in memory stream for testing
add_library(memory_stream_lib include/memory_stream/mstream.h)
target_link_libraries(memory_stream_lib PRIVATE photon_static)

# server
set(SERVER_HEADERS include/server.hh include/strings.hh)
set(SERVER_SOURCES src/server.cc src/strings.cc)
add_library(server_lib ${SERVER_SOURCES} ${SERVER_HEADERS})
target_link_libraries(server_lib PRIVATE photon_static)

add_executable(redis main.cpp)
target_link_libraries(redis PRIVATE server_lib photon_static)

add_executable(poton src/poton.cpp)
target_link_libraries(poton PRIVATE photon_static)

# #####################################################################################################################
# TEST TARGETS
# #####################################################################################################################
enable_testing()
add_executable(protocol_test tests/protocol_test.cc)
target_link_libraries(protocol_test GTest::gtest_main server_lib resp3_lib)
add_test(NAME protocol_test COMMAND protocol_test)

add_executable(memory_stream_test tests/memory_stream/mstream_test.cpp)
target_link_libraries(memory_stream_test GTest::gtest_main memory_stream_lib photon_static)
add_test(NAME memory_stream_test COMMAND memory_stream_test)

add_executable(parser_test tests/framer/parser_test.cpp)
target_link_libraries(parser_test GTest::gtest_main framer_lib memory_stream_lib photon_static)
add_test(NAME parser_test COMMAND parser_test)

include(GNUInstallDirs)
install(TARGETS redis
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# #####################################################################################################################
# MAKE TARGETS
# #####################################################################################################################

# #########################################
# "make format"
# "make check-format"
# #########################################
string(CONCAT REDISCXX_FORMAT_DIRS
        "${CMAKE_CURRENT_SOURCE_DIR}/src,"
        "${CMAKE_CURRENT_SOURCE_DIR}/include,"
        "${CMAKE_CURRENT_SOURCE_DIR}/test,"
)

# Runs clang format and updates files in place.
add_custom_target(format ${REDISCXX_BUILD_SUPPORT_DIR}/run_clang_format.py
        ${CLANG_FORMAT_BIN}
        ${REDISCXX_BUILD_SUPPORT_DIR}/clang_format_exclusions.txt
        --source_dirs
        ${REDISCXX_FORMAT_DIRS}
        --fix
        --quiet
)

# Runs clang format and exits with a non-zero exit code if any files need to be reformatted
add_custom_target(check-format ${REDISCXX_BUILD_SUPPORT_DIR}/run_clang_format.py
        ${CLANG_FORMAT_BIN}
        ${REDISCXX_BUILD_SUPPORT_DIR}/clang_format_exclusions.txt
        --source_dirs
        ${REDISCXX_FORMAT_DIRS}
        --quiet
)
