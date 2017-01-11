
CC = gcc
CFLAGS = -W -Wall
SRC = ./src/*.c

all: $(SRC)
	$(CC) $(SRC) -o wavfix $(CFLAGS)

#clean :
#	rm -rf ./src/*.o

