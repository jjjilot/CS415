CC=gcc

all: lab2

lab2: command.o main.o
	$(CC) -o $@ $^

main.o: main.c
	$(CC) -c main.c

%.o: %.c %.h
	$(CC) -c $<

clean:
	rm -f lab2 *.o