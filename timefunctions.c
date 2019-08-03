
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
void print_tm(const struct tm* time) {
	printf("sec=%i min=%i hour=%i mday=%i mon=%i year=%i wday=%i yday=%i isdst=%i\n", time->tm_sec, time->tm_min, time->tm_hour, time->tm_mday, time->tm_mon, time->tm_year, time->tm_wday, time->tm_yday, time->tm_isdst);
	printf("%s\n", asctime(time));
}

int compare_tm(const void *t1, const void *t2) {
	const struct tm *tm_t1 = (const struct tm *)t1;
	const struct tm *tm_t2 = (const struct tm *)t2;
	return (int)tm_diff(tm_t1, tm_t2);
}

/* Try to parse string s with the date-time-format format using strptime. Return 0 if the parsing failed (in which case tm is not modified) and 1 if every element of format was parsed (in this case tm is set to the parsed date-time). */
int try_strptime(const char* s, const char* format, struct tm* tm, char** rest) {
	struct tm tm_parsed;
	memcpy(&tm_parsed, tm, sizeof(tm_parsed));
	char* rest_tmp;
	rest_tmp = strptime(s, format, &tm_parsed);
	if (rest != NULL)
		*rest = rest_tmp;
	//if (rest_tmp == NULL || *rest_tmp != '\0')
	if (rest_tmp == NULL)
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
		perror("mktime error: maybe time too far into the future");
		exit(2);
	}
	struct tm tmp_b;
	memcpy(&tmp_b, b, sizeof(tmp_b));
	time_t t_b = mktime(&tmp_b);
	if (t_b == -1) {
		perror("mktime error: maybe time too far into the future");
		exit(2);
	}
	
	return difftime(t_a, t_b);
}

void get_tm_now(struct tm* tm_now) {
	// initialize tm with now.
	struct timeval tv_now;
	if (gettimeofday(&tv_now, NULL) == -1) {
		perror("gettimeofday error");
		exit(2);
	}
	if (!localtime_r(&(tv_now.tv_sec), tm_now)) {
		perror("localtime_r error");
		exit(2);
	}
}

int try_localtime(const char* time, struct tm* tm, char** rest) {
	if (time[0] == '@') {
		time_t sse = strtol(&time[1], rest, 10);
		return (localtime_r(&sse, tm) != NULL);
	}
	return 0;
}

/* Recognizes the following formats:
"%H:%M" (today or tomorrow, seconds=0)
"%H:%M:%S" (today or tomorrow)
"%A %H:%M" (weekday, in this or next week, seconds=0)
"%A %H:%M:%S" (weekday, in this or next week)
"%Y-%m-%d" (error if in the past, seconds=0)
"%Y-%m-%d %H" (error if in the past, seconds=0)
"%Y-%m-%d %H:%M" (error if in the past, seconds=0)
"%Y-%m-%d %H:%M:%S" (error if in the past)
"%Y%m%d %H%M%S" (error if in the past)
"%Y%m%d %H%M" (error if in the past)
"%Y%m%d %H" (error if in the past)
"%Y%m%d" (error if in the past)
"@%s" (seconds since the Epoch 1970-01-01)
Returns 1 if it succeeded, and in this case sets parsed_time.
Returns 0 if parsing did not conform to one of the above formats. */
int parse_with_strptime(char *time, const struct tm * const tm_now, struct tm* parsed_time, char** rest) {
	// TODO: Sometimes, when running "countdown 1:0:59", it wants to wait until 1:0:58, not 1:0:59.
	
	// init tm_stop
	struct tm tm_stop;
	memcpy(&tm_stop, tm_now, sizeof(tm_stop));
	tm_stop.tm_sec = 0;
	
	if (try_strptime(time, "%H:%M", &tm_stop, rest) || try_strptime(time, "%H:%M:%S", &tm_stop, rest)) {
		if (tm_diff(&tm_stop, tm_now) < 0) {
			tm_stop.tm_mday += 1;	// tomorrow
		}
	} else if (try_strptime(time, "%A%n%H:%M", &tm_stop, rest) || try_strptime(time, "%A%n%H:%M:%S", &tm_stop, rest)) {	//I'm not sure %A sets the whole date, it might set only tm_wday, in which case mktime normalizes a non-matching tm_wday away.
		// we need to transfer the info in tm_stop.tm_wday to tm_stop.mday, because mktime ignores tm_wday.
		int days_diff = tm_stop.tm_wday - tm_now->tm_wday;
		if (days_diff < 0)
			days_diff += 7;
		tm_stop.tm_mday += days_diff;
		// we still need to check if tm_stop is in the past, because tm_stop might be from today with a daytime before now.
		if (tm_diff(&tm_stop, tm_now) < 0) {
			tm_stop.tm_mday += 7;	// next week
		}
	} else if (try_strptime(time, "%Y-%m-%d%n%H:%M:%S", &tm_stop, rest)) {
	} else if (try_strptime(time, "%Y-%m-%d%n%H:%M", &tm_stop, rest)) {
	} else if (try_strptime(time, "%Y-%m-%d%n%H", &tm_stop, rest)) {
	} else if (try_strptime(time, "%Y-%m-%d%n", &tm_stop, rest)) {
	} else if (try_strptime(time, "%Y%n%m%n%d%n%H%n%M%n%S", &tm_stop, rest)) {
	} else if (try_strptime(time, "%Y%n%m%n%d%n%H%n%M", &tm_stop, rest)) {
	} else if (try_strptime(time, "%Y%n%m%n%d%n%H", &tm_stop, rest)) {
	} else if (try_strptime(time, "%Y%n%m%n%d", &tm_stop, rest)) {
	} else if (try_localtime(time, &tm_stop, rest)) {
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
			perror("mktime error: maybe time too far into the future");
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

/* Like `parse_with_strptime`, but with subsecond resolution. */
int parse_with_strptime_waittime(char *time, const struct tm * const tm_now, double* waittime) {
	char* rest;
	struct tm parsed_time;

	if (!parse_with_strptime(time, tm_now, &parsed_time, &rest)) return 0;
	*waittime = tm_diff_to_now_seconds(&parsed_time);
	// parsing was successful.

	char* endptr;
	double seconds = strtod(rest, &endptr);
	if (endptr != rest) {
		*waittime += seconds;
	}

	return 1;
}

void usage_of_parse_with_strptime(FILE* stream) {
	fprintf(stream, "7:4 or 07:04 (today or tomorrow, seconds=0)\n");
	fprintf(stream, "7:4:59 (today or tomorrow)\n");
	fprintf(stream, "Monday 7:4 (weekday, in this or next week, seconds=0)\n");
	fprintf(stream, "Monday 7:4:59 (weekday, in this or next week)\n");
	fprintf(stream, "2014-07-22 7:4 (error if in the past, seconds=0)\n");
	fprintf(stream, "2014-07-22 7:4:59 (error if in the past)\n");
	fprintf(stream, "@1564866700 (seconds since epoch, like 'date +%%s')\n");
}
