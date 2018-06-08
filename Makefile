all: countdown remindme

#DEBUG=-g
CFLAGS=--std=c99 ${DEBUG}
LDFLAGS=-lrt -lm ${DEBUG}

timefunctions.o: timefunctions.c timefunctions.h
	gcc ${CFLAGS} -c -o timefunctions.o timefunctions.c

countdown.o: countdown.c timefunctions.h
	gcc ${CFLAGS} -c -o countdown.o countdown.c

countdown: countdown.o timefunctions.o
	gcc ${LDFLAGS} -o countdown countdown.o timefunctions.o

remindme.o: remindme.c timefunctions.h
	gcc ${CFLAGS} -c -o remindme.o remindme.c

remindme: remindme.o timefunctions.o
	gcc ${LDFLAGS} -o remindme remindme.o timefunctions.o

clean:
	rm -f *.o countdown remindme

