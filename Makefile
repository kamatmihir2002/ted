CC=gcc
BIN=bin

all:
	$(CC) ted.c -o $(BIN)/ted -lncurses

clean:
	rm $(BIN)/ted
