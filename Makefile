CFLAGS = -g -O -Wall -Iinclude
CC=gcc ${CFLAGS}

all: bin/test bin/sillycon

# TODO: make proper shared object

bin/test: test/test.o core/bps.o
	${CC} -o $@ $^ 

bin/sillycon: sillycon/sillycon.o core/bps.o
	${CC} -o $@ $^

sillycon/sillycon.c: include/bps.h

test/test.c: include/bps.h include/bps_types.h

install: bin/sillycon
	cp bin/sillycon /usr/local/bin
	chmod a+x  /usr/local/bin/sillycon


clean: 
	rm -f lib/bps.o
	rm -f bin/sillycon
	rm -f bin/test
	rm -f core/*.o
	rm -f sillycon/*.o
	rm -f test/*.o

