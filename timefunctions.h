#include <time.h>

void print_tm(const struct tm* time);
int compare_tm(const void *t1, const void *t2);
int try_strptime(const char* s, const char* format, struct tm* tm, char** rest);
double tm_diff(const struct tm* a, const struct tm* b);
void get_tm_now(struct tm* tm_now);
int parse_with_strptime(char *time, const struct tm * const tm_now, struct tm* parsed_time, char** rest);
double tm_diff_to_now_seconds(const struct tm* tm_time);
int parse_with_strptime_waittime(char *time, const struct tm * const tm_now, double *waittime);
void usage_of_parse_with_strptime(FILE* stream);
