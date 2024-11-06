cmake_minimum_required(VERSION 3.28)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # For clang-tidy.
set(BUILD_SHARED_LIBS OFF) # We expect external libraries to be linked statically.
project(redis LANGUAGES CXX)

# Specify the desired output directories
set(BUILD_DIR "${CMAKE_SOURCE_DIR}/build")
set(OUTPUT_DIR "${CMAKE_SOURCE_DIR}/output")

# Create directories if they do not exist
file(MAKE_DIRECTORY ${BUILD_DIR})
file(MAKE_DIRECTORY ${OUTPUT_DIR})

set(CMAKE_CXX_STANDARD 23)
set(GTEST_GIT_TAG v1.15.2)

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

# Gtest
include(cmake/FindGtest.cmake)

# seastar
find_package(Seastar REQUIRED)

# Boost
find_package(Boost REQUIRED)

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
target_link_libraries(redis PRIVATE Seastar::seastar server_lib resp3_lib)

# #####################################################################################################################
# TEST TARGETS
# #####################################################################################################################
enable_testing()
add_executable(protocol_test tests/protocol_test.cc)
target_link_libraries(protocol_test GTest::gtest_main server_lib)
add_test(NAME protocol_test COMMAND protocol_test)

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
