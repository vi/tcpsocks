static void parse_argv(int argc, char* argv[]) {
    if (argc != 5) {
	printf
	    ("Usage: tcprelay 0.0.0.0 1236 80.83.124.150 2222\n"
	     "                bind_ip port connect_ip connect_port\n\n"
	     "If you specify REDIRECT as connect address or port, I will get addresses using SO_ORIGINAL_DST\n"
	     "(useful with \"iptables -t nat ... -j REDIRECT\")\n"
	     "Parameters will be: tcprelay 0.0.0.0 1236 REDIRECT REDIRECT\n"
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
}
