#  Makefile for the jupiter speech adapter

all : jupiter

CFLAGS = -I../acsint/bridge -I../acsint/driver

OBJS = jupiter.o tpencode.o tpxlate.o

$(OBJS) : tp.h ../acsint/bridge/acsbridge.h ../acsint/driver/acsint.h

BRIDGE = ../acsint/bridge/libacs.a

jupiter : $(OBJS) $(BRIDGE)
	cc -s -o jupiter $(OBJS) $(BRIDGE)

