CC = g++
CFLAGS = -Wall

All: test

test: main.o
	$(CC) $(CFLAGS) -o $@ main.cpp

clean: 
	rm -rf *.o
