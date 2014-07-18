// compile with: gcc -lrt -lm -o countdown countdown.c

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <math.h>

void usage(char* msg){
	fprintf(stderr, "%s\nUsage: countdown NUMBER[SUFFIX]...\n", msg);
}

int main(int argc, char** argv) {
	if (argc < 2) {
		usage("need at least one number");
		exit(1);
	}
	int i;
	float waittime = 0;
	for (i=1; i<argc; i++) {
		float number=0;
		char suffix='\0';
		//TODO: make entering "1ms" raise an error
		if (sscanf(argv[i], "%f%c", &number, &suffix) < 1) {
			usage("recognition error");
			exit(1);
		}
		if (suffix == '\0') suffix = 's';
		switch(suffix) {
			case 's': waittime += number; break;
			case 'm': waittime += number*60; break;
			case 'h': waittime += number*60*60; break;
			case 'd': waittime += number*60*60*24; break;
			default: usage("unknown suffix"); exit(1); break;
		}
	}
	//printf("waittime:%f\n", waittime);
		
	struct timeval tv_wait;
	tv_wait.tv_sec = (int)floorf(waittime);
	tv_wait.tv_usec = (int)((waittime - floorf(waittime)) * 1000000);
	
	struct timeval tv_start;
	struct timezone tz;
	if (gettimeofday(&tv_start, &tz) == -1) {
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
		struct timezone tz_now;
		if (gettimeofday(&tv_now, &tz_now) == -1) {
			perror("gettimeofday error");
			exit(2);
		}
		if (!timercmp(&tv_now, &tv_stop, <)) {
			break;
		}
		
		struct timeval tv_rem; // remaining
		timersub(&tv_stop, &tv_now, &tv_rem);
		
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
	
	printf("\n");
	
	return 0;
}
