
static void process_socks_phase_1(int fd) {
    if (!need_password) {
	char socks_connect_request[3 + 4 + 4 + 2];	/* Noauth method offer + connect command + IP address + port */
	memcpy(socks_connect_request,  "\x5\x1\x0"       "\x5\x1\x0\x1"    "XXXX"       "XX" , 3+4+4+2);
	/*                                      |              |     |         |          |
					      no auth        connect |        IP address  |
								    IPv4                 port	*/
	memcpy(socks_connect_request+3+4   , &fdinfo[fd].address.sin_addr,4);
	memcpy(socks_connect_request+3+4+4 , &fdinfo[fd].address.sin_port,2);

	int ret = send(fd, socks_connect_request, 3+4+4+2, 0);
	if (ret == 3+4+4+2) {
	    fdinfo[fd].status='2';
	    fdinfo[fd].we_should_epoll_for_reads=1;
	    epoll_update(fd);
	} else {
	    const char* msg;
	    if(ret==-1) {
		msg = "Cannot connect to SOCKS5 server.\n";
	    } else {
		msg = "Short write to SOCKS5 server.\n";
	    }

	    send(fd, msg, strlen(msg),0);
	    fprintf(stderr, "%s", msg);
	    close_fd(fd);
	}
    }
}

static void process_socks_phase_2(int fd) {
    char buf[2];
    const char* msg = NULL;
    int nn = read(fd, buf, 2);
    //fprintf(stderr, "SOCKS5 phase 2 reply: %02x%02x\n", buf[0], buf[1]);
    if(nn!=2) {
	msg = "Not exactly 2 bytes is received from SOCKS5 server. This situation is not handeled.\n";
    }
    if(buf[0]!=5 || (buf[1]!=0 && buf[1]!=255)) {
	msg = "Not SOCKS5 reply from SOCKS5 server\n";
    }
    if(buf[1]==255) {
	msg = "Authentication required on SOCKS5 server\n";
    }
    if(msg) {
	send(fd, msg, strlen(msg),0);
	fprintf(stderr, "%s", msg);
	close_fd(fd);
    } else {
	fdinfo[fd].status='3';
	fdinfo[fd].we_should_epoll_for_reads=1;
	epoll_update(fd);
    }

}

static void process_socks_phase_3(int fd) {
    char buf[10];
    int nn = read(fd, buf, 10);
    const char* msg = NULL;
    //fprintf(stderr, "SOCKS5 phase 3 reply: %02x%02x%02x%02x...\n", buf[0], buf[1], buf[2], buf[3]);
    if(nn<10) {
	msg = "Not exactly 10 is received from SOCKS5 server. This situation is not handeled.\n";
    }
    if(buf[0]!=5) {
	msg = "Not SOCKS5 reply from SOCKS5 server [phase 3]\n";
    }
    if(buf[1]!=0) {
	switch(buf[1]) {
	    case 1: msg = "general SOCKS server failure\n"; break;
	    case 2: msg =  "connection not allowed by ruleset\n"; break; 
	    case 3: msg =  "Network unreachable\n"; break; 
	    case 4: msg =  "Host unreachable\n"; break; 
	    case 5: msg =  "Connection refused\n"; break; 
	    case 6: msg =  "TTL expired\n"; break; 
	    case 7: msg =  "Command not supported\n"; break; 
	    case 8: msg =  "Address type not supported\n"; break; 
	    default: msg = "Unknown error at SOCKS5 server\n"; break; 
	}
    }
    if(buf[3]!=1) {
	msg = "Not an IPv4 address in SOCKS5 reply\n";
    }
    
    if(msg) {
	send(fd, msg, strlen(msg),0);
	fprintf(stderr, "%s", msg);
	close_fd(fd);
	return;
    } 

    /* SOCKS5 connection successed. Setting up piping. */
    int peerfd = fdinfo[fd].peerfd;
    fdinfo[fd].status='|';
    fdinfo[peerfd].status='|';
    
    fdinfo[fd].we_should_epoll_for_reads = 1;
    epoll_update(fd);
    fdinfo[peerfd].we_should_epoll_for_reads = 1;
    epoll_update(peerfd);
}
