CC = gcc

all: thread_signaling barriers bank

thread_signaling: thread_signaling.o
	$(CC) -pthread -lpthread $^ -o $@

barriers: barriers.o
	$(CC) -pthread -lpthread $^ -o $@

bank: bank.o
	$(CC) -pthread -lpthread $^ -o $@

%.o: %.c
	$(CC) -c $<

clean:
	rm -f core *.o thread_signaling barriers bank