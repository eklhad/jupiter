#  Makefile for the jupiter speech adapter

all : jupiter

CFLAGS = -I../acsint

OBJS = jcmd.o jxlate.o jencode.o

$(OBJS) : jup.h ../acsint/acsbridge.h ../acsint/acsint.h

BRIDGE = ../acsint/libacs.a

jupiter : $(OBJS) $(BRIDGE)
	cc -s -o jupiter $(OBJS) $(BRIDGE)

