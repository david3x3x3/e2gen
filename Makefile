#CC=x86_64-w64-mingw32-gcc
CC=gcc
#CC=clang
CFLAGS=-O6

all: e2 eternity2-score-16-32768

e2: e2.o
	$(CC) $(CFLAGS) -o e2 e2.o

eternity2-score-16-32768: solveforscore.o
	$(CC) $(CFLAGS) -o $@ solveforscore.o
