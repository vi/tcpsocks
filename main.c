/* Simple Linux epoll-based single thread SOCKS5 client. LGPL. Written by Vitaly "_Vi" Shukela.*/

/* We are not using EPOLLET, but use EPOLLONESHOT.
 * We store we_should_epoll_for_{reads,writes} and EPOLL_CTL_MODding almost every time we do something.
 * http://stackoverflow.com/questions/4003804/how-to-read-multiple-file-descriptors-using-epoll-select-with-epollet
 *
 * Out-of-band data is not processed.
 * Half-shutdown connections are expected to work properly.
 *
 * It also supports retrieving destination address with SO_ORIGINAL_DST
 */

#include "main.h"
int debug_output;

struct fdinfo_t fdinfo[MAXFD] = { [0 ... MAXFD - 1] = {0, 0, 0}};


int kdpfd; /* epoll fd */
int ss; /* server socket */
    
const char *bind_ip;
int bind_port;
const char *connect_ip;
int connect_port;
const char* socks_ip;
int socks_port;
const char* socks_user;
const char* socks_password;
int need_password;

int need_address_redirection;
int need_port_redirection;

void sigpipe() {
   fprintf(stderr, "SIGPIPE\n");
}

int main(int argc, char *argv[])
{
    parse_argv(argc, argv);
    
    listen_socket_and_setup_epoll();

    struct epoll_event events[MAX_EPOLL_EVENTS_AT_ONCE];

    {
        struct sigaction sa;
        memset(&sa, 0, sizeof sa);
        sa.sa_handler = sigpipe;
        sa.sa_flags = 0;
        sigaction(SIGPIPE, &sa, NULL);
    }

    /* Main event loop */
    for (;;) {
	int nfds = epoll_wait(kdpfd, events, MAX_EPOLL_EVENTS_AT_ONCE, -1);

	if (nfds == -1) {
	    if (errno == EAGAIN || errno == EINTR) {
		continue;
	    }
	    perror("epoll_pwait");
	    exit(EXIT_FAILURE);
	}

	int n;
	for (n = 0; n < nfds; ++n) {
	    if (events[n].data.fd == ss) {

		process_accept(ss);

	    } else if (events[n].data.fd == 0) {
		
                process_stdin();

	    } else { /* Handling the sends and recvs here */

		int fd = events[n].data.fd;
		int ev = events[n].events;

		if(fd < 0 || fd >= MAXFD) {
		    fprintf(stderr, "BAD FD %d\n", fd);
		    continue;
		}

                if (fdinfo[fd].status=='.') {
		    continue; /* happens when fails to connect */
		}

		if (ev & EPOLLOUT) {
		    dpf("%d becomes ready for writing\n", fd);
		    fdinfo[fd].writeready = 1;
		    fdinfo[fd].we_should_epoll_for_writes=0; /* as it is one shot event */
                    epoll_update(fd);

		    if (fdinfo[fd].status == '|' || fdinfo[fd].status == 's') {
			fdinfo[fdinfo[fd].peerfd].we_should_epoll_for_reads = 1;
			epoll_update(fdinfo[fd].peerfd);
		    }
		}           
		if (ev & EPOLLIN) {
		    dpf("%d becomes ready for reading\n", fd);
		    fdinfo[fd].readready = 1;
		    fdinfo[fd].we_should_epoll_for_reads=0; /* as it is one shot event */
		    epoll_update(fd);
		}
		if (ev & (EPOLLERR|EPOLLHUP) ) {
		    dpf("%d HUP or ERR\n", fd);
		    if ((fdinfo[fd].status>='1' && fdinfo[fd].status<='3') || fdinfo[fd].status=='A') {
			/* This is connection to a SOCKS5 server. Call process_socks to send error message to client. */
			if (fdinfo[fd].status=='1') process_socks_phase_1(fd);
			if (fdinfo[fd].status=='2') process_socks_phase_2(fd);
			if (fdinfo[fd].status=='A') process_socks_phase_A(fd);
			if (fdinfo[fd].status=='3') process_socks_phase_3(fd);
		    } else {
			dpf("    %d and %d are to be closed\n", fd, fdinfo[fd].peerfd);
			close_fd(fd);
		    }
		}

		if(fdinfo[fd].readready && 
			fdinfo[fdinfo[fd].peerfd].writeready && 
			fdinfo[fdinfo[fd].peerfd].debt==0 && 
			(fdinfo[fd].status=='|' || fdinfo[fd].status=='r') ) {

		    process_read(fd);

		}
		
		if(fdinfo[fd].writeready && 
			fdinfo[fd].debt>0 && 
			fdinfo[fd].buff!=NULL && 
			(fdinfo[fd].status=='|' || fdinfo[fd].status=='s') ) {

		    process_debt(fd);
		}
		
		if(fdinfo[fd].writeready && 
			(fdinfo[fd].status=='1') ) {

		    process_socks_phase_1(fd);
		}
		
		if(fdinfo[fd].readready && 
			(fdinfo[fd].status=='2') ) {

		    process_socks_phase_2(fd);
		}
		
		if(fdinfo[fd].readready && 
			(fdinfo[fd].status=='A') ) {

		    process_socks_phase_A(fd);
		}
		
		if(fdinfo[fd].readready && 
			(fdinfo[fd].status=='3') ) {

		    process_socks_phase_3(fd);
		}
	    }
	} 
    }

    fprintf(stderr, "Probably abnormal termination\n");
}
