CC=gcc

CFLAGS+= -O3 -Wno-int-to-void-pointer-cast

%.o: %.c
	$(CC) -g -c -fpic -o $@ $< $(CFLAGS)

ht_test: dinamite_hashtable.o hashtable_test.o
	$(CC) -o ht_test $^

all: ht_test

clean:
	rm *.o
