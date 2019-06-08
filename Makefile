CC = g++
CFLAGS = -Wall

All: test

test:
	$(CC) $(CFLAGS) -o $@ main.cpp
