CC=gcc
CFLAGS=-I.

%.o: %.c 
	$(CC) -c -o $@ $< $(CFLAGS)

all: main.o
	$(CC) -o SLIPstream main.o -I.
clean: 
	rm -f *.o *~ core SLIPstream

