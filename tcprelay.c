/* Simple Linux epoll-based single thread TCP relay. Public domain. Written by Vitaly "_Vi" Shukela.*/

/* We are not using EPOLLET, but use EPOLLONESHOT.
 * We store we_should_epoll_for_{reads,writes} and EPOLL_CTL_MODding almost every time we do something.
 * http://stackoverflow.com/questions/4003804/how-to-read-multiple-file-descriptors-using-epoll-select-with-epollet
 *
 * Out-of-band data is not processed.
 * Half-shutdown connections are expected to work properly.
 *
 * It also supports retrieving destination address with SO_ORIGINAL_DST
 */

#include <stdio.h>
#include <sys/epoll.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>

#define MAXFD 1024		/* Not checked for overflow anywhere */
static char buf[65536];
#define MAX_EPOLL_EVENTS_AT_ONCE 1024 /* even 1 should work more-or-less fine */

#define dpf(...) //fprintf (stderr, __VA_ARGS__)

struct {
    int peerfd; // where we connected by client's request (and vice versa). You'll see fdinfo[fd].peerfd frequently in the code.
    char writeready;  // epoll said that we can write here
    char readready;   // epoll said that we can read from here
    char we_should_epoll_for_reads;
    char we_should_epoll_for_writes;
    char status; 
    /*
       States:
       | - bidirectional connected peer
       s - we can only send (half-shutdown)
       r - we can only recv (half-shutdown)
       . - closed
     */   
    char group;
    /*
	Groups:
	c - incoming connection
	d - outgoing connection
     */
    struct sockaddr_in address;
    char* buff; // "inbox", allocated only in case of a short write to this socket.
    int debt; // length of buff.
    long long total_read;
} static fdinfo[MAXFD] = { [0 ... MAXFD - 1] = {0, 0, 0}};


static int kdpfd; /* epoll fd */
static int ss; /* server socket */
    
const char *bind_ip;
int bind_port;
const char *connect_ip;
int connect_port;

int need_address_redirection;
int need_port_redirection;


static void parse_argv(int argc, char* argv[]); 
static void process_read(int fd);  // we are both able to read from fd and write to fdinfo[fd].peerfd
static void process_debt(int fd);  // previous process_read to peer fd had problems (short/missing write to this fd). Now we can finish it.
static void process_accept(int ss); // new client connects. Need to connect to the peer, set up epoll, setup fdinfo.
static void listen_socket_and_setup_epoll(); // setup main socket to listen (and add it to epoll)
static void close_fd(int fd); // close both fd and peer fd. Clean up debt buffers and fdinfo states.
static void epoll_update(int fd); // call epoll_ctl for this fd accroding to we_should_epoll_for_* fields.
static void process_stdin();
#include "process_read.c"
#include "process_debt.c"
#include "process_accept.c"
#include "parse_argv.c"
#include "listen_socket_and_setup_epoll.c"
#include "close_fd.c"
#include "epoll_update.c"
#include "process_stdin.c"


int main(int argc, char *argv[])
{
    parse_argv(argc, argv);
    
    listen_socket_and_setup_epoll();

    struct epoll_event events[MAX_EPOLL_EVENTS_AT_ONCE];
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

                if (fdinfo[fd].status=='.') {
		    continue; /* happens when fails to connect */
		}

		if (ev & EPOLLOUT) {
		    dpf("%d becomes ready for writing\n", fd);
		    fdinfo[fd].writeready = 1;
		    fdinfo[fd].we_should_epoll_for_writes=0; /* as it is one shot event */
                    epoll_update(fd);

		    fdinfo[fdinfo[fd].peerfd].we_should_epoll_for_reads = 1;
                    epoll_update(fdinfo[fd].peerfd);
		}           
		if (ev & EPOLLIN) {
		    dpf("%d becomes ready for reading\n", fd);
		    fdinfo[fd].readready = 1;
		    fdinfo[fd].we_should_epoll_for_reads=0; /* as it is one shot event */
		    epoll_update(fd);
		}
		if (ev & (EPOLLERR|EPOLLHUP) ) {
		    dpf("%d HUP or ERR\n", fd);
		    dpf("    %d and %d are to be closed\n", fd, fdinfo[fd].peerfd);
		    fdinfo[fd].status='.';
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

		if (fdinfo[fd].status=='.') {
		    close_fd(fd);
		}
	    }
	} 
    }

    fprintf(stderr, "Probably abnormal termination\n");
}
