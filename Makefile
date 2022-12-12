cc = gcc
CFLAGS = -g -c
AR = ar -rc
RANLIB = ranlib

all:
	gcc -Wall -c writeonceFS.c -o writeonceFS.o
	$(AR) writeonceFS.a writeonceFS.o
	$(RANLIB) writeonceFS.a
	
clean:
	rm *.o *.a
