all: tcprelay

tcprelay: *.c
		gcc -O2 tcprelay.c -o tcprelay
