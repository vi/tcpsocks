static void close_fd(int fd) {
    dpf("Closing requested for %d and %d\n", fd, fdinfo[fd].peerfd);
    if(fdinfo[fd].status==0 || fdinfo[fd].peerfd==0) {
	fprintf(stderr, "%d is not valid fd to close\n", fd);
	return;
    }
    fdinfo[fd].status='.';
    fdinfo[fdinfo[fd].peerfd].status='.';
    close(fd);
    close(fdinfo[fd].peerfd);
    if (fdinfo[fd].buff) {
	free(fdinfo[fd].buff);
	fdinfo[fd].buff=NULL;
	fdinfo[fd].debt=0;
    }
    if (fdinfo[fdinfo[fd].peerfd].buff) {
	free(fdinfo[fdinfo[fd].peerfd].buff);
	fdinfo[fdinfo[fd].peerfd].buff=NULL;
	fdinfo[fdinfo[fd].peerfd].debt=0;
    }
    fdinfo[fd].group=0;
    fdinfo[fdinfo[fd].peerfd].group=0;
}
