
CC = gcc
CFLAGS = -W -Wall
SRC = ./src/*.c
PREFIX = /usr/local
BIN = ./bin/


all: $(SRC)
	$(CC) $(SRC) -o $(BIN)wavfix $(CFLAGS)

win32:
	i686-w64-mingw32-$(CC) $(SRC) -o $(BIN)wavfix-win32.exe $(CFLAGS)

win64:
	x86_64-w64-mingw32-$(CC) $(SRC) -o $(BIN)wavfix-win64.exe $(CFLAGS)



.PHONY: install
install: wavfix
	mkdir -p $(PREFIX)/bin
	cp $< $(PREFIX)/bin/wavfix

.PHONY: uninstall
uninstall:
	rm -f $(PREFIX)/bin/wavfix
