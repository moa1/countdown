#include <time.h>

void print_tm(const struct tm* time);
int compare_tm(const void *t1, const void *t2);
int try_strptime(const char* s, const char* format, struct tm* tm);
double tm_diff(const struct tm* a, const struct tm* b);
void get_tm_now(struct tm* tm_now);
int parse_with_strptime(char *time, struct tm* parsed_time, const struct tm * const tm_now);
double tm_diff_to_now_seconds(const struct tm* tm_time);
