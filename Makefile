CC=g++
CFLAGS=-I. -g

FAST_TrIPs.o: FAST_TrIPs.cpp
	$(CC) -c -g FAST_TrIPs.cpp

FT: FAST_TrIPs.o
	$(CC) -o FT FAST_TrIPs.o