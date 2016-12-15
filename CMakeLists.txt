cmake_minimum_required(VERSION 3.0)

file(WRITE ${CMAKE_BINARY_DIR}/VERSION "#define VERSION \"v1.4?\"\n")

include_directories(${CMAKE_BINARY_DIR})

add_executable(tcpsocks
    process_debt.c
    epoll_update.c
    close_fd.c
    process_read.c
    listen_socket_and_setup_epoll.c
    main.c
    main.h
    parse_argv.c
    process_socks.c
    process_stdin.c
    process_accept.c
)

install(TARGETS tcpsocks RUNTIME DESTINATION bin)
