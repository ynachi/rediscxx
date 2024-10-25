cmake_minimum_required(VERSION 3.28)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # For clang-tidy.
set(BUILD_SHARED_LIBS OFF) # We expect external libraries to be linked statically.
project(redis LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)

set(GTEST_GIT_TAG v1.15.2)
#cmake_policy (SET CMP0090 NEW)
#foreach (policy CMP0127 CMP0135 CMP0167)
#    if (POLICY ${policy})
#        cmake_policy (SET ${policy} NEW)
#    endif ()
#endforeach ()

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

# Gtest
include(cmake/FindGtest.cmake)

# Google benchmark
find_package(benchmark REQUIRED)
# how to use: target_link_libraries(MyTarget benchmark::benchmark)

# Boost
find_package(Boost COMPONENTS system filesystem REQUIRED)

# seastar
find_package(Seastar REQUIRED)

# project include dirs
include_directories(${CMAKE_SOURCE_DIR}/include)

# #####################################################################################################################
# TARGETS
# #####################################################################################################################

# Framer
set(FRAMER_HEADERS include/framer/frame_id.h include/framer/frame.h include/framer/handler.h)
set(FRAMER_SOURCES src/framer/frame_id.cpp src/framer/frame.cpp src/framer/handler.cpp)
add_library(framer_lib ${FRAMER_SOURCES} ${FRAMER_HEADERS} )
target_link_libraries(framer_lib PRIVATE Seastar::seastar)

# server
set(SERVER_HEADERS include/server.hh include/connection.hh include/parser.hh include/commands.hh include/strings.hh)
set(SERVER_SOURCES src/server.cc src/connection.cc src/parser.cc src/commands.cc src/strings.cc)
add_library(server_lib ${SERVER_SOURCES} ${SERVER_HEADERS})
target_link_libraries(server_lib PRIVATE Boost::headers)

add_executable(redis main.cpp)
target_link_libraries(redis PRIVATE server_lib resp3_lib)

# #####################################################################################################################
# TEST TARGETS
# #####################################################################################################################
enable_testing()
add_executable(protocol_test tests/protocol_test.cc)
target_link_libraries(protocol_test GTest::gtest_main server_lib resp3_lib)
add_test(NAME protocol_test COMMAND protocol_test)

add_executable(frame_test tests/frame_test.cc)
target_link_libraries(frame_test GTest::gtest_main resp3_lib)
add_test(NAME frame_test COMMAND frame_test)

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
