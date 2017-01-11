
CC = gcc
CFLAGS = -W -Wall

all : ./src/wavfix.o ./src/libriff.o ./src/libwav.o ./src/libbext.o ./src/file.o
	$(CC) ./src/wavfix.o ./src/libriff.o ./src/libwav.o ./src/libbext.o ./src/file.o -o wavfix

./src/wavfix.o : ./src/wavfix.c ./src/libriff.h ./src/libwav.h ./src/libbext.h ./src/file.h
	$(CC) -c ./src/wavfix.c -o ./src/wavfix.o $(CFLAGS)

./src/libriff.o : ./src/libriff.c ./src/libriff.h
	$(CC) -c ./src/libriff.c -o ./src/libriff.o $(CFLAGS)

./src/libwav.o : ./src/libwav.c ./src/libwav.h
	$(CC) -c ./src/libwav.c -o ./src/libwav.o $(CFLAGS)

./src/libbext.o : ./src/libbext.c ./src/libbext.h
	$(CC) -c ./src/libbext.c -o ./src/libbext.o $(CFLAGS)

./src/file.o : ./src/file.c ./src/file.h
	$(CC) -c ./src/file.c -o ./src/file.o $(CFLAGS)


clean :
	rm -rf ./src/*.o


