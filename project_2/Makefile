CC = gcc
CFLAGS = -g

all: part1 part2 part3 part4 iobound cpubound

part1: part1.o
	$(CC) $(CFLAGS) -o $@ $^

part2: part2.o
	$(CC) $(CFLAGS) -o $@ $^

part3: part3.o
	$(CC) $(CFLAGS) -o $@ $^

part4: part4.o
	$(CC) $(CFLAGS) -o $@ $^

iobound: iobound.c
	$(CC) -o $@ $<

cpubound: cpubound.c
	$(CC) -o $@ $<

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

clean:
	rm part1 part2 iobound cpubound *.o