#include "main.h"

/* These are debug messages */
const char* __attribute__((unused)) epoll_update_msgs[] = {
    "    setting up %d to listen only for error events\n",
    "    setting up %d to listen for read events\n",
    "    setting up %d to listen for write events\n",
    "    setting up %d to listen for read and write events\n"};


void epoll_update(int fd) {
    struct epoll_event ev;
    memset(&ev, 0, sizeof ev);
    ev.events = EPOLLONESHOT;

    if (fdinfo[fd].status=='.' || fdinfo[fd].status==0) {
	fprintf(stderr, "Bad epoll_update on fd %d with status '%c'\n", fd, fdinfo[fd].status);
	return;
    }
    
    dpf(epoll_update_msgs[fdinfo[fd].we_should_epoll_for_reads + 2*fdinfo[fd].we_should_epoll_for_writes], fd);

    if (fdinfo[fd].we_should_epoll_for_reads) {
	ev.events |= EPOLLIN;
    }
    if (fdinfo[fd].we_should_epoll_for_writes) {
	ev.events |= EPOLLOUT;
    }
    ev.data.fd = fd;
    int ret = epoll_ctl(kdpfd, EPOLL_CTL_MOD, fd, &ev);
    if(ret<0) {
	fprintf(stderr, "epoll_ctl MOD error fd=%d errno=%d\n", fd, errno);
    }
}
