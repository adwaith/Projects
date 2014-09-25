CC=gcc
CFLAGS=-I. -I../../../common/ -I../../common/

%.o: %.c 
	$(CC) -c -o $@ $< $(CFLAGS)

all: main.o ../../common/slipstream.o ../../../common/pkt.o
	gcc -o sparkmeter-thresh main.o ../../common/slipstream.o ../../../common/pkt.o -I. -I../../../common/ -I../../common/
clean: 
	rm -f ../../common/*.o ../../../common/*.o *.o *~ core sparkmeter-thresh 

