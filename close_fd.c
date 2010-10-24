static void close_fd(int fd) {
    dpf("Closing requested for %d and %d\n", fd, fdinfo[fd].peerfd);
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
