CC = gcc

all: threads pipes

threads: threads.o
	$(CC) -pthread -lpthread $^ -o $@

pipes: pipes.o
	$(CC) -pthread -lpthread $^ -o $@

threads.o: threads.c
	$(CC) -c $<

pipes.o: pipes.c
	$(CC) -c $<

clean:
	rm -f core *.o threads pipes