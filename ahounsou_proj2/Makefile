
CC = gcc
CFLAGS = 

all:  sim

sim:  main.o cache.o
	$(CC) -o sim main.o cache.o -lm

main.o:  main.c cache.h
	$(CC) $(CFLAGS) -g -c main.c

cache.o:  cache.c cache.h
	$(CC) $(CFLAGS) -g -c cache.c

clean:
	rm *.o sim 
