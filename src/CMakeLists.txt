
#only because jwt wanted it
find_package (Jansson REQUIRED)
find_package( Threads ) 
find_package (OpenSSL REQUIRED)

find_path(PFVAR_INCLUDE_DIR net/pfvar.h
    /usr/local/include
    /usr/include
)

message(STATUS "Net/pfvar.h detected ${PFVAR_INCLUDE_DIR}")

if(CMAKE_BUILD_TYPE MATCHES Debug)
    add_definitions(-DNANNY_DEBUG -DCS_ENABLE_DEBUG=0)
endif(CMAKE_BUILD_TYPE MATCHES Debug)

include_directories(${JANSSON_INCLUDE_DIRS})
include_directories(../external/jwt/include ${OPENSSL_INCLUDE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(../external/libbcrypt/)
include_directories(../external/sclog4c/include/)

set(NANNY_MAIN_SRC main.c)
set(NANNY_SRC 
              http_server.c
              database.c
              mongoose.c
              sqlite3.c
              block_controller.c
              allow_controller.c
              status_controller.c
              login_controller.c
              http_utils.c
              domain.c
              token_provider.c
              admin_controller.c
              security_utils.c
   )
   
set(SCLOG4C_SRC ../external/sclog4c/src/sclog4c.c)

set(JWT_SRC ../external/jwt/libjwt/base64.c
    ../external/jwt/libjwt/jwt.c
    ../external/jwt/libjwt/jwt-openssl.c
    )
write_file(${CMAKE_CURRENT_BINARY_DIR}/config.h "")    

set(BCRYPT_SRC ../external/libbcrypt/bcrypt.c
                ../external/libbcrypt/crypt_blowfish/crypt_blowfish.c
                ../external/libbcrypt/crypt_blowfish/crypt_gensalt.c
                ../external/libbcrypt/crypt_blowfish/wrapper.c
                ../external/libbcrypt/crypt_blowfish/x86.S
                )

set_source_files_properties(mongoose.c PROPERTIES COMPILE_FLAGS -Wno-format COMPILE_FLAGS -Wno-format-pedantic)
set_source_files_properties(sqlite3.c PROPERTIES COMPILE_FLAGS -Wno-implicit-fallthrough)

if(PFVAR_INCLUDE_DIR MATCHES PFVAR_INCLUDE_DIR-NOTFOUND)
    list(APPEND NANNY_SRC pf_mock.c)
    set_source_files_properties(pf_mock.c PROPERTIES COMPILE_FLAGS -Wno-unused-parameter)
else(PFVAR_INCLUDE_DIR MATCHES PFVAR_INCLUDE_DIR-NOTFOUND)
    include_directories(${PFVAR_INCLUDE_DIR})
    list(APPEND NANNY_SRC pf_real.c)
endif(PFVAR_INCLUDE_DIR MATCHES PFVAR_INCLUDE_DIR-NOTFOUND)

set(EXECUTABLE_NAME "nanny")
set(LIB_NAME "nanny_lib")

add_library(${LIB_NAME} STATIC ${NANNY_SRC} ${JWT_SRC} ${BCRYPT_SRC} ${SCLOG4C_SRC})
target_link_libraries(${LIB_NAME} ${CMAKE_DL_LIBS} ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(${LIB_NAME} ${JANSSON_LIBRARIES} ${OPENSSL_CRYPTO_LIBRARY})
target_link_libraries(${LIB_NAME} ${JWT_LIBRARIES})


add_executable(${EXECUTABLE_NAME} ${NANNY_MAIN_SRC} )
target_link_libraries(${EXECUTABLE_NAME} ${LIB_NAME})
install(TARGETS ${EXECUTABLE_NAME} RUNTIME DESTINATION bin)


