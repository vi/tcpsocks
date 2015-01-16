#include "main.h"

void listen_socket_and_setup_epoll() {
    /* Open the server side socket */
    ss = socket(PF_INET, SOCK_STREAM, 0);
    if (fcntl(ss, F_SETFL, O_NONBLOCK) == -1) { close(ss); ss = -1; }
    if (ss == -1) {
	perror("socket");
	exit(1);
    }

    int opt = 1;
    setsockopt(ss, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);

    struct sockaddr_in sa;
    memset(&sa, 0, sizeof sa);
    sa.sin_family = PF_INET;
    sa.sin_port = htons(bind_port);
    inet_aton(bind_ip, &sa.sin_addr);
    if (-1 == bind(ss, (struct sockaddr *) &sa, sizeof sa)) {
	close(ss);
	perror("bind");
	exit(1);
    }
    if (-1 == listen(ss, 0)) {
	close(ss);
	perror("listen");
	exit(1);
    }
    fcntl(0, F_SETFL, O_NONBLOCK);

    /* epoll setup */
    struct epoll_event ev;
    memset(&ev, 0, sizeof ev);
    kdpfd = epoll_create(4);
    if (kdpfd == -1) {
	perror("epoll_create");
	exit(EXIT_FAILURE);
    }
    ev.events = EPOLLIN;
    ev.data.fd = ss;
    if (epoll_ctl(kdpfd, EPOLL_CTL_ADD, ss, &ev) == -1) {
	perror("epoll_ctl: listen_sock");
	exit(EXIT_FAILURE);
    }

    ev.events = EPOLLIN;
    ev.data.fd = 0;
    if (epoll_ctl(kdpfd, EPOLL_CTL_ADD, 0, &ev) == -1) {
	perror("epoll_ctl: stdin");
    }
}
