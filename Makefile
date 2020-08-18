include config.mk

PROG = focusws focusmon focuswin
SRCS = ${PROG:=.c} util.c
OBJS = ${SRCS:.c=.o}

all: ${PROG}

focuswin: focuswin.o util.o
	${CC} -o $@ focuswin.o util.o ${LDFLAGS}
focuswin.o: util.h

focusws: focusws.o util.o
	${CC} -o $@ focusws.o util.o ${LDFLAGS}
focusws.o: util.h

focusmon: focusmon.o util.o
	${CC} -o $@ focusmon.o util.o ${LDFLAGS}
focusmon.o: util.h

.c.o:
	${CC} ${CFLAGS} -c $<

clean:
	-rm ${OBJS} ${PROG}

install: all
	install -D -m 755 focuswin ${DESTDIR}${PREFIX}/bin/focuswin
	install -D -m 644 focuswin.1 ${DESTDIR}${MANPREFIX}/man1/focuswin.1
	install -D -m 755 focusmon ${DESTDIR}${PREFIX}/bin/focusmon
	install -D -m 644 focusmon.1 ${DESTDIR}${MANPREFIX}/man1/focusmon.1
	install -D -m 755 focusws ${DESTDIR}${PREFIX}/bin/focusws
	install -D -m 644 focusws.1 ${DESTDIR}${MANPREFIX}/man1/focusws.1

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/focuswin
	rm -f ${DESTDIR}${MANPREFIX}/man1/focuswin.1
	rm -f ${DESTDIR}${PREFIX}/bin/focusmon
	rm -f ${DESTDIR}${MANPREFIX}/man1/focusmon.1
	rm -f ${DESTDIR}${PREFIX}/bin/focusws
	rm -f ${DESTDIR}${MANPREFIX}/man1/focusws.1

.PHONY: all clean install uninstall
