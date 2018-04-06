cmake_minimum_required(VERSION 3.0)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build (Debug or Release)" FORCE)
endif()
#set(CMAKE_C_STANDARD 11)
project(nanny)

add_definitions(-std=c99 -Wall -pedantic -Wextra -DMG_ENABLE_HTTP_WEBSOCKET=0 -DMG_ENABLE_MQTT=0)
find_package(PkgConfig REQUIRED)
pkg_search_module(JSON_C REQUIRED json-c)

find_path(PFVAR_INCLUDE_DIR net/pfvar.h
    /usr/local/include
    /usr/include
)

message(STATUS "Net/pfvar.h detected ${PFVAR_INCLUDE_DIR}")

if(CMAKE_BUILD_TYPE MATCHES Debug)
    add_definitions(-DNANNY_DEBUG -DCS_ENABLE_DEBUG=1)
endif(CMAKE_BUILD_TYPE MATCHES Debug)

include_directories(${JSON_C_INCLUDE_DIRS})
link_directories(${JSON_C_LIBRARY_DIRS})

set(NANNY_SRC src/main.c
              src/http_server.c
              src/database.c
              src/mongoose.c
              src/sqlite3.c
   )

set_source_files_properties(src/mongoose.c PROPERTIES COMPILE_FLAGS -Wno-format COMPILE_FLAGS -Wno-format-pedantic)
set_source_files_properties(src/sqlite3.c PROPERTIES COMPILE_FLAGS -Wno-implicit-fallthrough COMPILE_FLAGS -Wno-unused-parameter)

if(PFVAR_INCLUDE_DIR MATCHES PFVAR_INCLUDE_DIR-NOTFOUND)
    list(APPEND NANNY_SRC src/pf_mock.c)
    set_source_files_properties(src/pf_mock.c PROPERTIES COMPILE_FLAGS -Wno-unused-parameter)
else(PFVAR_INCLUDE_DIR MATCHES PFVAR_INCLUDE_DIR-NOTFOUND)
  include_directories(${PFVAR_INCLUDE_DIR})
   list(APPEND NANNY_SRC src/pf_real.c)
endif(PFVAR_INCLUDE_DIR MATCHES PFVAR_INCLUDE_DIR-NOTFOUND)

set(EXECUTABLE_NAME "nanny")

add_executable(${EXECUTABLE_NAME} ${NANNY_SRC})
target_link_libraries(${EXECUTABLE_NAME} pthread ${CMAKE_DL_LIBS})
target_link_libraries(${EXECUTABLE_NAME} ${JSON_C_LIBRARIES})

install(TARGETS ${EXECUTABLE_NAME} RUNTIME DESTINATION bin)
