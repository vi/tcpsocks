#include "main.h"

void process_socks_phase_1(int fd) {
    dpf("process_socks_phase_1\n");
    char socks_connect_request[1024]; // To be sure it can handly anything we need
    int len;
    if (!need_password) {
	memcpy(socks_connect_request,  "\x5\x1\x0"       "\x5\x1\x0\x1"    "XXXX"       "XX" , 3+4+4+2);
	/*                                      |              |     |         |          |
					      no auth        connect |        IP address  |
								    IPv4                 port	*/
	memcpy(socks_connect_request+3+4   , &fdinfo[fd].address.sin_addr,4);
	memcpy(socks_connect_request+3+4+4 , &fdinfo[fd].address.sin_port,2);
	len = 3+4+4+2;
    } else {
	unsigned char username_len, password_len;
	memcpy(socks_connect_request,  "\x5\x1\x2"                      "\x1X", 3+2);
	/*                                      |                           |
	 *                                   username/password auth        username_length
	 *                                   */   
	username_len =  strlen(socks_user);
	len = 3;
	memcpy(socks_connect_request+len+1,                     &username_len, 1);
	memcpy(socks_connect_request+len+2,        socks_user, username_len);
	len += 1 + 1 + username_len;

	password_len   =  strlen(socks_password);
	memcpy(socks_connect_request+len,          &password_len, 1);
	memcpy(socks_connect_request+len+1,        socks_password, password_len);
	len += 1 + password_len;
	
	memcpy(socks_connect_request+len,   "\x5\x1\x0\x1", 4);
	memcpy(socks_connect_request+len+4,  &fdinfo[fd].address.sin_addr, 4);
	memcpy(socks_connect_request+len+4+4,  &fdinfo[fd].address.sin_port, 2);
	len += 4+4+2;
    }

    int ret;
    const char* msg = NULL;
send_again:
    ret = send(fd, socks_connect_request, len, 0);
    if (ret == -1) {
	if(errno==EINTR) goto send_again;
	if(errno==EAGAIN) {
	    dpf("    eagain\n");
	    fdinfo[fd].we_should_epoll_for_writes=1;
	    epoll_update(fd);
	    return;
	}
	msg = "tcpsocks: Cannot connect to SOCKS5 server\n";
    } else
    if (ret != len) {
	msg = "tcpsocks: Short write to SOCKS5 server.\n";
	dpf("    send returned %d\n", ret);
    }

    if (msg) {
	send(fdinfo[fd].peerfd, msg, strlen(msg),0);
	fprintf(stdout, "%s", msg);
	close_fd(fd);
	return;
    }

    fdinfo[fd].status='2';

    fdinfo[fd].we_should_epoll_for_reads=1;
    epoll_update(fd);
}

void process_socks_phase_2(int fd) {
    dpf("process_socks_phase_2\n");
    char buf[2];
    memset(buf, 0, sizeof buf);
    const char* msg = NULL;
    int nn;
read_again:
    nn = read(fd, buf, 2);
    dpf("    SOCKS5 phase 2 reply: %02x%02x\n", buf[0], buf[1]);
    if(nn==-1) {
	if(errno==EINTR) goto read_again;
	if(errno==EAGAIN) {
	    dpf("    eagain\n");
	    fdinfo[fd].we_should_epoll_for_reads=1;
	    epoll_update(fd);
	    return;
	}
	perror("read");
	msg = "tcpsocks: Read failure from SOCKS5 server\n";
    } else 
    if(nn==0) {
	msg = "tcpsocks: End-of-file from SOCKS5 server [phase 2]\n";
    }
    if(nn!=2) {
	msg = "tcpsocks: Not exactly 2 bytes is received from SOCKS5 server. This situation is not handled.\n";
    } else
    if(buf[0]!=5) {
	msg = "tcpsocks: Not SOCKS5 reply from SOCKS5 server\n";
    } else
    if (buf[1]!=0 && buf[1]!=255 && buf[1]!=2) {
	msg = "tcpsocks: Unknown authentication method in SOCKS5 reply\n";
    } else
    if (need_password) {
	if (buf[1]!=2) {
	    if (buf[1]==0) {
		msg = "tcpsocks: Server refused to accept our authentication mode and offsers just anonymous access\n";
	    } else {
		msg = "tcpsocks: Server refused to accept our authentication mode\n";
	    }
	}
	fdinfo[fd].status='A';
    } else {
	if(buf[1]!=0) {
	    msg = "tcpsocs: Authentication required on SOCKS5 server\n";
	} 
	fdinfo[fd].status='3';
    }
    if(msg) {
	send(fdinfo[fd].peerfd, msg, strlen(msg),0);
	fprintf(stdout, "%s", msg);
	close_fd(fd);
    } else {
	fdinfo[fd].we_should_epoll_for_reads=1;
	epoll_update(fd);
    }

}

void process_socks_phase_A(int fd) {
    dpf("process_socks_phase_A\n");
    char buf[2];
    memset(buf, 0, sizeof buf);
    const char* msg = NULL;
    int nn;
read_again:
    nn = read(fd, buf, 2);
    dpf("    SOCKS5 phase A reply: %02x%02x\n", buf[0], buf[1]);
    if(nn==-1) {
	if(errno==EINTR) goto read_again;
	if(errno==EAGAIN) {
	    dpf("    eagain\n");
	    fdinfo[fd].we_should_epoll_for_reads=1;
	    epoll_update(fd);
	    return;
	}
	perror("read");
	msg = "tcpsocks: Read failure from SOCKS5 server\n";
    } else
    if (nn==0) {
        msg = "tcpsocks: End-of-file from SOCKS5 server [phase A]\n";
    } else
    if(nn!=2) {
	msg = "tcpsocks: Not exactly 2 bytes is received from SOCKS5 server. This situation is not handled.\n";
    } else
    if (buf[0]!=1) {
	msg = "tcpsocks: Invalid auth method in reply\n";
    } else
    if (buf[1]!=0) {
	msg = "tcpsocks: Authentication failed on SOCKS5 server\n";
    } 
    if(msg) {
	send(fdinfo[fd].peerfd, msg, strlen(msg),0);
	fprintf(stdout, "%s", msg);
	close_fd(fd);
    } else {
	fdinfo[fd].status='3';
	fdinfo[fd].we_should_epoll_for_reads=1;
	epoll_update(fd);
    }

}

void process_socks_phase_3(int fd) {
    dpf("process_socks_phase_3\n");
    char buf[10];
    memset(buf, 0, sizeof buf);
    int nn;
recv_again:
    nn = read(fd, buf, 10);
    const char* msg = NULL;
    dpf("    SOCKS5 phase 3 reply: nn=%d %02x%02x%02x%02x...\n", nn, buf[0], buf[1], buf[2], buf[3]);
    if(nn==-1) {
	if(errno==EINTR) goto recv_again;
	if(errno==EAGAIN) {
	    dpf("    eagain\n");
	    fdinfo[fd].we_should_epoll_for_reads=1;
	    epoll_update(fd);
	    return;
	}
	perror("read");
	msg = "tcpsocks: Read failure from SOCKS5 server\n";
    } else
    if(nn==0){
        msg = "tcpsocks: End of file from SOCKS5 server [phase 3]\n";
    } else
    if(buf[0]!=5) {
	msg = "tcpsocks: Not a SOCKS5 reply from SOCKS5 server [phase 3]\n";
    } else
    if(buf[1]!=0) {
	switch(buf[1]) {
	    case 1: msg = "tcpsocks: general SOCKS server failure\n"; break;
	    case 2: msg =  "tcpsocks: connection not allowed by ruleset\n"; break; 
	    case 3: msg =  "tcpsocks: Network unreachable\n"; break; 
	    case 4: msg =  "tcpsocks: Host unreachable (auth required for 3proxy?)\n"; break; 
	    case 5: msg =  "tcpsocks: Connection refused (invalid username for 3proxy?)\n"; break; 
	    case 6: msg =  "tcpsocks: TTL expired (failed password for 3proxy?)\n"; break; 
	    case 7: msg =  "tcpsocks: Command not supported\n"; break; 
	    case 8: msg =  "tcpsocks: Address type not supported\n"; break; 
	    default: msg = "tcpsocks: Unknown error in SOCKS5 server\n"; break; 
	}
    } else
    if(buf[3]!=1) {
	msg = "tcpsocks: Not an IPv4 address in SOCKS5 reply\n";
    } else
    if(nn!=10) {
	msg = "tcpsocks: Not exactly 10 bytes is received from SOCKS5 server [phase 3]\n";
    }
    
    print_connection(fd, "    ", " ");

    if(msg) {
	send(fdinfo[fd].peerfd, msg, strlen(msg),0);
	fprintf(stdout, "%s", msg);
	close_fd(fd);
	return;
    } 

    printf("Started \n"); /* otherwise \n is printed by error message */

    /* SOCKS5 connection successed. Setting up piping. */

    int peerfd = fdinfo[fd].peerfd;
    fdinfo[fd].status='|';
    fdinfo[peerfd].status='|';
    
    fdinfo[fd].we_should_epoll_for_reads = 1;
    epoll_update(fd);

    struct epoll_event ev;
    memset(&ev, 0, sizeof ev);
    fdinfo[peerfd].we_should_epoll_for_reads = 1;
    ev.events = EPOLLIN  | EPOLLONESHOT;
    ev.data.fd = peerfd;
    if (epoll_ctl(kdpfd, EPOLL_CTL_ADD, peerfd, &ev) < 0) {
	fprintf(stderr, "epoll peer set insertion error\n");
	write(peerfd, "epoll peer set insertion error\n", 31);
	close_fd(fd);
	return;
    }
    fdinfo[peerfd].writeready = 1; /* Nothing bad if it is really not */
    fdinfo[fd].writeready = 1; /* Nothing bad if it is really not */
}
