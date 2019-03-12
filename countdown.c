// compile with: gcc -lrt -lm -o countdown countdown.c

#define _XOPEN_SOURCE //strptime, localtime_r
#define _DEFAULT_SOURCE //timersub
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <math.h>
#include "timefunctions.h"

void usage(char* msg){
	fprintf(stderr, "Usage: countdown [ NUMBER[SUFFIX]... | POINT_IN_TIME ]\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "SUFFIX may be one of:\n");
	fprintf(stderr, "'s' or no suffix for seconds,\n");
	fprintf(stderr, "'m' for minutes,\n");
	fprintf(stderr, "'h' for hours\n");
	fprintf(stderr, "'d' for days.\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "POINT_IN_TIME can have one of the following formats:\n");
	fprintf(stderr, "7:4 or 07:04 (today or tomorrow, seconds=0)\n");
	fprintf(stderr, "7:4:59 (today or tomorrow)\n");
	fprintf(stderr, "Monday 7:4 (weekday, in this or next week, seconds=0)\n");
	fprintf(stderr, "Monday 7:4:59 (weekday, in this or next week)\n");
	fprintf(stderr, "2014-07-22 7:4 (error if in the past, seconds=0)\n");
	fprintf(stderr, "2014-07-22 7:4:59 (error if in the past)\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Example 1: countdown 1.5m 3s\n");
	fprintf(stderr, "Example 2: countdown 16:4\n");
	if (msg != NULL) {
		fprintf(stderr, "\nError: %s\n", msg);
	}
}

/* Parse the number of seconds that are in the args.
`argv[i]` (with 1<=i<`argc`) may be a float or int number with a suffix.
Possible suffixes are:
's' or no suffix for seconds,
'm' for minutes,
'h' for hours
'd' for days.
Returns 1 if parsing into `seconds` succeeded.
Returns 0 if parsing failed. */
int parse_duration(int argc, char** argv, double* seconds) {
	double waittime = 0;
	int i;
	for (i=1; i<argc; i++) {
		float number=0;
		char suffix='\0';
		//TODO: make entering "1ms" raise an error
		if (sscanf(argv[i], "%f%c", &number, &suffix) < 1) {
			fprintf(stderr, "recognition error\n");
			return 0;
		}
		if (suffix == '\0') suffix = 's';
		switch(suffix) {
			case 's': waittime += number; break;
			case 'm': waittime += number*60; break;
			case 'h': waittime += number*60*60; break;
			case 'd': waittime += number*60*60*24; break;
			default: fprintf(stderr, "unknown suffix\n"); return 0;
		}
	}
	//printf("waittime:%f\n", waittime);
	*seconds = waittime;
	return 1;
}

int main(int argc, char** argv) {
	if (argc < 2) {
		usage("need at least one number");
		exit(1);
	}
	double waittime;
	{
		char *args;
		{
			// length of all argv
			int l=0;
			for (int i=1; i<argc; i++) {
				l += strlen(argv[i]);
			}
			l += 1; // \0 at the end
			args = malloc(sizeof(char)*l);
			
			// concatenate argv
			args[0] = '\0';
			for (int i=1; i<argc; i++) {
				strcat(args, argv[i]);
			}
		}

		struct tm tm_now, parsed_time;
		get_tm_now(&tm_now);
		if (parse_with_strptime(args, &parsed_time, &tm_now)) {
			// parsing was successful.
			waittime = tm_diff_to_now_seconds(&parsed_time);
			if (waittime < 0) {
				usage("time is in the past");
				exit(1);
			}
			// parsing was successful and time `tm_stop` is in the future
		} else if (!parse_duration(argc, argv, &waittime)) {
			usage("cannot parse time or duration");
			exit(1);
		}
	}

	// convert `waittime` to absolute time `tv_stop`.
	struct timeval tv_stop;
	{
		struct timeval tv_wait;
		{
			int waittime_int = (int)floor(waittime);
			tv_wait.tv_sec = waittime_int;
			tv_wait.tv_usec = 0;
			int waittime_usec = (int)((waittime - waittime_int) * 1000000);

			// wait for the fractional part of tv_wait.
			{
				printf("waiting %i microseconds", waittime_usec);
				fflush(stdout);
				if (usleep(waittime_usec) == -1) { // sleep microseconds
					perror("usleep error");
					exit(2);
				}
			}
		}

		// compute `tv_stop`.
		{
			struct timeval tv_start;
			if (gettimeofday(&tv_start, NULL) == -1) {
				perror("gettimeofday error");
				exit(2);
			}
			timeradd(&tv_start, &tv_wait, &tv_stop);
		}
	}
	
	// print `tv_stop` into `buf_stop`.
	char buf_stop[1000];
	{
		if (ctime_r(&(tv_stop.tv_sec), buf_stop) == NULL) {
			perror("ctime_r error");
			exit(2);
		}
		// remove the apparently existing newline sign.
		buf_stop[strlen(buf_stop)-1] = '\0';
	}
	
	while(1) {
		struct timeval tv_now;
		if (gettimeofday(&tv_now, NULL) == -1) {
			perror("gettimeofday error");
			exit(2);
		}
		if (!timercmp(&tv_now, &tv_stop, <)) {
			break;
		}
		
		struct timeval tv_rem; // remaining
		timersub(&tv_stop, &tv_now, &tv_rem);
		
		tv_rem.tv_sec += 1; //we're waiting 1s after printing, not before
		
		int d_rem = tv_rem.tv_sec / (60*60*24);
		int h_rem = (tv_rem.tv_sec % (60*60*24)) / (60*60);
		int m_rem = (tv_rem.tv_sec % (60*60)) / 60;
		int s_rem = (tv_rem.tv_sec % 60);
		printf("\r%i seconds (%i d %2i h %2i m %2i s) until %s", tv_rem.tv_sec, d_rem, h_rem, m_rem, s_rem, buf_stop);
		fflush(stdout);
		
		if (usleep(1000000) == -1) {
			perror("usleep error");
			exit(2);
		}
	}
	
	printf("\r0 seconds (0 d  0 h  0 m  0 s) until %s", buf_stop);
	printf("\n");
	
	return 0;
}
