CC=gcc
BIN=bin
INSTALLDIR=/usr/bin
all:
	$(CC) ted.c -o $(BIN)/ted -lncurses

clean:
	rm $(BIN)/ted

install:
	$(CC) ted.c -o $(INSTALLDIR)/ted -lncurses

purge:
	rm $(INSTALLDIR)/ted

