project(nanny)

add_executable(nanny main.c)

install(TARGETS nanny RUNTIME DESTINATION bin)
