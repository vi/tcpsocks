all: tcpsocks

tcpsocks: *.c
		gcc -O2 tcpsocks.c -o tcpsocks
