all: countdown

countdown: countdown.c
	gcc --std=c99 -lrt -lm -o countdown countdown.c

clean:
	rm *.o countdown

