#include "main.h"
#include "VERSION"

void parse_argv(int argc, char* argv[]) {

    if (getenv("DEBUG")) {
	debug_output = 1;
    } else {
	debug_output = 0;
    }

    if (argc != 7 && argc != 9) {
	printf
	    (
	     "tcpsocks version " VERSION "\n"
	     "Usage: tcpsocks 0.0.0.0 1236 REDIRECT REDIRECT 127.0.0.1 1080 [username password]\n"
	     "                bind_ip port destination_ip destination_port socks_ip socks_port socks_username socks_password\n\n"
	     "If you specify REDIRECT as destination address or port, I will get addresses using SO_ORIGINAL_DST and tell it to SOCKS server\n"
	     "(useful with \"iptables -t nat ... -j REDIRECT\")\n"
	     );
	exit(1);
    }

    /* Setup the command line arguments */
    bind_ip = argv[1];
    bind_port = atoi(argv[2]);
    
    need_address_redirection = !strcmp(argv[3], "REDIRECT");
    need_port_redirection = !strcmp(argv[4], "REDIRECT");

    if (!need_address_redirection) {
	connect_ip = argv[3];
    }
    if (!need_port_redirection) {
	connect_port = atoi(argv[4]);
    }

    socks_ip = argv[5];
    socks_port = atoi(argv[6]);

    if (argc == 9) {
	socks_user = argv[7];
	socks_password = argv[8];
	need_password = 1;
    } else {
	need_password = 0;
    }
}
