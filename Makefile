
CC = gcc
CFLAGS = -W -Wall
SRC = ./src/*.c
PREFIX = /usr/local


all: $(SRC)
	$(CC) $(SRC) -o wavfix $(CFLAGS)


.PHONY: install
install: wavfix
	mkdir -p $(PREFIX)/bin
	cp $< $(PREFIX)/bin/wavfix

.PHONY: uninstall
uninstall:
	rm -f $(PREFIX)/bin/wavfix

