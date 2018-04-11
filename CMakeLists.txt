cmake_minimum_required(VERSION 3.0)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build (Debug or Release)" FORCE)
endif()
set(CMAKE_C_STANDARD 11)
project(nanny)
list (APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

add_definitions( -Wall -pedantic -Wextra -DMG_ENABLE_HTTP_WEBSOCKET=0 -DMG_ENABLE_MQTT=0)

find_package (Jansson REQUIRED)
find_package( Threads ) 
find_package (OpenSSL REQUIRED)

find_path(PFVAR_INCLUDE_DIR net/pfvar.h
    /usr/local/include
    /usr/include
)

message(STATUS "Net/pfvar.h detected ${PFVAR_INCLUDE_DIR}")

if(CMAKE_BUILD_TYPE MATCHES Debug)
    add_definitions(-DNANNY_DEBUG -DCS_ENABLE_DEBUG=1)
endif(CMAKE_BUILD_TYPE MATCHES Debug)

include_directories(${JANSSON_INCLUDE_DIRS})
include_directories(external/jwt/include ${OPENSSL_INCLUDE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(NANNY_SRC src/main.c
              src/http_server.c
              src/database.c
              src/mongoose.c
              src/sqlite3.c
   )
   
set(JWT_SRC external/jwt/libjwt/base64.c
    external/jwt/libjwt/jwt.c
    external/jwt/libjwt/jwt-openssl.c
    )
write_file(${CMAKE_CURRENT_BINARY_DIR}/config.h "")    

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

add_executable(${EXECUTABLE_NAME} ${NANNY_SRC} ${JWT_SRC})
target_link_libraries(${EXECUTABLE_NAME} ${CMAKE_DL_LIBS} ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(${EXECUTABLE_NAME} ${JANSSON_LIBRARIES} ${OPENSSL_CRYPTO_LIBRARY})

install(TARGETS ${EXECUTABLE_NAME} RUNTIME DESTINATION bin)




target_link_libraries(${EXECUTABLE_NAME} ${JWT_LIBRARIES})
