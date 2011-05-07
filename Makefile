#  Makefile for the jupiter speech adapter

all : jupiter

CFLAGS = -I../acsint/bridge -I../acsint/driver

OBJS = jcmd.o jxlate.o jencode.o

$(OBJS) : jup.h ../acsint/bridge/acsbridge.h ../acsint/driver/acsint.h

BRIDGE = ../acsint/bridge/libacs.a

jupiter : $(OBJS) $(BRIDGE)
	cc -s -o jupiter $(OBJS) $(BRIDGE)

