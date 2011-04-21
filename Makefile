#  Makefile for the jupiter speech adapter

all : jupiter

CFLAGS = -I../acsint

OBJS = jcmd.o jxlate.o jencode.o

$(OBJS) : jup.h ../acsint/acsbridge.h ../acsint/acsint.h

#  Need these objects from the bridge layer
BRIDGE = ../acsint/acsbridge.o ../acsint/acsbind.o

jupiter : $(OBJS) $(BRIDGE)
	cc -s -o jupiter $(OBJS) $(BRIDGE)

