
CC = gcc
CFLAGS = -W -Wall

all : wavfix.o libriff.o libwav.o libbext.o file.o
	$(CC) wavfix.o libriff.o libwav.o libbext.o file.o -o wavfix

wavfix.o : wavfix.c libriff.h libwav.h libbext.h file.h
	$(CC) -c wavfix.c -o wavfix.o $(CFLAGS)

libriff.o : libriff.c libriff.h
	$(CC) -c libriff.c -o libriff.o $(CFLAGS)

libwav.o : libwav.c libwav.h
	$(CC) -c libwav.c -o libwav.o $(CFLAGS)

libbext.o : libbext.c libbext.h
	$(CC) -c libbext.c -o libbext.o $(CFLAGS)

file.o : file.c file.h
	$(CC) -c file.c -o file.o $(CFLAGS)


clean :
	rm -rf *.o


