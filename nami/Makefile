

CFLAGS= -Wall -I../inc/ -I../libami -L../libami -g


NAMI_OBJECTS=nami.o ncurses_support.o events.o config.o


all: nami amiwatch amiconsole

nami: build_h ${NAMI_OBJECTS}
	cc ${CFLAGS} -o nami ${NAMI_OBJECTS} -L. -lami -lncurses

build_h:
	./gen_build_h > build.h

clean:
	rm  -f *.o nami build.h amiwatch

install: all
	cp nami /usr/bin/
	cp amiwatch /usr/bin/

uninstall:
	rm -f /usr/bin/nami

amiwatch: amiwatch.o

	gcc ${CFLAGS} -o amiwatch -L. -lami amiwatch.o

amiconsole: amiconsole.o

	gcc ${CFLAGS} -o amiconsole -L. -lami amiconsole.o


sample:

	cp nami.cfg.sample /etc/nami.cfg
