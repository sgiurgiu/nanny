cmake_minimum_required(VERSION 3.0)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build (Debug or Release)" FORCE)
endif()
set(CMAKE_C_STANDARD 11)

project(nanny)

add_definitions(-Wall -pedantic -Wextra -DMG_ENABLE_HTTP_WEBSOCKET=0 -DMG_ENABLE_MQTT=0)


if(CMAKE_BUILD_TYPE MATCHES Debug)
    add_definitions(-DNANNY_DEBUG)
endif(CMAKE_BUILD_TYPE MATCHES Debug)

set(NANNY_SRC src/main.c
              src/http_server.c
              src/database.c
              src/mongoose.c
              src/sqlite3.c
   )
if(CMAKE_SYSTEM_NAME MATCHES OpenBSD)
    list(APPEND NANNY_SRC src/pf_openbsd.c)
else(CMAKE_SYSTEM_NAME MATCHES OpenBSD)
    list(APPEND NANNY_SRC src/pf_mock.c)
endif(CMAKE_SYSTEM_NAME MATCHES OpenBSD)

set(EXECUTABLE_NAME "nanny")

add_executable(${EXECUTABLE_NAME} ${NANNY_SRC})
target_link_libraries(${EXECUTABLE_NAME} pthread dl)
install(TARGETS ${EXECUTABLE_NAME} RUNTIME DESTINATION bin)
