
CFLAGS=-g -Wall -pedantic

test_read_write: test_read_write.c
	gcc $(CFLAGS) -o $@ $^ myio.c

all: clean
.PHONY: clean
clean:
	rm -f *.o

