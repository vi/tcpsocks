all: tcpsocks

tcpsocks: *.c VERSION
		gcc -O2 tcpsocks.c -o tcpsocks

githead=$(wildcard .git/HEAD)

ifeq (${githead}, .git/HEAD)
VERSION: .git
	( echo -n '#define VERSION "'; git describe --dirty 2> /dev/null | tr -d '\n'; echo '"' ) > VERSION
else
VERSION:
	echo '#define VERSION "unknown"' > VERSION
endif

clean:
	rm -f VERSION tcpsocks *.o

#install target for checkinstall
install: tcpsocks
	install -o root -g staff tcpsocks /usr/bin/
