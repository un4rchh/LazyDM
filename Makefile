#lazydm - simple lightwaight display manager
# See LICENSE file for copyright and license details.

include config.mk

SRC = lazy.c auth.c desktop.c session.c term.c utils.c
OBJ = ${SRC:.c=.o}
TARGET = lazydm

all: ${TARGET}

.c.o:
	${CC} -c ${CFLAGS} $<

${OBJ}: config.h config.mk

config.h:
	cp config.def.h $@

${TARGET}: ${OBJ}
	${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	rm -f ${TARGET} ${OBJ} ${TARGET}-${VERSION}.tar.gz

dist: clean
	mkdir -p ${TARGET}-${VERSION}
	cp -R LICENSE Makefile README config.def.h config.mk \
		lazy.c auth.c desktop.c session.c term.c utils.c ${TARGET}-${VERSION}
	tar -cf ${TARGET}-${VERSION}.tar ${TARGET}-${VERSION}
	gzip ${TARGET}-${VERSION}.tar
	rm -rf ${TARGET}-${VERSION}
	rm -f ${TARGET} ${OBJ} ${TARGET}-${VERSION}.tar.gz config.h

install: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f ${TARGET} ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/${TARGET}
	mkdir -p ${DESTDIR}/etc/lazydm
	cp -f config.def.h ${DESTDIR}/etc/lazydm/config.def.h
	chmod 644 ${DESTDIR}/etc/lazydm/config.def.h

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/${TARGET}

.PHONY: all clean dist install uninstall
