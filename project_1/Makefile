CC=gcc
CFLAGS=-g

all: pseudo-shell

pseudo-shell: command.o main.o string_parser.o
	$(CC) $(CFLAGS) -o $@ $^

main.o: main.c string_parser.h
	$(CC) $(CFLAGS) -c main.c

command.o: command.c command.h string_parser.h
	$(CC) $(CFLAGS) -c command.c

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f pseudo-shell output.txt *.o