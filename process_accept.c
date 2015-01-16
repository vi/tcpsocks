#include "main.h"

void process_accept(int ss) {
    /* Accepting the client socket */
    struct sockaddr_in sa, da, ca; 
    struct epoll_event ev;
    memset(&ev, 0, sizeof ev);
    socklen_t len = sizeof sa;
    int client = accept(ss, (struct sockaddr *) &sa, &len);
    if (client < 0) {
	perror("accept");
	return;
    }
    
    if(fcntl(client, F_SETFL, O_NONBLOCK)==-1) {
        // Reject the client instead of possibly blocking the entire tcpsocks
        close(client);
        return;
    }

    memset(&da, 0, sizeof da);
#ifndef SO_ORIGINAL_DST
#define SO_ORIGINAL_DST 80
#endif
    if (need_address_redirection || need_port_redirection) {
	len = sizeof da;
	if (getsockopt
	    (client, SOL_IP, SO_ORIGINAL_DST,
	     (struct sockaddr *) &da, &len) != 0) {
	    write(client, "Unable to get destination address\n", 34);
	    close(client);
	    printf("%s:%d -> ???\n", inet_ntoa(sa.sin_addr),
		   ntohs(sa.sin_port));
	    return;
	}
    }

    da.sin_family = PF_INET;
    if (!need_port_redirection) {
	da.sin_port = htons(connect_port);
    }
    if (!need_address_redirection) {
	inet_aton(connect_ip, &da.sin_addr);
    }
    
    memset(&ca, 0, sizeof ca);
    ca.sin_family = PF_INET;
    ca.sin_port = htons(socks_port);
    inet_aton(socks_ip, &ca.sin_addr);


    /* Now start connecting to the SOCKS5 server */

    int destsocket = socket(PF_INET, SOCK_STREAM, 0);
    if (fcntl(destsocket, F_SETFL, O_NONBLOCK) == -1) { close(destsocket); destsocket = -1; }
    if (destsocket == -1) {
	const char* msg = "Cannot create a socket to destination address.\n";

	write(client, msg, strlen(msg));
	close(client);
	fprintf(stderr, "%s", msg);
	return;
    }
    if (-1 == connect(destsocket, (struct sockaddr *) &ca, sizeof ca)) {
	if (errno != EWOULDBLOCK && errno != EINPROGRESS) {
	    fprintf(stderr, "Cannot connect a socket to destination address\n");
	    write(client, "Cannot connect a socket to destination address\n", 58-11);
	    close(client);
	    close(destsocket);
	    return;
	}
    }

    /* Set up events and pipes and fdinfo*/
    
    ev.events = EPOLLOUT  | EPOLLONESHOT;
    ev.data.fd = destsocket;
    if (epoll_ctl(kdpfd, EPOLL_CTL_ADD, destsocket, &ev) < 0) {
	fprintf(stderr, "epoll peer set insertion error\n");
	write(client, "epoll peer set insertion error\n", 31);
	close(client);
	close(destsocket);
	return;
    }
		
    /* But not yet adding client socket to epoll */
    
    fdinfo[client].peerfd = destsocket;
    fdinfo[client].status='C';
    fdinfo[client].address=sa;
    fdinfo[client].writeready=0;
    fdinfo[client].readready=0;
    fdinfo[client].buff=NULL;
    fdinfo[client].debt=0;
    fdinfo[client].we_should_epoll_for_writes=0;  /* we've already set up epoll above */
    fdinfo[client].we_should_epoll_for_reads=0; /* first we get writing ready, only then start reading peer */
    fdinfo[client].group='c';
    fdinfo[client].total_read = 0;
    fdinfo[destsocket].peerfd = client;
    fdinfo[destsocket].status='1';
    fdinfo[destsocket].address=da;
    fdinfo[destsocket].writeready=0;
    fdinfo[destsocket].readready=0;
    fdinfo[destsocket].buff=NULL;
    fdinfo[destsocket].debt=0;
    fdinfo[destsocket].we_should_epoll_for_writes=1; /* we've already set up epoll above */
    fdinfo[destsocket].we_should_epoll_for_reads=0;  /* first we get writing ready, only then start reading peer */ 
    fdinfo[destsocket].group='d';
    fdinfo[destsocket].total_read = 0;
    
    print_connection(client, "", "\n");
}
