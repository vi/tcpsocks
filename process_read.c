#include "main.h"

void process_read(int fd) {
    dpf("Selecting %d for reading. Peer is %d.\n", fd, fdinfo[fd].peerfd);
    char buf[BUFSIZE];
    ssize_t ret, ret2;
recv_was_interrupted:
    ret = recv(fd, buf, sizeof buf, 0);
    dpf("    got %d bytes\n", ret);
    if (ret == 0) {
	/* EOF */
	/* Do not bother about "debts" because of epoll handler is set only if there are no debts */
	switch(fdinfo[fd].status) {
	    case '|': 
		dpf("    %d becomes send-only. %d becomes receive-only\n", fd, fdinfo[fd].peerfd);
		fdinfo[fd].status = 's'; 
		fdinfo[fdinfo[fd].peerfd].status = 'r';
		shutdown(fd, SHUT_RD);
		shutdown(fdinfo[fd].peerfd, SHUT_WR);
		break;
	    case 'r':
		dpf("    %d and %d are to be closed\n", fd, fdinfo[fd].peerfd);
		close_fd(fd); // Close if cannot both send and recv
		break; 
	}
	return;
    } else
    if (ret < 0) {
	if(errno==EINTR) goto recv_was_interrupted;
	if(errno==EAGAIN) {
	    dpf(    "%d is not ready for reading\n", fd);
	    fdinfo[fd].readready = 0;			    
	    fdinfo[fd].we_should_epoll_for_reads=1;
	    epoll_update(fd);
	} else {
	    fprintf(stderr, "Why epoll hasn't told us that there is some error?\n");
	    perror("recv");
	    close_fd(fd);
	    return;
	}
    } else {
	fdinfo[fd].total_read += ret;
send_was_interrupted:
	ret2 = send(fdinfo[fd].peerfd, buf, ret, 0);
	dpf("    sent %d bytes\n", ret2);
	if (ret2 == 0) {
	    fprintf(stderr, "send returned 0? Submit to codinghorror?\n");
	    close_fd(fd);
	    return;
	}
	if (ret2 < 0) {
	    if(errno==EINTR) goto send_was_interrupted;
	    if(errno==EAGAIN) {
		dpf("    %d is not ready for writing anymore [peer]\n", fdinfo[fd].peerfd);
		fdinfo[fdinfo[fd].peerfd].writeready=0;
		ret2 = 0; // Handled by ret2 < ret case
	    } else {
		fprintf(stderr, "Why epoll hasn't told us that there would be error at sending?\n");
		perror("send");
		close_fd(fd);
		return;
	    }
	}
	if (ret2 < ret) {
	    /* Short write or EINTR at send. 
	     * Need to remember the rest of the buffer somewhere before continuing. */
	    dpf("    short write or EAGAIN at %d\n", fdinfo[fd].peerfd);
	    int l = ret - ret2;
	    char* b = (char*)malloc(l);
	    if (b == NULL) {
		perror("malloc");
		close_fd(fd);
		return;
	    }
	    memcpy(b, buf + ret2, l);
	    fdinfo[fdinfo[fd].peerfd].buff=b;
	    fdinfo[fdinfo[fd].peerfd].debt=l;
	    dpf("    stored debt %d for %d\n", l, fdinfo[fd].peerfd);
	    fdinfo[fdinfo[fd].peerfd].we_should_epoll_for_writes=1;
	    epoll_update(fdinfo[fd].peerfd);
	} else {
	    /* we have copied all the data successfully. Can try one more cycle */
	    fdinfo[fd].we_should_epoll_for_reads=1;
	    epoll_update(fd);
	}
    }
}
