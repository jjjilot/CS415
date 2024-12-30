CC = gcc
CFLAGS = -g -pthread

all: bank

bank: bank.o
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

clean:
	rm bank Output/output.txt Output/ledger.txt *.o