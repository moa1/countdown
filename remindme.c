#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "timefunctions.h"

int verbose_parsing = 0;

void usage(char *msg) {
	fprintf(stderr, "Usage: [OPTIONS] DATESFILE\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "OPTIONS:\n");
	fprintf(stderr, "  -u  output reminders in the order in DATESFILE (sorting by date is default)\n");
	fprintf(stderr, "  -c  enable color output\n");
	fprintf(stderr, "  -v  verbose output\n");
	fprintf(stderr, "  -p  verbose parsing of DATESFILE (for debugging DATES)\n");
	fprintf(stderr, "  -d  enable debugging output\n");
	fprintf(stderr, "  -h  print this help\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "DATESFILE must contain one or multiple 'DATE / MESSAGE' lines.\n");
	fprintf(stderr, "It may also contain comments starting with '#' and extending to the end of line.\n");
	fprintf(stderr, "If DATESFILE is specified with '-- -', reminders are read from standard input.\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "DATE can have one of the following formats:\n");
	fprintf(stderr, "7:4 or 07:04 (today or tomorrow, seconds=0)\n");
	fprintf(stderr, "7:4:59 (today or tomorrow)\n");
	fprintf(stderr, "Monday 7:4 (weekday, in this or next week, seconds=0)\n");
	fprintf(stderr, "Monday 7:4:59 (weekday, in this or next week)\n");
	fprintf(stderr, "2014-07-22 7:4 (seconds=0)\n");
	fprintf(stderr, "2014-07-22 7:4:59\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "MESSAGE may be any string.\n");
	fprintf(stderr, "\n");
	if (msg != NULL) {
		fprintf(stderr, "Error: %s\n", msg);
	}
}

typedef struct reminder_struct {
	struct tm *date;
	char *message;
} reminder;

void print_reminder(const reminder *reminder) {
	printf("date=%s", asctime(reminder->date));
	printf("message=%s\n", reminder->message);
}

void parse_datesfile(FILE *stream, int *parsed_reminders_num, reminder **parsed_reminders) {
	void* safe_malloc(int bytes) {
		void* ptr = malloc(bytes);
		if (ptr == NULL) {
			perror("malloc error");
			exit(3);
		}
		return ptr;
	}
	
	int reminders_num = 0;
	reminder *reminders = NULL;

	// add `tm_date` to `reminders`.
	void add_reminder(const struct tm *tm_date, const char *message) {
		reminders = (reminder*)realloc(reminders, sizeof(reminder) * (reminders_num + 1));
		if (reminders == NULL) {
			perror("realloc error");
			exit(3);
		}
		reminders[reminders_num].date = (struct tm*)safe_malloc(sizeof(struct tm));
		memcpy(reminders[reminders_num].date, tm_date, sizeof(struct tm));
		int length = 0;
		for (length = 0;; length++) { if (message[length] == '\0') { length++; break;}}
		reminders[reminders_num].message = (char*)safe_malloc(sizeof(char) * length);
		strcpy(reminders[reminders_num].message, message);
		//print_reminder(&reminders[reminders_num]);
		reminders_num++;
	}
	
	const int bufsize = 1024;
	char buf[bufsize];
	int field_num = 0;
	int field_count = 0;
	char field[bufsize];

	void buf_add_char(char ch) {
		field[field_count++] = ch;
		if (field_count > bufsize) {
			fprintf(stderr, "Error: a line may only contain %i bytes\n", bufsize);
			exit(4);
		}
	}

	int char_is_whitespace(char ch) {
		return ch == ' ' || ch == '\t' || ch == '\r';
	}

	struct tm tm_date;
	void parse_date(char* field) {
		if (verbose_parsing) fprintf(stderr, "parsing DATE '%s'\n", field);
		if (!parse_with_strptime(field, &tm_date)) {
			const char *midnight = " 0:0:0";
			strncat(field, midnight, bufsize-strlen(field));
			// try again
			if (verbose_parsing) fprintf(stderr, "parsing DATE '%s'\n", field);
			if (!parse_with_strptime(field, &tm_date)) {
				usage("wrong DATE format:");
				fprintf(stderr, "%s\n", field);
				exit(2);
			}
		}
	}
	
	int state = 0; // ignore whitespace, wait for date
	do {
		int read_count = fread(buf, sizeof(char), bufsize, stream);

		for (int i=0; i<read_count; i++) {
			char ch = buf[i];
			//printf("state=%i ch=%i\n",state,ch);
			if (state == -1) {
				if (char_is_whitespace(ch) || ch == '\n') {
					// ignore
				} else {
					state = 0; //read date
				}
			}
			if (ch == '#') {
				state = 1; //comment
			} else if (state == -1) {
				// ignore
			} else if (state == 1) {
				// skip comment
				if (ch == '\n') {
					state = 0;
				}
			} else if (state == 0 && char_is_whitespace(ch)) {
				state = 2; //whitespace
			} else if (state == 2 && char_is_whitespace(ch)) {
				// ignore whitespace
			} else if (state == 0 || (state == 2 && !char_is_whitespace(ch))) {
				if (state == 2) {
					if (ch != '/')
						field[field_count++] = ' ';
					state = 0;
				}
				if (ch == '/') {
					field[field_count++] = '\0';
					parse_date(field); //this assigns `tm_date`
					field_count = 0;
					state = 3; //skip to beginning of message
				} else {
					buf_add_char(ch);
				}
			} else if (state == 3 && char_is_whitespace(ch)) {
				// skip whitespace
			} else if (state == 4 || (state == 3 && !char_is_whitespace(ch))) {
				state = 4;
				if (ch == '\n') {
					field[field_count++] = '\0';
					add_reminder(&tm_date, field);
					field_count = 0;
					state = -1;
				} else {
					buf_add_char(ch);
				}
			} else {
				fprintf(stderr, "internal error: state %i unknown!\n", state);
				exit(4);
			}
		}
	} while (!feof(stream));

	*parsed_reminders_num = reminders_num;
	*parsed_reminders = reminders;
}

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

int main(int argc, const char **argv) {
	if (argc < 2) {
		usage("Must specify an input file");
		exit(1);
	}

	int sorted = 1;
	int colors = 0;
	int verbose = 0;
	int debug = 0;
	char *filename;
	{
		int no_opts = 0;
		for (int i=0; i<argc; i++) {
			const char *arg = argv[i];
			if (strcmp(arg, "--") == 0) {
				no_opts = 1;
			} else if (strcmp(arg, "-u") == 0) {
				sorted = 0;
			} else if (strcmp(arg, "-c") == 0) {
				colors = 1;
			} else if (strcmp(arg, "-v") == 0) {
				verbose++;
			} else if (strcmp(arg, "-p") == 0) {
				verbose_parsing++;
			} else if (strcmp(arg, "-d") == 0) {
				debug++;
			} else if (strcmp(arg, "-h") == 0) {
				usage(NULL);
				exit(0);
			} else if (arg[0] != '-' || no_opts) {
				filename = (char*)arg;
			} else {
				usage(NULL);
				fprintf(stderr, "Unknown option '%s'\n", arg);
				exit(1);
			}
		}
	}

	int reminders_num;
	reminder *reminders;
	{
		FILE *stream;
		if (strcmp(filename, "-") == 0) {
			stream = stdin;
		} else {
			stream = fopen(filename, "r");
			if (stream == NULL) {
				perror("fopen error");
				exit(3);
			}
		}
		parse_datesfile(stream, &reminders_num, &reminders);
		if (strcmp(filename, "-") != 0) {
			if (!fclose(stream) == -1) {
				perror("fclose error");
				exit(3);
			}
		}
	}

	if (debug)
		printf("Number of reminders: %i\n", reminders_num);

	int compare_reminders_tm(const void *r1p, const void *r2p) {
		const reminder *r1 = (const reminder*) r1p;
		const reminder *r2 = (const reminder*) r2p;
		return compare_tm(r1->date, r2->date);
	}
	if (sorted)
		qsort(reminders, reminders_num, sizeof(reminder), compare_reminders_tm);

	const char *color_red = colors?ANSI_COLOR_RED:"'";
	const char *color_green = colors?ANSI_COLOR_GREEN:"'";
	const char *color_yellow = colors?ANSI_COLOR_YELLOW:"'";
	const char *color_blue = colors?ANSI_COLOR_BLUE:"'";
	const char *color_magenta = colors?ANSI_COLOR_MAGENTA:"'";
	const char *color_cyan = colors?ANSI_COLOR_CYAN:"'";
	const char *color_reset = colors?ANSI_COLOR_RESET:"'";

	int first_start = 1;
	int first_hours = 1;
	int first_today = 1;
	int first_week = 1;
	for (int i = 0; i < reminders_num; i++) {
		double seconds = tm_diff_to_now_seconds(reminders[i].date);
		if (debug) {
			printf("seconds=%f ", seconds); print_reminder(&reminders[i]);
		}

		char *date = asctime(reminders[i].date);
		if (date[strlen(date)-1] == '\n') {
			date[strlen(date)-1] = '\0';
		}
		char *message = reminders[i].message;
		int d_rem = (int)seconds / (60*60*24);
		int d_int = (int)ceil(seconds / (60*60*24));
		int h_rem = ((int)seconds % (60*60*24)) / (60*60);
		int h_int = (int)ceil((int)seconds % (60*60*24)) / (60*60);
		int m_rem = ((int)seconds % (60*60)) / 60;
		int m_int = (int)ceil((int)seconds % (60*60)) / 60;

		const int hours_3 = 60*60*3;
		const int day = 24*60*60;
		const int day_7 = day*7;

		if (seconds < -day) {
			// do nothing
		} else if (seconds < 0) {
			first_hours = 1; first_today = 1; first_week = 1;
			if (first_start && verbose) {
				printf("Today:\n");
				first_start = 0;
			}
			printf("%2i hours %2i minutes since %s%s%s\n", h_rem, m_rem, color_magenta, message, color_reset);
		} else if (seconds < hours_3) {
			first_start = 1; first_today = 1; first_week = 1;
			if (first_hours && verbose) {
				printf("Within 3 hours:\n");
				first_hours = 0;
			}
			printf("%2i hours %2i minutes until %s%s%s\n", h_rem, m_rem, color_blue, message, color_reset);
		} else if (seconds < day) {
			first_start = 1; first_hours = 1; first_week = 1;
			if (first_today && verbose) {
				printf("Within 24 hours:\n");
				first_today = 0;
			}
			printf("%2i hours until %s%s%s\n", h_int, color_green, message, color_reset);
		} else if (seconds < day_7) {
			first_start = 1; first_hours = 1; first_today = 1;
			if (first_week && verbose) {
				printf("Within a week:\n");
				first_week = 0;
			}
			printf("%i days until %s%s / %s%s\n", d_int, color_red, date, message, color_reset);
		}
	}
	
}
