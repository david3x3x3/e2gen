#CC=x86_64-w64-mingw32-gcc
CC=gcc
#CC=clang
CFLAGS=-O2

e2: e2.o
	$(CC) $(CFLAGS) -o e2 e2.o
