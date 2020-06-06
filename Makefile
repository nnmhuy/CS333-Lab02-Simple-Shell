CC=gcc
CFLAGS= -Wall -g

run:
	make all
	./simple-shell

all: 
	$(CC) $(CFLAGS) simple-shell.c -o simple-shell

clean:
	rm -rf *o main
	rm -rf simple-shell
	
