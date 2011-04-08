Simple Linux epoll-based single thread SOCKS5 client.
Supports getting destination address with SO_ORIGINAL_DST (for use with `-j REDIRECT` iptables target) and telling that address to SOCKS server.

Author page: [vi-server.org](http://vi-server.org/)

Alternative, more complete libevent-based implementation: [redsocks](https://github.com/darkk/redsocks)

Example setup:

    # # Prepare iptables:
    # iptables -t nat -N QQQ
    # iptables -t nat -A QQQ -d 80.83.124.150 -p tcp --dport 22 -j RETURN
    # iptables -t nat -A QQQ -p tcp -j REDIRECT --to-ports 1234
    # iptables -t nat -I OUTPUT 1 -j QQQ
    # iptables -t nat -I PREROUTING 1 -j QQQ
    
    $ # connect to SSH:
    $ ssh -D 127.0.0.1:1080 vi@80.83.124.150
    
    $ # start tcpsocks:
    $ tcpsocks 0.0.0.0 1234 REDIRECT REDIRECT 127.0.0.1 1080
    192.168.99.2:54761 -> 66.102.13.103:80 [5->6]
        192.168.99.2:54761 -> 66.102.13.103:80 [5->6] Started 
        192.168.99.2:54761 -> 66.102.13.103:80 [5->6] Finished
    192.168.99.2:56097 -> 79.105.152.94:24692 [5->6]
        192.168.99.2:56097 -> 79.105.152.94:24692 [5->6] Started
    192.168.99.2:45627 -> 79.97.43.185:59283 [17->18]
        192.168.99.2:45620 -> 79.97.43.185:59283 [13->14] tcpsocks: End of file from SOCKS5 server [phase 3]
        192.168.99.2:45620 -> 79.97.43.185:59283 [13->14] Finished
    ?
    Commands: quit list kill Debug
    l
        192.168.99.2:56097 -> 79.105.152.94:24692 [5->6] 2317:425
    k 5
        192.168.99.2:56097 -> 79.105.152.94:24692 [5->6] Finished
    l

tcpsocks allow interactive control of connections: you can list then and kill (by specifying fd). When listing, it prints uploaded/downloaded bytes for each connection.

tcpsocks does not require configuration expect of command line parameters.

You can't limit connection speed, use [tcplim](https://github.com/vi/tcplim) for this.


