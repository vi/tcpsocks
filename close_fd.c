#include "main.h"

void close_fd(int fd) {
    dpf("Closing requested for %d and %d\n", fd, fdinfo[fd].peerfd);
    if(fdinfo[fd].status==0 || fdinfo[fd].peerfd==0) {
	fprintf(stderr, "%d is not valid fd to close\n", fd);
	return;
    }
    
    if (fdinfo[fd].group=='c') {
	/* Exchange fd and peerfd so that fd is client and peerfd is SOCKS */
	fd = fdinfo[fd].peerfd;
    }
    int peerfd = fdinfo[fd].peerfd;

    print_connection(fd, "    ", " Finished\n");

    fdinfo[fd].status='.';
    fdinfo[peerfd].status='.';
    close(fd);
    close(peerfd);
    if (fdinfo[fd].buff) {
	free(fdinfo[fd].buff);
	fdinfo[fd].buff=NULL;
	fdinfo[fd].debt=0;
    }
    if (fdinfo[peerfd].buff) {
	free(fdinfo[peerfd].buff);
	fdinfo[peerfd].buff=NULL;
	fdinfo[peerfd].debt=0;
    }
    fdinfo[fd].group=0;
    fdinfo[peerfd].group=0;
 
}
