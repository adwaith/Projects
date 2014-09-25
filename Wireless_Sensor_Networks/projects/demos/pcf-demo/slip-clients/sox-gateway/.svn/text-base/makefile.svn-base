SOX_ROOT=/home/agr/sox/
SOX_LIBRARY_PATH=/home/agr/sox/libs/libc_strophe

-include $(SOX_LIBRARY_PATH)/vars.mk
UNAME := $(shell uname)

CC=gcc
LIBS = -l$(SOX_LIB_NAME) -lstrophe -lexpat -lpthread -lm -lresolv -lssl
ifeq ($(UNAME), Linux)
	 LIBS += -luuid
endif
INCLUDE=-I. -I../common/ -I$(STROPHE_INCLUDE) -I$(STROPHE_INCLUDE_SRC) -I$(SOX_LIBRARY_PATH) -I../../common/
CFLAGS= -g -Wall $(INCLUDE) -O2
LDFLAGS=-L. -L$(SOX_LIBRARY_PATH) -L$(STROPHE_LIB) $(LIBS)
DEFINES=-DUSE_SOX

all: slipclient.o slipstream.o ../../common/pkt.o
	gcc -o sox_gateway slipclient.o slipstream.o ../../common/pkt.o $(CFLAGS) $(LDFLAGS) $(DEFINES)
	
slipclient.o: slipclient.c
	gcc -c -o $@ $< $(CFLAGS) $(LDFLAGS) $(DEFINES)
	
pkt.o: pkt.c
	gcc -c -o $@ $< $(CFLAGS) $(LDFLAGS)
	
slipstream.o: ../common/slipstream.c
	gcc -c -o $@ $< $(CFLAGS) $(LDFLAGS)

clean: 
	rm -f ../../common/*.o *.o *~ core sox_gateway
	
