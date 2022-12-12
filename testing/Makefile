cc = gcc
CFLAGS = -g -w

all:: memgrinder
	
memgrinder:
	$(CC) $(CFLAGS) -o memgrinder memgrinder.c -L../writeonceFS.a

clean:
	rm memgrinder
