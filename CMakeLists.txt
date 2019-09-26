cmake_minimum_required(VERSION 2.8)

PROJECT(corpc)

set(CMAKE_CXX_COMPILER "g++")
set(CMAKE_C_COMPILER "gcc")

set(CMAKE_CXX_FLAGS "-g2 -Wall -D_REENTRANT -Wno-invalid-offsetof -fPIC -std=c++11")

message(${CMAKE_CXX_COMPILER})

include_directories(${PROJECT_SOURCE_DIR}/libconet/include)
include_directories(${PROJECT_SOURCE_DIR}/libzk/include)
include_directories(${PROJECT_SOURCE_DIR}/libmysql_cli/include)
include_directories(${PROJECT_SOURCE_DIR}/corpc/include)
include_directories(${PROJECT_SOURCE_DIR}/libshmconfig_server/include)
include_directories(${PROJECT_SOURCE_DIR}/libshmconfig_agent/include)
include_directories(${PROJECT_SOURCE_DIR}/libshm_config/include)
include_directories(${PROJECT_SOURCE_DIR}/libxmlconfig_server/include)
include_directories(${PROJECT_SOURCE_DIR}/libxml_config/include)

add_subdirectory(libconet)
add_subdirectory(libzk)
add_subdirectory(libmysql_cli)
add_subdirectory(corpc)
add_subdirectory(libshmconfig_server)
add_subdirectory(libshmconfig_agent)
add_subdirectory(libshm_config)
add_subdirectory(libxmlconfig_server)
add_subdirectory(libxml_config)

