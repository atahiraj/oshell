CC=gcc
CFLAGS=-g --pedantic -Wall -Wextra -Wmissing-prototypes -std=c99
PROGRAM=oshell

# DO NOT MODIFY CC, CFLAGS and PROGRAM

$(PROGRAM): main.o oshell.o queue.o
	$(CC) $(CFLAGS) -o $(PROGRAM) main.o oshell.o queue.o

main.o: main.c oshell.h
	$(CC) $(CFLAGS) -o main.o -c main.c

oshell.o: oshell.c oshell.h queue.h
	$(CC) $(CFLAGS) -o oshell.o -c oshell.c

queue.o: queue.h
	$(CC) $(CFLAGS) -o queue.o -c queue.c
clean:
	rm -f main.o oshell.o queue.o oshell
