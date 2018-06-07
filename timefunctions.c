// TODO: make another parse function which allows specifying the format string of strptime.

#define _XOPEN_SOURCE //strptime, localtime_r
#define _DEFAULT_SOURCE //timersub
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include "timefunctions.h"

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
