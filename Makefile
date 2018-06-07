all: countdown

CFLAGS="--std=c99 -lrt

timefunctions.o: timefunctions.c timefunctions.h
	gcc -c --std=c99 -o timefunctions.o timefunctions.c

countdown.o: countdown.c timefunctions.h
	gcc -c --std=c99 -o countdown.o countdown.c

countdown: countdown.o timefunctions.o
	gcc --std=c99 -lrt -lm -o countdown countdown.o timefunctions.o

clean:
	rm *.o countdown

