all: countdown

countdown: countdown.c
	gcc -lrt -lm -o countdown countdown.c

