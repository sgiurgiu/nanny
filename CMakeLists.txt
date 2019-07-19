cmake_minimum_required(VERSION 3.0)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build (Debug or Release)" FORCE)
endif()
set(CMAKE_C_STANDARD 11)
project(nanny)

option(BUILD_TESTS "Build the tests" ON)

list (APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

add_definitions( -Wall -pedantic -Wextra -DMG_ENABLE_HTTP_WEBSOCKET=0 -DMG_ENABLE_MQTT=0 -Wformat -Wsign-compare)

add_subdirectory(src)

if(BUILD_TESTS)
    add_subdirectory(tests)
endif(BUILD_TESTS)
