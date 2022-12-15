cc = gcc
CFLAGS = -g -w

all:: memgrinder
	
memgrinder: memgrinder.c
	$(CC) $(CFLAGS) -o memgrinder memgrinder.c -L../writeonceFS.a

clean:
	rm memgrinder
