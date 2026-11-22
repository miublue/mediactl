CC=tcc
LIBS=`pkg-config --cflags --libs dbus-1`
PREFIX=/usr/local
CFLAGS=-Wall -Werror -Wextra

all:
	${CC} -O2 -o mediactl mediactl.c ${CFLAGS} ${LIBS}

install: all
	mkdir -p ${PREFIX}/bin
	install -s mediactl ${PREFIX}/bin

uninstall:
	rm ${PREFIX}/bin/mediactl

