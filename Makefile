#  Makefile for the jupiter speech adapter

prefix = /usr/local
exec_prefix = ${prefix}
bindir = ${exec_prefix}/bin

DEPFLAG =  -MMD
CFLAGS += $(DEPFLAG)

DRIVERPATH = ../acsint/drivers
ifneq "$(DRIVERPATH)" ""
	CFLAGS += -I$(DRIVERPATH)
	endif

LDLIBS = -lacs

SRCS = jupiter.c tpencode.c tpxlate.c
OBJS = $(SRCS:.c=.o)

all : jupiter

jupiter : $(OBJS)

install : jupiter
	${INSTALL} -d ${DESTDIR}${bindir}
	${INSTALL_PROGRAM} jupiter ${DESTDIR}${bindir}

uninstall :
	rm -f ${DESTDIR}${bindir}/jupiter

-include $(SRCS:.c=.d)
