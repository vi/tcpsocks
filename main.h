#ifndef MAIN_H
#define MAIN_H

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
#include <stdarg.h>
#include <signal.h>

#define MAXFD 1024		/* Not checked for overflow anywhere */
#define BUFSIZE 65536
#define MAX_EPOLL_EVENTS_AT_ONCE 1024 /* even 1 should work more-or-less fine */

extern int debug_output;

static inline void dpf(const char *fmt, ...) {
    if (!debug_output) {
	return;
    }

    va_list argp;
    va_start(argp, fmt);
    vfprintf(stderr, fmt, argp);
    va_end(argp);
}


extern struct fdinfo_t {
    int peerfd; // where we connected by client's request (and vice versa). You'll see fdinfo[fd].peerfd frequently in the code.
    char writeready;  // epoll said that we can write here
    char readready;   // epoll said that we can read from here
    char we_should_epoll_for_reads;
    char we_should_epoll_for_writes;
    char status; 
    /*
       States:
       C - connected client

       1 - pending connection to SOCKS5 server
       2 - sent response
       A - need authentication step
       3 - got auth method choice response

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
} fdinfo[MAXFD];


int kdpfd; /* epoll fd */
int ss; /* server socket */
    
extern const char *bind_ip;
extern int bind_port;
extern const char *connect_ip;
extern int connect_port;
extern const char* socks_ip;
extern int socks_port;
extern const char* socks_user;
extern const char* socks_password;
extern int need_password;

extern int need_address_redirection;
extern int need_port_redirection;

void parse_argv(int argc, char* argv[]); 
void process_read(int fd);  // we are both able to read from fd and write to fdinfo[fd].peerfd
void process_debt(int fd);  // previous process_read to peer fd had problems (short/missing write to this fd). Now we can finish it.
void process_accept(int ss); // new client connects. Need to connect to the peer, set up epoll, setup fdinfo.
void listen_socket_and_setup_epoll(); // setup main socket to listen (and add it to epoll)
void close_fd(int fd); // close both fd and peer fd. Clean up debt buffers and fdinfo states.
void epoll_update(int fd); // call epoll_ctl for this fd accroding to we_should_epoll_for_* fields.
void print_connection(int fd, const char* prologue, const char* epilogue);
void process_stdin();
void process_socks_phase_1(int fd);
void process_socks_phase_2(int fd);
void process_socks_phase_A(int fd);
void process_socks_phase_3(int fd);

#endif // MAIN_H
