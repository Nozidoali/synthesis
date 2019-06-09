CC = g++
WFLAGS = -lm -ldl -lpthread

All: test

test: why_main.o
	$(CC) -g -o $@ why_main.o libabc.a $(WFLAGS)

why_main.o: why_main.cpp
	$(CC) -g -c why_main.cpp -o why_main.o

clean:
	rm -rf *.o
