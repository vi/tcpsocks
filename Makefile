all: tcprelay

tcprelay: *.c *.h
		gcc -Wall -O2 *.c -o tcprelay
