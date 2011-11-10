#include "main.h"

/* TODO: too many common code with process_read */

void process_debt(int fd) {
    /* After a short write, we finally can deliver the "debt" */
    ssize_t ret;
    dpf("Selecting %d for writing because of debt %d\n", fd, fdinfo[fd].debt);
send_was_interrupted2:
    ret = send(fd, fdinfo[fd].buff, fdinfo[fd].debt, 0);
    dpf("    sent %d bytes\n", ret);
    if (ret == 0) {
	fprintf(stderr, "send returned 0? Submit to codinghorror?\n");
	close_fd(fd);
	return;
    } else
    if (ret < 0) {
	if(errno==EINTR) goto send_was_interrupted2;
	if(errno==EAGAIN) {
	    fdinfo[fd].writeready=0;			    
	    fdinfo[fd].we_should_epoll_for_writes=1;
	    epoll_update(fd);
	} else {
	    fprintf(stderr, "Why epoll hasn't told us that there would be error at sending?\n");
	    perror("send");
	    close_fd(fd);
	    return;
	}
    } else {
	if (ret < fdinfo[fd].debt) {
	    dpf("    short write at %d\n", fd);
	    /* _Again_ short write. Need to remember the rest of the buffer somewhere. */
	    int l = fdinfo[fd].debt - ret;
	    char* b = (char*)malloc(l);
	    if (b == NULL) {
		perror("malloc");
		close_fd(fd);
		return;
	    }
	    memcpy(b, fdinfo[fd].buff + ret, l);
	    free(fdinfo[fd].buff);
	    fdinfo[fd].buff = b;
	    fdinfo[fd].debt = l;
        
	    fdinfo[fd].we_should_epoll_for_writes=1;
	    epoll_update(fd);

	    dpf("    re-stored debt %d for %d\n", l, fd);
	} else {
	    /* Now the debt is fulfilled */
	    free(fdinfo[fd].buff);
	    fdinfo[fd].buff = NULL;
	    fdinfo[fd].debt = 0;
	    dpf("    debt is fulfilled for %d\n", fd);
	    /* and we can start a new cycle */
	    fdinfo[fdinfo[fd].peerfd].we_should_epoll_for_reads = 1;
	    epoll_update(fdinfo[fd].peerfd);
	}
    }
}
