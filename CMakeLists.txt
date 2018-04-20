cmake_minimum_required(VERSION 3.0)
project(musketeer)

set(CMAKE_CXX_COMPILER "g++")

add_compile_options(-std=c++17 -g -Wall -Werror -pthread)
add_definitions(-D_GNU_SOURCE)

link_libraries(pthread)

aux_source_directory(base BASE_SOURCE)
aux_source_directory(event EVENT_SOURCE)
aux_source_directory(event/poller POLLER_SOURCE)
aux_source_directory(net NET_SOURCE)

set(SOURCES musketeer.cpp ${BASE_SOURCE} ${EVENT_SOURCE} ${POLLER_SOURCE} ${NET_SOURCE})

include_directories(${PROJECT_BINARY_DIR})

add_executable(musketeer ${SOURCES})
