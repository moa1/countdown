// compile with: gcc -lrt -lm -o countdown countdown.c

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <math.h>

void usage(char* msg){
	fprintf(stderr, "%s\nUsage: countdown [ NUMBER[SUFFIX]... | POINT_IN_TIME ]\nExample 1: countdown 1.5m 3s\nExample 2: countdown 16:4\n", msg);
}

int parse_duration(int argc, char** argv, float* seconds) {
	float waittime = 0;
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

int parse_timepoint(int argc, char** argv, float* seconds) {
	int i;
	int is_timepoint=0;
	for (i=1; i<argc; i++) {
		if (strchr(argv[i], ':')) {
			is_timepoint=1;
			break;
		}
	}
	if (!is_timepoint)
		return 0;
	if (argc > 2)
		return -1;
	
	int hour, minute;
	// TODO: allow specifying hh:mm:ss as well as hh:mm.
	int second = 0;
	if (sscanf(argv[1], "%i:%i", &hour, &minute) < 2) {
		fprintf(stderr, "recognition error\n");
		return -1;
	}
	
	// check whether the point in time is in the next day
	struct timeval tv_now;
	if (gettimeofday(&tv_now, NULL) == -1) {
		perror("gettimeofday error");
		exit(2);
	}
	struct tm tm;
	if (!localtime_r(&(tv_now.tv_sec), &tm)) {
		perror("localtime_r error");
		exit(2);
	}
	int tomorrow=0;
	if (tm.tm_hour > hour
		|| tm.tm_hour == hour && tm.tm_min > minute
		|| tm.tm_hour == hour && tm.tm_min == minute && tm.tm_sec != 0
		)
		tomorrow=1;
	
	// set tm to target time
	if (tomorrow) {
		// add 1 day
		tm.tm_mday += 1;
	}
	tm.tm_sec = 0;
	tm.tm_min = minute;
	tm.tm_hour = hour;
	
	time_t time_stop = mktime(&tm);
	if (time_stop == -1) {
		perror("mktime error");
		exit(2);
	}
	
	// convert time_stop to timeval-structure tv_stop
	struct timeval tv_stop;
	tv_stop.tv_sec = time_stop;
	tv_stop.tv_usec = 0;
	
	// compute difference between tv_now and tv_stop
	struct timeval tv_diff;
	timersub(&tv_stop, &tv_now, &tv_diff);
	float waittime = (float)tv_diff.tv_usec/1000000 + tv_diff.tv_sec;

	*seconds = waittime;
	return 1;
}

int main(int argc, char** argv) {
	if (argc < 2) {
		usage("need at least one number");
		exit(1);
	}
	float waittime;
	int r = parse_timepoint(argc, argv, &waittime);
	if (r == -1) {
		usage("cannot parse point in time");
		exit(1);
	} else if (r == 0) {
		if (!parse_duration(argc, argv, &waittime)) {
			usage("cannot parse duration");
			exit(1);
		}
	} else if  (r != 1) {
		printf("Internal error: unknown return value\n");
		exit(9);
	}
		
	struct timeval tv_wait;
	tv_wait.tv_sec = (int)floorf(waittime);
	tv_wait.tv_usec = (int)((waittime - floorf(waittime)) * 1000000);
	
	struct timeval tv_start;
	if (gettimeofday(&tv_start, NULL) == -1) {
		perror("gettimeofday error");
		exit(2);
	}
	struct timeval tv_stop;
	timeradd(&tv_start, &tv_wait, &tv_stop);

	char buf_stop[1000];
	if (ctime_r(&(tv_stop.tv_sec), buf_stop) == NULL) {
		perror("ctime_r error");
		exit(2);
	}
	buf_stop[strlen(buf_stop)-1] = '\0';	//remove the apparently existing newline sign
	
	printf("waiting %i microseconds", tv_wait.tv_usec);
	fflush(stdout);
	
	if (usleep(tv_wait.tv_usec) == -1) { // sleep microseconds
		perror("usleep error");
		exit(2);
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
	
	printf("\r0 seconds (0 d    0 h  0 m  0 s) until %s", buf_stop);
	printf("\n");
	
	return 0;
}
