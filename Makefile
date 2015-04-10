CC=g++
CFLAGS=-I.

FT: FAST_TrIPs.o
	$(CC) -o FT FAST_TrIPs.o