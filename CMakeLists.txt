cmake_minimum_required(VERSION 3.0)
project(musketeer)

set(CMAKE_CXX_COMPILER "/usr/local/bin/g++")

set(CMAKE_CXX_FLAGS "-std=c++17 -g -pthread")

add_definitions(-D_GNU_SOURCE)

link_libraries(pthread)

aux_source_directory(base BASE_SOURCE)
aux_source_directory(event EVENT_SOURCE)
aux_source_directory(event/poller POLLER_SOURCE)
aux_source_directory(net NET_SOURCE)
aux_source_directory(net/http HTTP_SOURCE)
aux_source_directory(net/http/plugins PLUGINS_SOURCE)

set(SOURCES musketeer.cpp ${BASE_SOURCE} ${EVENT_SOURCE} ${POLLER_SOURCE}
${NET_SOURCE} ${HTTP_SOURCE} ${PLUGINS_SOURCE} thirdparty/http-parser/http_parser.c)

include_directories(${PROJECT_BINARY_DIR})

add_executable(musketeer ${SOURCES})
