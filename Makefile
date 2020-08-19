PREFIX = /usr/local

CDEBUGFLAGS = -Os -g -Wall

DEFINES = $(PLATFORM_DEFINES)

CFLAGS = $(CDEBUGFLAGS) $(DEFINES) $(EXTRA_DEFINES)

SRCS = heat.c timers.c networks.c errorcheck.c application.c dht11.c display.c config.c control.c proximity.c

OBJS = heat.o timers.o networks.o errorcheck.o application.o dht11.o display.o config.o control.o proximity.o

DEPS = heat.h timers.h networks.h errorcheck.h application.h dht11.h display.h

LDLIBS = -lrt -lwiringPi -lpthread -lbluetooth

all: heat

install: heat
	install -m 775 -d $(PREFIX)/bin
	install -m 775 heat $(PREFIX)/bin/heat
ifeq ($(wildcard /etc/heat.conf),)
	install -m 666 scripts/heat.conf /etc/heat.conf
endif
ifeq ($(wildcard /etc/heating.conf),)
	install -m 666 scripts/heating.conf /etc/heating.conf
endif
ifeq ($(wildcard /etc/systemd/system/heat.service),)
	install -m 755 scripts/heat.service /etc/systemd/system/heat.service
endif
ifeq ($(wildcard /var/www/html/changelog.txt),)
	install -m 644 -o www-data html/*.* /var/www/html/
endif
ifeq ($(wildcard /home/pi/monitor.py),)
	install -m 644 scripts/monitor.py /home/pi/
endif
ifeq ($(wildcard /etc/network/interfaces),)
	install -m 666 scripts/interfaces /etc/network/interfaces
endif
ifeq ($(wildcard /etc/wpa_supplicant/wpa_supplicant.conf),)
	install -m 666 scripts/wpa_supplicant.conf /etc/wpa_supplicant/wpa_supplicant.conf
endif

clear:
	rm -f /etc/heat.conf
	rm -f /etc/heating.conf
	rm -f /etc/systemd/system/heat.service
	rm -f /var/www/html/changelog.txt
	rm -f /home/pi/monitor.py
	rm -f /etc/network/interfaces
	rm -f /etc/wpa_supplicant/wpa_supplicant.conf
	rm -f /var/log/heat.log
	rm -f /var/log/monitor.log

release: clear
	$(MAKE) install

%.o: %.c $(DEPS)
	$(CC) -c $(CFLAGS) $<

OBJS := $(SRCS:.c=.o)

heat: $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o heat $(OBJS) $(LDLIBS)


clean:
	-rm -f heat
	-rm -f *.o *~ core TAGS gmon.out
