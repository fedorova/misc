CC=gcc
CFLAGS=-I.
LIBS = -pthread
DEPS = fairlock.h
OBJ = locks.o fairlock.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

locks: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)
