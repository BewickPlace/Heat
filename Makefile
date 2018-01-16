PREFIX = /usr/local

CDEBUGFLAGS = -Os -g -Wall

DEFINES = $(PLATFORM_DEFINES)

CFLAGS = $(CDEBUGFLAGS) $(DEFINES) $(EXTRA_DEFINES)

SRCS = heat.c timers.c networks.c errorcheck.c application.c dht11.c display.c config.c control.c

OBJS = heat.o timers.o networks.o errorcheck.o application.o dht11.o display.o config.o control.o

LDLIBS = -lrt -lwiringPi -lpthread

heat: $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o heat $(OBJS) $(LDLIBS)

.PHONY: all install.minimal install

all: heat

install.minimal: all

install: all install.minimal

.PHONY: uninstall

uninstall:

.PHONY: clean

clean:
	-rm -f heat
	-rm -f *.o *~ core TAGS gmon.out
