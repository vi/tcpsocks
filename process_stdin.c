#include "main.h"

void print_connection(int fd, const char* prologue, const char* epilogue) {
    int peerfd = fdinfo[fd].peerfd;
    
    printf("%s%s:%d -> ",
	   prologue,
	   inet_ntoa(fdinfo[fd].address.sin_addr),
	   ntohs(fdinfo[fd].address.sin_port)
	  );
    printf("%s:%d [%d->%d]",
	   inet_ntoa(fdinfo[peerfd].address.sin_addr),
	   ntohs(fdinfo[peerfd].address.sin_port), 
	   fd, 
	   peerfd);

    if (fdinfo[fd].total_read || fdinfo[peerfd].total_read) {
	printf(" %lld:%lld",
	       fdinfo[fd].total_read,
	       fdinfo[peerfd].total_read);
    }
    printf("%s", epilogue);
}

void list_connections() {
    int fd;
    for(fd=0; fd<MAXFD; ++fd) {
	if(fdinfo[fd].status == 0 || fdinfo[fd].status == '.') {
	    continue;
	}
	if(fdinfo[fd].group != 'c') {
	    continue;
	}

	print_connection(fd, "    ", "\n");
    }
}

void process_stdin() {
    dpf("processing stdin\n");
    char cmd[256]={0};
    char arg[256]={0};
    scanf("%256s%256s", cmd, arg); 
    dpf("    c=\"%s\" arg=\"%s\"\n", cmd, arg);
    switch(cmd[0]) {
	case 'q':
	    exit(0);
	    break;
	case 'l':
	    list_connections();
	    break;
	case 'k':
	    if(!arg[0]) {
                printf("k[ill] fd_number\n");
	    } else {
		close_fd(atoi(arg));
	    }
	    break;
	case 'D':
	    debug_output = !debug_output;
	    break;
	case '\0':
	    break;
	default:
	    printf("Commands: quit list kill Debug\n");
    }
}
