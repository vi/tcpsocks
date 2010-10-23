static void parse_argv(int argc, char* argv[]) {
    if (argc != 7 && argc != 5) {
	printf
	    ("Usage: tcprelay 0.0.0.0 1236 80.83.124.150 2222\n"
	     "                bind_ip port connect_ip connect_port\n");
	exit(1);
    }

    /* Setup the command line arguments */
    bind_ip = argv[1];
    bind_port = atoi(argv[2]);
    connect_ip = argv[3];
    connect_port = atoi(argv[4]);
}
