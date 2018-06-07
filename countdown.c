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

// for debugging
void print_tm(const struct tm* t) {
	printf("%s\n", asctime(t));
}

/* Try to parse string s with the date-time-format format using strptime. Return 0 if the parsing failed (in which case tm is not modified) and 1 if every element of format was parsed (in this case tm is set to the parsed date-time). */
int try_strptime(const char* s, const char* format, struct tm* tm) {
	struct tm tm_parsed;
	memcpy(&tm_parsed, tm, sizeof(tm_parsed));
	char* r = strptime(s, format, &tm_parsed);
	if (r == NULL || *r != '\0')
		return 0;
	memcpy(tm, &tm_parsed, sizeof(*tm));
	return 1;
}

/* Return the time difference between a and b in seconds. */
double tm_diff(const struct tm* a, const struct tm* b) {
	struct tm tmp_a;
	memcpy(&tmp_a, a, sizeof(tmp_a));
	time_t t_a = mktime(&tmp_a);
	if (t_a == -1) {
		perror("mktime error");
		exit(2);
	}
	struct tm tmp_b;
	memcpy(&tmp_b, b, sizeof(tmp_b));
	time_t t_b = mktime(&tmp_b);
	if (t_b == -1) {
		perror("mktime error");
		exit(2);
	}
	
	return difftime(t_a, t_b);
}

/* Recognizes the following formats:
"%H:%M" (today or tomorrow, seconds=0)
"%H:%M:%S" (today or tomorrow)
"%A %H:%M" (weekday, in this or next week, seconds=0)
"%A %H:%M:%S" (weekday, in this or next week)
"%Y-%m-%d %H:%M" (error if in the past, seconds=0)
"%Y-%m-%d %H:%M:%S" (error if in the past)
Returns 1 if it succeeded, and in this case sets parsed_time.
Returns 0 if parsing did not conform to one of the above formats. */
int parse_with_strptime(int argc, char** argv, 	struct tm* parsed_time) {
	// TODO: Sometimes, when running "countdown 1:0:59", it wants to wait until 1:0:58, not 1:0:59.
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
	
	struct tm tm_now;
	{
		// initialize tm with now.
		struct timeval tv_now;
		if (gettimeofday(&tv_now, NULL) == -1) {
			perror("gettimeofday error");
			exit(2);
		}
		if (!localtime_r(&(tv_now.tv_sec), &tm_now)) {
			perror("localtime_r error");
			exit(2);
		}
	}
	
	// init tm_stop
	struct tm tm_stop;
	memcpy(&tm_stop, &tm_now, sizeof(tm_stop));
	tm_stop.tm_sec = 0;
	
	if (try_strptime(args, "%H:%M", &tm_stop) || try_strptime(args, "%H:%M:%S", &tm_stop)) {
		if (tm_diff(&tm_stop, &tm_now) < 0) {
			tm_stop.tm_mday += 1;	// tomorrow
		}
	} else if (try_strptime(args, "%A%n%H:%M", &tm_stop) || try_strptime(args, "%A%n%H:%M:%S", &tm_stop)) {	//I'm not sure %A sets the whole date, it might set only tm_wday, in which case mktime normalizes a non-matching tm_wday away.
		// we need to transfer the info in tm_stop.tm_wday to tm_stop.mday, because mktime ignores tm_wday.
		int days_diff = tm_stop.tm_wday - tm_now.tm_wday;
		if (days_diff < 0)
			days_diff += 7;
		tm_stop.tm_mday += days_diff;
		// we still need to check if tm_stop is in the past, because tm_stop might be from today with a daytime before now.
		if (tm_diff(&tm_stop, &tm_now) < 0) {
			tm_stop.tm_mday += 7;	// next week
		}
	} else if (try_strptime(args, "%Y-%m-%d%n%H:%M", &tm_stop)) {
	} else if (try_strptime(args, "%Y-%m-%d%n%H:%M:%S", &tm_stop)) {
	} else {
		return 0; //time parsing error
	}
	
	memcpy(parsed_time, &tm_stop, sizeof(tm_stop));
	//print_tm(parsed_time);
	return 1;
}

/* Return how many seconds `tm_time` is in the future. */
double tm_diff_to_now_seconds(const struct tm* tm_time) {
	// convert `tm_time` to `time_t` and then to `timeval`-structure `tv_stop`
	struct timeval tv_stop;
	{
		struct tm tm_stop;
		memcpy(&tm_stop, tm_time, sizeof(tm_stop));
		
		time_t time_stop = mktime(&tm_stop);
		if (time_stop == -1) {
			perror("mktime error");
			exit(2);
		}

		tv_stop.tv_sec = time_stop;
		tv_stop.tv_usec = 0;
	}
	
	// compute difference between now and tv_stop
	double waittime;
	{
		struct timeval tv_now;
		if (gettimeofday(&tv_now, NULL) == -1) {
			perror("gettimeofday error");
			exit(2);
		}

		struct timeval tv_diff;
		timersub(&tv_stop, &tv_now, &tv_diff);
		waittime = (double)tv_diff.tv_usec/1000000 + tv_diff.tv_sec;
	}

	return waittime;
}

// TODO: make another parse function which allows specifying the format string of strptime.

int main(int argc, char** argv) {
	if (argc < 2) {
		usage("need at least one number");
		exit(1);
	}
	double waittime;
	{
		struct tm parsed_time;
		if (parse_with_strptime(argc, argv, &parsed_time)) {
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
