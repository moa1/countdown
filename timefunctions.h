void print_tm(const struct tm* t);
int try_strptime(const char* s, const char* format, struct tm* tm);
double tm_diff(const struct tm* a, const struct tm* b);
int parse_with_strptime(int argc, char** argv, 	struct tm* parsed_time);
double tm_diff_to_now_seconds(const struct tm* tm_time);
