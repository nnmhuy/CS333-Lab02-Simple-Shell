CC=g++
CFLAGS= -Wall -g

run:
	make all
	./simple-shell

all: 
	$(CC) $(CFLAGS) *.c -o simple-shell

clean:
	rm -rf *o main
	
